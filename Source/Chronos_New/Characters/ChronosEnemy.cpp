#include "Characters/ChronosEnemy.h"

#include "AIController.h"
#include "TimerManager.h"

#include "Components/CombatComponent.h"
#include "EngineUtils.h"
#include "GameFlow/ChronosGameMode.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Weapons/ChronosWeapon.h"
#include "Weapons/WeaponDataAsset.h"

AChronosEnemy::AChronosEnemy()
{
	// 基类 Tick 只做玩家准星焦点扫描，敌人无需
	PrimaryActorTick.bCanEverTick = false;
}

void AChronosEnemy::BeginPlay()
{
	Super::BeginPlay();

	if (EnemyWeaponData)
	{
		SpawnDefaultWeapon();
	}

	// 正常运行走 GetAuthGameMode；迭代器兜底兼容自动化测试等
	// "GameMode 是普通 SpawnActor 而非世界注册 GameMode" 的场景
	AChronosGameMode* GM = GetWorld()->GetAuthGameMode<AChronosGameMode>();
	if (!GM)
	{
		for (TActorIterator<AChronosGameMode> It(GetWorld()); It; ++It)
		{
			GM = *It;
			break;
		}
	}
	if (GM)
	{
		GM->RegisterEnemy(this);
	}
}

void AChronosEnemy::SpawnDefaultWeapon_Implementation()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 必须生成蓝图武器类：它的 FP/TP 视觉网格负责装备态的表现，
	// 裸 C++ 类只有物理网格（装备态会被隐藏），敌人看起来会像空手。
	UClass* WeaponClass = DefaultWeaponClass.LoadSynchronous();
	if (!WeaponClass)
	{
		// 软引用可能因配置丢失（ini 未生效 / 蓝图 CDO 值被覆盖）而为空。
		// 这里硬回退到模板手枪，确保敌人永远不会退化成裸 AChronosWeapon ——
		// 那正是"敌人掉落的枪拾取后不切换 ABP"的根因。
		WeaponClass = LoadClass<AChronosWeapon>(nullptr,
			TEXT("/Game/Variant_Shooter/Blueprints/Pickups/Weapons/BP_ShooterWeapon_Pistol.BP_ShooterWeapon_Pistol_C"));
	}
	if (!WeaponClass)
	{
		WeaponClass = AChronosWeapon::StaticClass();
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	AChronosWeapon* Weapon = World->SpawnActor<AChronosWeapon>(WeaponClass, GetActorTransform(), Params);
	if (!Weapon)
	{
		return;
	}

	// 武器蓝图自带的数据优先 —— "选了步枪类就该是步枪"。
	// EnemyWeaponData 只在武器蓝图没配数据时兜底。
	// 否则会出现"类换成了 Rifle、数据还是手枪"的怪现象：关卡里复制一个敌人、
	// 只改 DefaultWeaponClass，结果拿的枪和数据对不上。
	UWeaponDataAsset* DataToUse = Weapon->WeaponData ? Weapon->WeaponData : EnemyWeaponData;
	Weapon->InitFromData(DataToUse);

	// 难度差异（弹药）在 AI 侧覆写，武器数据本身与玩家共用同一份
	if (StartingAmmoOverride > 0)
	{
		Weapon->SetRemainingAmmo(StartingAmmoOverride);
	}

	// Task 1 修复后 EquipWeapon 内部会调用 EquipTo 完成挂接与状态切换
	CombatComponent->EquipWeapon(Weapon);
}

void AChronosEnemy::StartShooting()
{
	bIsShooting = true;
	FireOneShot();
}

void AChronosEnemy::StopShooting()
{
	bIsShooting = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefireTimerHandle);
	}
}

