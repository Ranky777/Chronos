#include "Weapons/ChronosWeapon.h"

#include "Characters/ChronosCharacter.h"
#include "Components/CombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/HealthComponent.h"
#include "Feedback/ChronosFeedbackSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Projectiles/Projectile.h"
#include "Subsystems/ProjectilePoolSubsystem.h"
#include "Weapons/WeaponDataAsset.h"

AChronosWeapon::AChronosWeapon()
{
	PrimaryActorTick.bCanEverTick = true;

	WeaponMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMeshComponent"));
	WeaponMeshComponent->SetCollisionProfileName(TEXT("NoCollision"));
	WeaponMeshComponent->bReceivesDecals = false;
	// 装备态下本组件会被隐藏（改由蓝图 FP/TP 网格显示），但枪口插槽仍需参与解算
	WeaponMeshComponent->VisibilityBasedAnimTickOption =
		EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	RootComponent = WeaponMeshComponent;
}

void AChronosWeapon::BeginPlay()
{
	Super::BeginPlay();

	// 撞击记录用于取消投掷者的自伤免疫
	WeaponMeshComponent->OnComponentHit.AddDynamic(this, &AChronosWeapon::OnWeaponHit);

	// 直接放置在关卡中的武器实例：开始运行时应用数据资产配置。
	// 以"未赋值弹药 + 无网格"作为未初始化标记，避免对已初始化的武器重复执行
	if (WeaponData && RemainingAmmo <= 0 && !WeaponMeshComponent->GetSkeletalMeshAsset())
	{
		InitFromData(WeaponData);
	}
}

void AChronosWeapon::InitFromData(UWeaponDataAsset* Data)
{
	WeaponData = Data;

	if (!Data)
	{
		return;
	}

	if (Data->WeaponMesh)
	{
		WeaponMeshComponent->SetSkeletalMesh(Data->WeaponMesh);
	}

	RemainingAmmo = Data->AmmoCount;

	// 初始处于世界状态（作为地面拾取物生成）
	EnterWorldState(FVector::ZeroVector);
}

//~ 装备/脱手表现：C++ 默认实现，蓝图可覆写
void AChronosWeapon::OnEquipVisuals_Implementation()
{
	AChronosCharacter* Holder = Cast<AChronosCharacter>(GetOwner());
	if (!Holder)
	{
		return;
	}

	// 动画实例来自数据资产，因此不依赖蓝图也能正确切换持枪姿态。
	// 这是"敌人生成了裸 AChronosWeapon"时表现仍然正常的最后一道保险。
	TSubclassOf<UAnimInstance> FirstPersonAnimClass = nullptr;
	TSubclassOf<UAnimInstance> ThirdPersonAnimClass = nullptr;
	UAnimMontage* EquipMontage = nullptr;

	if (WeaponData)
	{
		FirstPersonAnimClass = WeaponData->FirstPersonAnimClass;
		ThirdPersonAnimClass = WeaponData->ThirdPersonAnimClass;
		EquipMontage = WeaponData->EquipMontage;
	}

	Holder->SetWeaponAnimClasses(FirstPersonAnimClass, ThirdPersonAnimClass);

	if (EquipMontage)
	{
		Holder->PlayWeaponMontage(EquipMontage);
	}
}

void AChronosWeapon::OnUnequipVisuals_Implementation(AChronosCharacter* PreviousHolder)
{
	if (PreviousHolder)
	{
		PreviousHolder->RestoreDefaultAnimClasses();
	}
}

void AChronosWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 全自动连发：按住开火键时按武器射速持续开火。
	// CanFire 内含射速冷却判定，所以每帧调用是安全的（真正的节流在那里）。
	// 弹药耗尽时 CanFire 为假，随后战斗组件会自动把空枪投出去。
	if (bIsFiring && WeaponData && WeaponData->bAutomatic)
	{
		Fire_Implementation();
	}

	UpdateRestState();
}

void AChronosWeapon::UpdateRestState()
{
	if (WeaponState != EWeaponState::InWorld)
	{
		return;
	}

	const FVector Velocity = WeaponMeshComponent->GetPhysicsLinearVelocity();

	// 只看线速度不够：武器会"位置停了但仍原地打转"—— 判定为静止却一直在转，
	// 玩家看到的就是落地后还停不下来。
	const FVector AngularVelocity = WeaponMeshComponent->GetPhysicsAngularVelocityInDegrees();
	const bool bSettled = Velocity.SizeSquared() < FMath::Square(10.f) &&
		AngularVelocity.SizeSquared() < FMath::Square(RestAngularSpeedDeg);

	bAtRest = !WeaponMeshComponent->IsSimulatingPhysics() || bSettled;

	// 停稳后让刚体睡眠，彻底锁住残余旋转（被再次碰撞/投掷会自动唤醒）
	if (bAtRest && WeaponMeshComponent->IsSimulatingPhysics())
	{
		WeaponMeshComponent->PutAllRigidBodiesToSleep();
	}
}

