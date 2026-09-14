#include "Characters/ChronosCharacter.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "BrainComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/CombatComponent.h"
#include "Components/HealthComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Feedback/ChronosFeedbackSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Settings/ChronosGameUserSettings.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interactables/Interactable.h"
#include "Subsystems/TimeDilationSubsystem.h"
#include "Weapons/ChronosWeapon.h"
#include "Weapons/WeaponDataAsset.h"

AChronosCharacter::AChronosCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
}

void AChronosCharacter::BeginPlay()
{
	Super::BeginPlay();

	CachedCamera = FindComponentByClass<UCameraComponent>();

	// 应用玩家设置：灵敏度与 FOV 都在设置面板里，角色是唯一知道相机在哪的一层
	if (const UChronosGameUserSettings* Settings = UChronosGameUserSettings::GetChronosSettings())
	{
		MouseSensitivity = Settings->MouseSensitivity;

		if (CachedCamera)
		{
			CachedCamera->SetFieldOfView(Settings->FieldOfView);
		}
	}

	HealthComponent->OnDeath.AddDynamic(this, &AChronosCharacter::HandleDeath);

	// 推送本角色需要的输入映射上下文（避免在多处重复推送）
	if (DefaultMappingContexts.Num() == 0)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	const ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!InputSubsystem)
	{
		return;
	}

	for (const TSoftObjectPtr<const UInputMappingContext>& ContextRef : DefaultMappingContexts)
	{
		if (const UInputMappingContext* MappingContext = ContextRef.LoadSynchronous())
		{
			InputSubsystem->AddMappingContext(MappingContext, 0);
		}
	}
}

void AChronosCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 复活/重新被控制时恢复正常时间流
	if (UTimeDilationSubsystem* TimeSubsystem = GetWorld()->GetSubsystem<UTimeDilationSubsystem>())
	{
		if (IsPlayerControlled())
		{
			TimeSubsystem->SetPlayerDead(false);
		}
	}
}

void AChronosCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateInteractionFocus();
}

void AChronosCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		return;
	}

	if (IA_Move)
	{
		Input->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AChronosCharacter::OnMove);
	}
	if (IA_Look)
	{
		Input->BindAction(IA_Look, ETriggerEvent::Triggered, this, &AChronosCharacter::OnLook);
	}
	if (IA_LookMouse)
	{
		Input->BindAction(IA_LookMouse, ETriggerEvent::Triggered, this, &AChronosCharacter::OnLook);
	}
	if (IA_Jump)
	{
		Input->BindAction(IA_Jump, ETriggerEvent::Started, this, &AChronosCharacter::OnJumpStarted);
		Input->BindAction(IA_Jump, ETriggerEvent::Completed, this, &AChronosCharacter::OnJumpEnded);
	}
	if (IA_Fire)
	{
		Input->BindAction(IA_Fire, ETriggerEvent::Started, this, &AChronosCharacter::OnFireStarted);
		// 松开：全自动武器必须靠它停止连发，否则按一次就打不停
		Input->BindAction(IA_Fire, ETriggerEvent::Completed, this, &AChronosCharacter::OnFireCompleted);
	}
	if (IA_Throw)
	{
		Input->BindAction(IA_Throw, ETriggerEvent::Started, this, &AChronosCharacter::OnThrowStarted);
	}
	if (IA_Interact)
	{
		Input->BindAction(IA_Interact, ETriggerEvent::Started, this, &AChronosCharacter::OnInteractStarted);
	}
}

void AChronosCharacter::OnMove(const FInputActionValue& Value)
{
	if (UTimeDilationSubsystem* TimeSubsystem = GetWorld()->GetSubsystem<UTimeDilationSubsystem>())
	{
		TimeSubsystem->NotifyPlayerInput();
	}

	const FVector2D MoveVector = Value.Get<FVector2D>();
	if (MoveVector.IsNearlyZero() || !Controller)
	{
		return;
	}

	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), MoveVector.Y);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), MoveVector.X);
}