void AChronosEnemy::FireOneShot()
{
	UWorld* World = GetWorld();
	if (!World || !bIsShooting || !CombatComponent)
	{
		return;
	}

	// 视线丢失就停火，避免隔墙扫射
	if (!HasLineOfSightToCombatTarget())
	{
		StopShooting();
		return;
	}

	AChronosWeapon* Weapon = Cast<AChronosWeapon>(CombatComponent->GetCurrentWeapon().GetObject());
	const AActor* Target = CombatTarget.Get();
	if (!Weapon || !Target)
	{
		StopShooting();
		return;
	}

	Weapon->FireAtTarget(Target->GetActorLocation());

	// 敌人节奏由 AI 侧的 FireInterval 决定（与玩家共用武器数据，难度不在数据里）。
	// 弹药耗尽时 FireAtTarget 内部会广播，战斗组件随即自动投掷空枪，这里无需额外处理
	const float Delay = FMath::Max(0.05f, FireInterval);
	World->GetTimerManager().SetTimer(RefireTimerHandle, this, &AChronosEnemy::FireOneShot, Delay, false);
}

FVector AChronosEnemy::GetAimDirection() const
{
	if (const AActor* Target = CombatTarget.Get())
	{
		FVector Direction = Target->GetActorLocation() - GetActorLocation();
		if (!Direction.IsNearlyZero())
		{
			return Direction.GetSafeNormal();
		}
	}

	return Super::GetAimDirection();
}

AActor* AChronosEnemy::GetPerceivedTarget() const
{
	// 单人对抗：敌人只与玩家交战。
	// ⚠️ 不能直接取 PerceivedActors[0] —— 模板的 AI 感知按角色基类配置、不区分敌我，
	// 场上放第二个敌人后，感知列表里就会混进同伴，于是敌人把队友当成目标：
	// 表现为"敌人互相开火却打玩家时不开枪"。这里只认玩家。
	APawn* PlayerPawn = nullptr;
	if (const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		PlayerPawn = PlayerController->GetPawn();
	}

	if (!PlayerPawn)
	{
		return nullptr;
	}

	const AAIController* AIController = Cast<AAIController>(GetController());
	const UAIPerceptionComponent* Perception =
		AIController ? AIController->GetAIPerceptionComponent() : nullptr;

	// 没有感知组件时无条件锁定玩家（自动化测试就是这种场景）
	if (!Perception)
	{
		return PlayerPawn;
	}

	// 只有玩家确实被看见才交战，避免隔墙锁定；其余感知物（含同伴）一律忽略
	TArray<AActor*> PerceivedActors;
	Perception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
	return PerceivedActors.Contains(PlayerPawn) ? PlayerPawn : nullptr;
}

bool AChronosEnemy::HasLineOfSightToCombatTarget() const
{
	const AActor* Target = CombatTarget.Get();
	if (!Target)
	{
		return false;
	}

	// 视锥：目标方向须落在自身朝向的半角内（FaceActor 任务会持续转身对准）
	const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	if (ToTarget.IsNearlyZero())
	{
		return true;
	}
	if (FVector::DotProduct(ToTarget.GetSafeNormal(), GetActorForwardVector())
		< FMath::Cos(FMath::DegreesToRadians(LineOfSightConeAngle)))
	{
		return false;
	}

	// 从眼睛到目标碰撞包围盒做若干条垂直射线，任一畅通即有视线（对齐模板条件语义）
	const FBox TargetBounds = Target->GetComponentsBoundingBox(/*bNonColliding=*/false);
	const float HalfExtentZ = TargetBounds.GetExtent().Z;
	const float ZStep = HalfExtentZ * 2.f / FMath::Max(NumberOfVerticalLineOfSightChecks, 1);

	FVector EyeLocation = GetActorLocation();
	FRotator EyeRotation = GetActorRotation();
	GetActorEyesViewPoint(EyeLocation, EyeRotation);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ChronosLineOfSight), /*bTraceComplex=*/false);
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(Target);

	for (int32 Index = 0; Index < NumberOfVerticalLineOfSightChecks; ++Index)
	{
		const FVector AimPoint = TargetBounds.GetCenter() + FVector(0.f, 0.f, HalfExtentZ - Index * ZStep);
		if (!GetWorld()->LineTraceTestByChannel(EyeLocation, AimPoint, ECC_Visibility, Params))
		{
			return true;
		}
	}

	return false;
}