bool AChronosWeapon::CanFire_Implementation()
{
	return WeaponState == EWeaponState::Equipped &&
		RemainingAmmo > 0 &&
		(FPlatformTime::Seconds() - LastFireRealTime) >= RefireCooldown;
}

bool AChronosWeapon::FireAtTarget(const FVector& TargetPoint)
{
	if (!CanFire_Implementation() || !WeaponData)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector MuzzleLocation = GetMuzzleLocation_Implementation();
	FVector FireDirection = TargetPoint - MuzzleLocation;
	if (FireDirection.IsNearlyZero())
	{
		FireDirection = GetActorForwardVector();
	}
	else
	{
		FireDirection.Normalize();
	}

	// ProjectileClass 为空的武器（如榴弹发射器）由 BP OnFireVisuals 自行生成投射物
	if (WeaponData->ProjectileClass)
	{
		if (UChronosProjectilePoolSubsystem* Pool = World->GetSubsystem<UChronosProjectilePoolSubsystem>())
		{
			Pool->FireProjectile(WeaponData->ProjectileClass, GetOwner(), MuzzleLocation, FireDirection.Rotation());
		}
	}

	--RemainingAmmo;
	LastFireRealTime = FPlatformTime::Seconds();
	RefireCooldown = 1.f / WeaponData->FireRate;

	OnAmmoChanged.Broadcast(RemainingAmmo, WeaponData->AmmoCount);
	OnFireVisuals();

	return true;
}

FVector AChronosWeapon::GetMuzzleLocation_Implementation()
{
	if (WeaponMeshComponent && WeaponData &&
		WeaponMeshComponent->DoesSocketExist(WeaponData->MuzzleSocketName))
	{
		return WeaponMeshComponent->GetSocketLocation(WeaponData->MuzzleSocketName);
	}

	return GetActorLocation();
}

void AChronosWeapon::EquipTo(AChronosCharacter* NewHolder)
{
	if (!NewHolder)
	{
		return;
	}

	WeaponState = EWeaponState::Equipped;
	LastThrower = nullptr;
	CurrentHolder = NewHolder;
	SetInstigator(Cast<APawn>(NewHolder));
	SetOwner(NewHolder);

	EnterEquippedState();

	AttachToComponent(NewHolder->GetHeldWeaponAttachComponent(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		NewHolder->GetHeldWeaponSocketName());

	OnEquipVisuals();
}

//~ IWeaponUser interface implementations
void AChronosWeapon::Fire_Implementation()
{
	// 记录"按住"状态：全自动武器在 Tick 里据此连发。
	// 半自动下连点也走这里，Tick 不会重复触发（bAutomatic 为假时直接跳过）。
	bIsFiring = true;

	// 接口版本：使用枪口朝向发射
	if (!CanFire_Implementation() || !WeaponData)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector MuzzleLocation = GetMuzzleLocation_Implementation();
	const FVector FireDirection = WeaponMeshComponent->GetSocketRotation(WeaponData->MuzzleSocketName).Vector();

	// ProjectileClass 为空的武器（如榴弹发射器）由 BP OnFireVisuals 自行生成投射物
	if (WeaponData->ProjectileClass)
	{
		if (UChronosProjectilePoolSubsystem* Pool = World->GetSubsystem<UChronosProjectilePoolSubsystem>())
		{
			Pool->FireProjectile(WeaponData->ProjectileClass, GetOwner(), MuzzleLocation, FireDirection.Rotation());
		}
	}

	--RemainingAmmo;
	LastFireRealTime = FPlatformTime::Seconds();
	RefireCooldown = 1.f / WeaponData->FireRate;

	OnAmmoChanged.Broadcast(RemainingAmmo, WeaponData->AmmoCount);
	OnFireVisuals();

	PlayFireSound();

	// 打击感统一出口：音效 / 屏幕震动 / 命中统计都由反馈子系统派发。
	// 传 GetOwner() 而不是 this —— 反馈要按"是谁在开枪"决定玩家全屏音还是敌人 3D 音。
	if (UChronosFeedbackSubsystem* Feedback = World->GetSubsystem<UChronosFeedbackSubsystem>())
	{
		Feedback->NotifyWeaponFired(GetOwner(), MuzzleLocation, WeaponData->FireSfx);
	}
}

void AChronosWeapon::PlayFireSound()
{
	if (!WeaponData || WeaponData->FireSound.IsNull())
	{
		return;
	}

	if (USoundBase* Sound = WeaponData->FireSound.LoadSynchronous())
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
	}
}