void AChronosCharacter::OnLook(const FInputActionValue& Value)
{
	if (UTimeDilationSubsystem* TimeSubsystem = GetWorld()->GetSubsystem<UTimeDilationSubsystem>())
	{
		TimeSubsystem->NotifyPlayerInput();
	}

	const FVector2D LookVector = Value.Get<FVector2D>();

	// 反转 Y 轴从设置里实时读：设置面板改完切回游戏立刻生效，不需要重新 Possess
	float PitchSign = 1.f;
	if (const UChronosGameUserSettings* Settings = UChronosGameUserSettings::GetChronosSettings())
	{
		PitchSign = Settings->bInvertMouseY ? -1.f : 1.f;
	}

	AddControllerYawInput(LookVector.X * MouseSensitivity);
	AddControllerPitchInput(LookVector.Y * MouseSensitivity * PitchSign);
}

void AChronosCharacter::OnJumpStarted(const FInputActionValue& Value)
{
	if (UTimeDilationSubsystem* TimeSubsystem = GetWorld()->GetSubsystem<UTimeDilationSubsystem>())
	{
		TimeSubsystem->NotifyPlayerInput();
	}

	Jump();
}

void AChronosCharacter::OnJumpEnded(const FInputActionValue& Value)
{
	if (UTimeDilationSubsystem* TimeSubsystem = GetWorld()->GetSubsystem<UTimeDilationSubsystem>())
	{
		TimeSubsystem->NotifyPlayerInput();
	}

	StopJumping();
}

void AChronosCharacter::OnFireStarted(const FInputActionValue& Value)
{
	if (UTimeDilationSubsystem* TimeSubsystem = GetWorld()->GetSubsystem<UTimeDilationSubsystem>())
	{
		TimeSubsystem->NotifyPlayerInput();
	}

	if (!HealthComponent->IsAlive())
	{
		return;
	}

	TScriptInterface<IWeaponUser> Weapon = CombatComponent->GetCurrentWeapon();
	if (Weapon.GetObject())
	{
		// 接口自带冷却判断；蓝图实现里直接转调现成的 StartFiring
		IWeaponUser::Execute_Fire(Weapon.GetObject());
	}
}

void AChronosCharacter::OnFireCompleted(const FInputActionValue& Value)
{
	// 松开就停火：全自动武器在 Tick 里按射速连发，靠这里结束。
	// 武器切换/投掷/死亡时 EnterWorldState 也会置 bIsFiring=false 兜底。
	TScriptInterface<IWeaponUser> Weapon = CombatComponent->GetCurrentWeapon();
	if (Weapon.GetObject())
	{
		IWeaponUser::Execute_StopFiring(Weapon.GetObject());
	}
}

void AChronosCharacter::OnThrowStarted(const FInputActionValue& Value)
	{
		if (UTimeDilationSubsystem* TimeSubsystem = GetWorld()->GetSubsystem<UTimeDilationSubsystem>())
		{
			TimeSubsystem->NotifyPlayerInput();
		}

		if (CombatComponent->GetCurrentWeapon().GetObject())
		{
			// 先扔：内部会还原无武器动画实例，之后播投掷蒙太奇才会落在正确的实例上
			CombatComponent->ThrowCurrentWeapon(GetAimDirection());
			PlayWeaponMontage(ThrowMontage);

			if (UChronosFeedbackSubsystem* Feedback = GetWorld()->GetSubsystem<UChronosFeedbackSubsystem>())
			{
				Feedback->NotifyWeaponThrown(GetActorLocation());
			}
		}
	}

void AChronosCharacter::OnInteractStarted(const FInputActionValue& Value)
{
	if (UTimeDilationSubsystem* TimeSubsystem = GetWorld()->GetSubsystem<UTimeDilationSubsystem>())
	{
		TimeSubsystem->NotifyPlayerInput();
	}

	if (UObject* Focus = FocusedInteractable.Get())
	{
		if (IInteractable* Interactable = Cast<IInteractable>(Focus))
		{
			Interactable->OnInteract(this);
		}
	}
}

