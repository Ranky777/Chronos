#include "AIController.h"
#include "Characters/ChronosEnemy.h"

#include "Components/CombatComponent.h"
#include "EngineUtils.h"
#include "GameFlow/ChronosGameMode.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Weapons/ChronosWeapon.h"

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

	FActorSpawnParameters Params;
	Params.Owner = this;
	AChronosWeapon* Weapon = World->SpawnActor<AChronosWeapon>(AChronosWeapon::StaticClass(), GetActorTransform(), Params);
	if (!Weapon)
	{
		return;
	}

	Weapon->InitFromData(EnemyWeaponData);
	// Task 1 修复后 EquipWeapon 内部会调用 EquipTo 完成挂接与状态切换
	CombatComponent->EquipWeapon(Weapon);
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
	const AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController)
	{
		const UAIPerceptionComponent* Perception = AIController->GetAIPerceptionComponent();
		if (Perception)
		{
			TArray<AActor*> PerceivedActors;
			Perception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
			if (!PerceivedActors.IsEmpty())
			{
				return PerceivedActors[0];
			}
		}
	}

	// 感知未命中时回退到玩家 Pawn：单人对抗玩法里敌人永远与玩家交战，
	// 是否真的开火仍由视线判定（视锥 + 射线检测）把关，不会隔墙射击
	if (const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		return PlayerController->GetPawn();
	}

	return nullptr;
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