void AChronosWeapon::StopFiring_Implementation()
{
	// 松开开火键：停止全自动连发
	bIsFiring = false;
}

int32 AChronosWeapon::GetCurrentAmmo_Implementation()
{
	return RemainingAmmo;
}

int32 AChronosWeapon::GetMaxAmmo_Implementation()
{
	return WeaponData ? WeaponData->AmmoCount : 0;
}

FName AChronosWeapon::GetMuzzleSocketName_Implementation()
{
	return WeaponData ? WeaponData->MuzzleSocketName : NAME_None;
}

void AChronosWeapon::ThrowWeapon_Implementation(const FVector& Direction)
{
	// 注意：不能用 LastThrower —— EquipTo 会把它清空，导致投掷者丢失自伤免疫（自杀 Bug 根因）
	Throw(Direction, CurrentHolder.Get());
}

void AChronosWeapon::DropWeapon_Implementation()
{
	// 同上：丢弃/死亡掉落同样要知道是谁在持有
	Drop(CurrentHolder.Get());
}

void AChronosWeapon::EnterEquippedState()
{
	WeaponState = EWeaponState::Equipped;
	bHasHitWorldSinceThrown = false;

	WeaponMeshComponent->SetCollisionProfileName(TEXT("NoCollision"));
	WeaponMeshComponent->SetSimulatePhysics(false);
	WeaponMeshComponent->SetEnableGravity(false);
	WeaponMeshComponent->SetGenerateOverlapEvents(false);

	SetActorTickEnabled(true);

	// 装备态：物理网格只作枪口/附加参考，隐藏；视觉交给蓝图的 FP/TP 网格。
	// 若这把武器没有任何视觉网格（例如裸 AChronosWeapon），则保留物理网格可见，
	// 否则会出现"角色空手"的退化表现。
	const int32 VisualMeshCount = SetVisualMeshesHidden(false);
	WeaponMeshComponent->SetHiddenInGame(VisualMeshCount > 0);
}

void AChronosWeapon::EnterWorldState(const FVector& InitialVelocity)
{
	WeaponState = EWeaponState::InWorld;
	bAtRest = false;
	bHasHitWorldSinceThrown = false;
	LastWorldEnterRealTime = FPlatformTime::Seconds();

	// 武器脱手（投掷/丢弃/死亡掉落）必须停火，否则换个持有者后可能仍在连发
	bIsFiring = false;

	SetOwner(nullptr);

	// 脱手：视觉网格收回，物理网格重新可见（否则会出现"手上一把 + 世界一把"）
	WeaponMeshComponent->SetHiddenInGame(false);
	SetVisualMeshesHidden(true);
	if (AChronosCharacter* PreviousHolder = CurrentHolder.Get())
	{
		CurrentHolder = nullptr;
		OnUnequipVisuals(PreviousHolder);
	}

	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponMeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
	WeaponMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	WeaponMeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	WeaponMeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	WeaponMeshComponent->SetGenerateOverlapEvents(true);
	WeaponMeshComponent->OnComponentBeginOverlap.AddUniqueDynamic(this, &AChronosWeapon::OnWeaponBeginOverlap);

	WeaponMeshComponent->SetSimulatePhysics(true);
	WeaponMeshComponent->SetEnableGravity(true);

	// 没有这两行，脱手的枪会像球一样滚个不停（网格无 PhysicsAsset，退化成简单形状）
	WeaponMeshComponent->SetLinearDamping(LinearDamping);
	WeaponMeshComponent->SetAngularDamping(AngularDamping);
	// 上一轮可能已把刚体睡下去，重新脱手必须唤醒，否则会僵在原地
	WeaponMeshComponent->WakeAllRigidBodies();
	if (!InitialVelocity.IsZero())
	{
		WeaponMeshComponent->SetPhysicsLinearVelocity(InitialVelocity);
	}
}

void AChronosWeapon::OnWeaponHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// 首次撞击环境后取消投掷者的永久免疫：扔出去撞墙弹回来可以砸中自己
	bHasHitWorldSinceThrown = true;
}

int32 AChronosWeapon::SetVisualMeshesHidden(bool bShouldHide)
{
	TArray<USkeletalMeshComponent*> Meshes;
	GetComponents<USkeletalMeshComponent>(Meshes);

	int32 Count = 0;
	for (USkeletalMeshComponent* Mesh : Meshes)
	{
		if (Mesh && Mesh != WeaponMeshComponent)
		{
			Mesh->SetHiddenInGame(bShouldHide);
			++Count;
		}
	}

	return Count;
}