USkeletalMeshComponent* AChronosCharacter::GetFirstPersonMesh() const
{
	TArray<USkeletalMeshComponent*> SkeletalMeshes;
	GetComponents<USkeletalMeshComponent>(SkeletalMeshes);
	for (USkeletalMeshComponent* SkeletalMesh : SkeletalMeshes)
	{
		if (SkeletalMesh && SkeletalMesh->GetName().Contains(TEXT("FirstPersonMesh")))
		{
			return SkeletalMesh;
		}
	}

	return nullptr;
}

USkeletalMeshComponent* AChronosCharacter::GetHeldWeaponAttachComponent() const
{
	// 优先第一人称手臂网格（仅拥有者可见），找不到则退回第三人称网格
	if (USkeletalMeshComponent* FirstPersonMesh = GetFirstPersonMesh())
	{
		return FirstPersonMesh;
	}

	return GetMesh();
}

void AChronosCharacter::CacheDefaultAnimClasses()
{
	if (bDefaultAnimClassesCached)
	{
		return;
	}
	bDefaultAnimClassesCached = true;

	if (!DefaultFirstPersonAnimClass)
	{
		if (const USkeletalMeshComponent* FirstPersonMesh = GetFirstPersonMesh())
		{
			DefaultFirstPersonAnimClass = FirstPersonMesh->AnimClass;
		}
	}

	if (!DefaultThirdPersonAnimClass)
	{
		if (const USkeletalMeshComponent* BodyMesh = GetMesh())
		{
			DefaultThirdPersonAnimClass = BodyMesh->AnimClass;
		}
	}
}

void AChronosCharacter::SetWeaponAnimClasses(TSubclassOf<UAnimInstance> FirstPersonAnimClass,
	TSubclassOf<UAnimInstance> ThirdPersonAnimClass)
{
	CacheDefaultAnimClasses();

	if (FirstPersonAnimClass)
	{
		if (USkeletalMeshComponent* FirstPersonMesh = GetFirstPersonMesh())
		{
			FirstPersonMesh->SetAnimInstanceClass(FirstPersonAnimClass);
		}
	}

	if (ThirdPersonAnimClass && GetMesh())
	{
		GetMesh()->SetAnimInstanceClass(ThirdPersonAnimClass);
	}
}

void AChronosCharacter::RestoreDefaultAnimClasses()
{
	CacheDefaultAnimClasses();

	if (DefaultFirstPersonAnimClass)
	{
		if (USkeletalMeshComponent* FirstPersonMesh = GetFirstPersonMesh())
		{
			if (FirstPersonMesh->AnimClass != DefaultFirstPersonAnimClass)
			{
				FirstPersonMesh->SetAnimInstanceClass(DefaultFirstPersonAnimClass);
			}
		}
	}

	if (DefaultThirdPersonAnimClass && GetMesh() && GetMesh()->AnimClass != DefaultThirdPersonAnimClass)
	{
		GetMesh()->SetAnimInstanceClass(DefaultThirdPersonAnimClass);
	}
}

float AChronosCharacter::PlayWeaponMontage(UAnimMontage* Montage, float PlayRate)
{
	if (!Montage)
	{
		return 0.f;
	}

	// 本地玩家播在第一人称手臂上；敌人（无本地控制）播在身体网格上
	USkeletalMeshComponent* TargetMesh = IsLocallyControlled() ? GetFirstPersonMesh() : nullptr;
	if (!TargetMesh)
	{
		TargetMesh = GetMesh();
	}
	if (!TargetMesh)
	{
		return 0.f;
	}

	UAnimInstance* AnimInstance = TargetMesh->GetAnimInstance();
	if (!AnimInstance)
	{
		return 0.f;
	}

	return AnimInstance->Montage_Play(Montage, PlayRate);
}

