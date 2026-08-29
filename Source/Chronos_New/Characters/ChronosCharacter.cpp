#include "Characters/ChronosCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/CombatComponent.h"
#include "Components/HealthComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
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
	AddControllerYawInput(LookVector.X * MouseSensitivity);
	AddControllerPitchInput(LookVector.Y * MouseSensitivity);
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

void AChronosCharacter::OnThrowStarted(const FInputActionValue& Value)
	{
		if (UTimeDilationSubsystem* TimeSubsystem = GetWorld()->GetSubsystem<UTimeDilationSubsystem>())
		{
			TimeSubsystem->NotifyPlayerInput();
		}

		if (CombatComponent->GetCurrentWeapon().GetObject())
		{
			CombatComponent->ThrowCurrentWeapon(GetAimDirection());
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

USkeletalMeshComponent* AChronosCharacter::GetHeldWeaponAttachComponent() const
{
	// 优先第一人称手臂网格（仅拥有者可见），找不到则退回第三人称网格
	TArray<USkeletalMeshComponent*> SkeletalMeshes;
	GetComponents<USkeletalMeshComponent>(SkeletalMeshes);
	for (USkeletalMeshComponent* SkeletalMesh : SkeletalMeshes)
	{
		if (SkeletalMesh && SkeletalMesh->GetName().Contains(TEXT("FirstPersonMesh")))
		{
			return SkeletalMesh;
		}
	}

	return GetMesh();
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