void AChronosWeapon::OnWeaponBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (WeaponState != EWeaponState::InWorld || !OtherActor || OtherActor == GetOwner())
	{
		return;
	}

	// 投掷者免疫（旧项目 PI-5 教训）。
	// 免疫条件：命中对象就是投掷者，且（尚未撞击过环境 或 仍在出手免疫窗口内）。
	// 之前这里拿到的 Thrower 恒为空（EquipTo 清掉了 LastThrower），
	// 所以右键扔枪 / 打空自动甩枪都会砸死自己。
	AChronosCharacter* Thrower = LastThrower.Get();
	if (Thrower && OtherActor == Cast<AActor>(Thrower))
	{
		const double SinceThrown = FPlatformTime::Seconds() - LastWorldEnterRealTime;
		if (!bHasHitWorldSinceThrown || SinceThrown < ThrowerImmunityDuration)
		{
			return;
		}
	}

	const float Speed = WeaponMeshComponent->GetPhysicsLinearVelocity().Size();
	if (Speed < ThrowKillSpeedThreshold)
	{
		return;
	}

	if (UHealthComponent* TargetHealth = OtherActor->FindComponentByClass<UHealthComponent>())
	{
		const bool bKillingBlow = TargetHealth->IsAlive();
		TargetHealth->TakeDamage(Thrower ? Cast<AActor>(Thrower) : this);

		if (bKillingBlow)
		{
			// 走 NotifyKill 而不是 NotifyHitConfirmed：投掷击杀没消耗子弹，
			// 算进"命中数"会把命中率刷到 100% 以上，结算数据就废了。
			if (UChronosFeedbackSubsystem* Feedback = GetWorld()->GetSubsystem<UChronosFeedbackSubsystem>())
			{
				Feedback->NotifyKill(Thrower ? Cast<AActor>(Thrower) : this, OtherActor->GetActorLocation());
			}
		}
	}
}

void AChronosWeapon::OnInteract(AChronosCharacter* Interactor)
{
	if (!CanInteract(Interactor))
	{
		return;
	}

	Interactor->GetCombatComponent()->EquipWeapon(this);
}

bool AChronosWeapon::CanInteract(const AChronosCharacter* Interactor) const
{
	if (WeaponState != EWeaponState::InWorld || !bAtRest)
	{
		return false;
	}

	// 刚被自己扔出的武器在短延迟内不可立即捡回
	if (LastThrower.IsValid() && LastThrower.Get() == Interactor)
	{
		const double Now = FPlatformTime::Seconds();
		if (Now - LastWorldEnterRealTime < MinPickupDelay)
		{
			return false;
		}
	}

	return true;
}

FText AChronosWeapon::GetInteractionText() const
{
	if (WeaponData)
	{
		// 空枪仍然可拾取：SUPERHOT 里打空的枪是有效的投掷物，
		// 但必须让玩家一眼看出它没子弹了，否则会误以为捡起来能开火
		if (RemainingAmmo <= 0)
		{
			return FText::Format(
				NSLOCTEXT("Chronos", "PickupFormatEmpty", "拾取 {0}（已空 · 可投掷）"),
				WeaponData->WeaponName);
		}

		return FText::Format(
			NSLOCTEXT("Chronos", "PickupFormatAmmo", "拾取 {0}  {1}/{2}"),
			WeaponData->WeaponName,
			FText::AsNumber(RemainingAmmo),
			FText::AsNumber(WeaponData->AmmoCount));
	}

	return NSLOCTEXT("Chronos", "PickupWeapon", "拾取武器");
}

void AChronosWeapon::SetRemainingAmmo(int32 NewAmmo)
{
	RemainingAmmo = FMath::Max(0, NewAmmo);
	OnAmmoChanged.Broadcast(RemainingAmmo, WeaponData ? WeaponData->AmmoCount : 0);
}

void AChronosWeapon::Throw(const FVector& Direction, AChronosCharacter* Thrower)
{
	const float ThrowSpeed = WeaponData ? WeaponData->ThrowSpeed : 1500.f;
	LastThrower = Thrower;

	if (Thrower)
	{
		SetInstigator(Cast<APawn>(Thrower));
		SetOwner(Thrower);
	}

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	EnterWorldState(Direction * ThrowSpeed);
}

void AChronosWeapon::Drop(AChronosCharacter* Dropper)
{
	LastThrower = Dropper;
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	EnterWorldState(FVector::ZeroVector);
}