FName AChronosCharacter::GetHeldWeaponSocketName() const
{
	// 数据资产里配置的插槽名在当前网格上不存在时，
	// 依次回退：模版手臂网格的握把插槽 → Manny 骨骼的标准手部骨骼
	if (const USkeletalMeshComponent* AttachMesh = GetHeldWeaponAttachComponent())
	{
		const FName RequestedSocket = TEXT("hand_rWeaponSocket");
		if (AttachMesh->DoesSocketExist(RequestedSocket))
		{
			return RequestedSocket;
		}

		if (AttachMesh->DoesSocketExist(TEXT("HandGrip_R")))
		{
			return TEXT("HandGrip_R");
		}

		if (AttachMesh->DoesSocketExist(TEXT("hand_r")))
		{
			return TEXT("hand_r");
		}
	}

	return NAME_None;
}

FVector AChronosCharacter::GetAimDirection() const
{
	if (CachedCamera)
	{
		return CachedCamera->GetComponentRotation().Vector();
	}

	return GetBaseAimRotation().Vector();
}

FVector AChronosCharacter::GetAimTargetPoint() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return GetActorLocation();
	}

	const FVector Start = CachedCamera ? CachedCamera->GetComponentLocation() : GetActorLocation();
	const FVector Direction = GetAimDirection();
	const FVector End = Start + Direction * 10000.f;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(AimTrace), false, this);
	FHitResult HitResult;
	if (World->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
	{
		return HitResult.ImpactPoint;
	}

	return End;
}

void AChronosCharacter::UpdateInteractionFocus()
{
	UWorld* World = GetWorld();
	if (!World || !IsPlayerControlled())
	{
		return;
	}

	const FVector Start = CachedCamera ? CachedCamera->GetComponentLocation() : GetActorLocation();
	const FVector End = Start + GetAimDirection() * 300.f;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractFocus), false, this);
	FHitResult HitResult;

	UObject* NewFocus = nullptr;
	FText FocusText;

	if (World->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params) &&
		HitResult.GetActor() &&
		HitResult.GetActor()->Implements<UInteractable>())
	{
		IInteractable* Interactable = Cast<IInteractable>(HitResult.GetActor());
		if (Interactable && Interactable->CanInteract(this))
		{
			NewFocus = HitResult.GetActor();
			FocusText = Interactable->GetInteractionText();
		}
	}

	if (NewFocus != FocusedInteractable.Get())
	{
		FocusedInteractable = NewFocus;
		OnInteractionFocusChanged.Broadcast(NewFocus != nullptr, FocusText);
	}
}

void AChronosCharacter::HandleDeath(AActor* DamagedActor, AActor* Killer)
{
	// 停止移动与碰撞，交出时间控制权（玩家死亡时世界恢复正常流速）
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CombatComponent->DropCurrentWeapon();

	if (IsPlayerControlled())
	{
		if (UTimeDilationSubsystem* TimeSubsystem = GetWorld()->GetSubsystem<UTimeDilationSubsystem>())
		{
			TimeSubsystem->SetPlayerDead(true);
		}

		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			DisableInput(PC);
		}
	}
	else if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		// 必须先停掉 AI 逻辑再 UnPossess。StateTree 的上下文是「Actor + Controller」，
		// UnPossess 以及随后（蓝图里 10s 后）的 DestroyActor 会让这个上下文失效，
		// 而 StateTree 组件仍在 Tick，于是持续报
		// "The tree started with a valid context and it's now invalid"。
		// StopLogic 还会走一遍 ExitState，正好连带清理敌人的连发计时器。
		if (UBrainComponent* Brain = AIController->GetBrainComponent())
		{
			Brain->StopLogic(TEXT("Death"));
		}

		AIController->UnPossess();
	}
	else if (AController* ControllerRef = GetController())
	{
		ControllerRef->UnPossess();
	}

	// 布娃娃表现（网格存在时）
	if (GetMesh() && GetMesh()->GetPhysicsAsset())
	{
		GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetSimulatePhysics(true);
	}

	OnCharacterDeath.Broadcast(this);
}
