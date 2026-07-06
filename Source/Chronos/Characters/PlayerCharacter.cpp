// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "Chronos/Chronos.h"
#include "Chronos/Components/HealthComponent.h"
#include "Chronos/Components/WeaponComponent.h"
#include "Chronos/Subsystems/TimeDilationSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

// Sets default values
APlayerCharacter::APlayerCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	// 创建并设置生命值组件
    HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	
	// 创建并设置武器组件
	WeaponComponent = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComponent"));
	
	// 创建弹簧臂组件
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = 0.0f;
	SpringArmComponent->bUsePawnControlRotation = true;
	
	// 创建相机组件
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent);
	CameraComponent->bUsePawnControlRotation = false;
	
	// 禁用角色自动旋转，由输入控制
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// 设置角色移动配置
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// 绑定死亡事件
	BindDeathEvent();
	
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			InputSubsystem->AddMappingContext(InputMappingContext, 0);
		}
	}
	
	// 初始化输入活动状态
	bHasInputActivity = false;
	bIsDead = false;
	
	// 通知时间子系统玩家存活
	if (UTimeDilationSubsystem* TimeSubsystem = ChronosSubsystems::GetTimeDilationSubsystem(GetWorld()))
	{
		TimeSubsystem->SetPlayerDead(false);
	}
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// 如果玩家已死亡，不处理任何输入逻辑
	if (bIsDead)
	{
		return;
	}

	// 更新输入活动状态并通知时间子系统
	UpdateInputActivity();
}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	// 获取增强输入组件
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 绑定移动输入
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::HandleMoveInput);

		// 绑定视角输入
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::HandleLookInput);

		// 绑定射击输入（按下时触发）
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &APlayerCharacter::HandleFireInput);

		// 绑定换弹输入（按下时触发）
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &APlayerCharacter::HandleReloadInput);
	}
}

bool APlayerCharacter::IsAlive() const
{
	return HealthComponent ? HealthComponent->IsAlive() : true;
}

void APlayerCharacter::OnPlayerDeath(AActor* Killer)
{
	if (bIsDead)
	{
		return;
	}
	
	bIsDead = true;

	// 禁用角色移动
	GetCharacterMovement()->DisableMovement();

	// 禁用输入
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		PlayerController->DisableInput(PlayerController);
	}

	// 通知时间子系统玩家死亡（恢复正常时间流速）
	if (UTimeDilationSubsystem* TimeSubsystem = ChronosSubsystems::GetTimeDilationSubsystem(GetWorld()))
	{
		TimeSubsystem->SetPlayerDead(true);
	}

	// 停止武器系统（当前武器无自动射击逻辑，无需额外处理）
}

void APlayerCharacter::RespawnPlayer()
{
	bIsDead = false;
	
	// 恢复生命值
	if (HealthComponent)
	{
		HealthComponent->Revive();
	}
	
	// 重置武器状态
	if (WeaponComponent && WeaponComponent->GetCurrentWeapon())
	{
		WeaponComponent->EquipWeapon(WeaponComponent->GetCurrentWeapon());
	}
	
	// 启用角色移动
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	
	// 启用输入
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		PlayerController->EnableInput(PlayerController);
	}
	
	// 重置视角
	CurrentPitch = 0.0f;
	Controller->SetControlRotation(FRotator::ZeroRotator);

	// 通知时间子系统玩家存活
	if (UTimeDilationSubsystem* TimeSubsystem = ChronosSubsystems::GetTimeDilationSubsystem(GetWorld()))
	{
		TimeSubsystem->SetPlayerDead(false);
	}
}

void APlayerCharacter::HandleMoveInput(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}
	
	bHasInputActivity = true;
	
	// 获取移动方向向量
	const FVector2D MoveDirection = Value.Get<FVector2D>();

	// 获取控制器的旋转方向
	const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	
	// 计算前后方向
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	// 计算左右方向
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	
	// 移动角色
	AddMovementInput(ForwardDirection, MoveDirection.Y);
	AddMovementInput(RightDirection, MoveDirection.X);
}

void APlayerCharacter::HandleLookInput(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}
	
	bHasInputActivity = true;
	
	const FVector2D LookDelta = Value.Get<FVector2D>() * MouseSensitivity;
	
	// 处理Y轴反转
	const float YAxisValue = bInvertYAxis ? -LookDelta.Y : LookDelta.Y;
	
	// 更新俯仰角度
	CurrentPitch = FMath::Clamp(CurrentPitch + YAxisValue, -MaxPitchAngle, MaxPitchAngle);
	
	// 旋转控制器（AddControllerYawInput 和 AddControllerPitchInput 是 APawn 的方法）
	AddControllerYawInput(LookDelta.X);
	AddControllerPitchInput(YAxisValue);
}

void APlayerCharacter::HandleFireInput()
{
	if (bIsDead)
	{
		return;
	}

	// 标记有输入活动
	bHasInputActivity = true;
	bIsFiring = true;

	// 如果武器组件存在且可以射击，则执行射击
	if (WeaponComponent && WeaponComponent->CanFire())
	{
		WeaponComponent->Fire();
	}

	// 射击状态会在 UpdateInputActivity 中每帧自动重置
}

void APlayerCharacter::HandleReloadInput()
{
	if (bIsDead)
	{
		return;
	}

	// 标记有输入活动
	bHasInputActivity = true;
	bIsReloading = true;

	// 如果武器组件存在且可以换弹，则执行换弹
	if (WeaponComponent && WeaponComponent->CanReload())
	{
		WeaponComponent->Reload();
	}
}

void APlayerCharacter::BindDeathEvent()
{
	if (HealthComponent)
	{
		// 将死亡事件绑定到处理方法
		HealthComponent->OnDeath.AddDynamic(this, &APlayerCharacter::OnPlayerDeath);
	}
}

void APlayerCharacter::UpdateInputActivity()
{
	// 检测是否有输入活动
	bool bCurrentInputActivity = bHasInputActivity || bIsFiring || bIsReloading;
	
	// 获取时间子系统并通知输入状态
	if (UTimeDilationSubsystem* TimeSubsystem = ChronosSubsystems::GetTimeDilationSubsystem(GetWorld()))
	{
		TimeSubsystem->SetInputActive(bCurrentInputActivity);
	}
	
	// 重置输入活动状态（在下一帧重新检测）
	bHasInputActivity = false;
	bIsFiring = false;
	bIsReloading = false;
}

