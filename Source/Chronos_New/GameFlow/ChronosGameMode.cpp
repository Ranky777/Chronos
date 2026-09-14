#include "GameFlow/ChronosGameMode.h"

#include "Characters/ChronosCharacter.h"
#include "Characters/ChronosEnemy.h"
#include "GameFlow/ChronosPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystems/TimeDilationSubsystem.h"

AChronosGameMode::AChronosGameMode()
{
	PrimaryActorTick.bCanEverTick = false;

	// 必须显式指定：AGameModeBase 默认用引擎的 APlayerController，
	// 那样 AChronosPlayerController 上的 HUD、死亡/清关事件、R 键重开全都不会生效。
	// DefaultPawnClass 不在这里写死 —— 它由关卡的 WorldSettings 指定（BP_ShooterCharacter），
	// 写死会让换关卡/换角色变得很麻烦。
	PlayerControllerClass = AChronosPlayerController::StaticClass();
}

void AChronosGameMode::BeginPlay()
{
	Super::BeginPlay();

	// OpenLevel 之后 GameMode 会重新实例化，用世界名在列表中恢复进度
	CurrentLevelIndex = FindLevelIndexForCurrentWorld();

	LevelStartTime = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.f;
}

float AChronosGameMode::GetLevelElapsedSeconds() const
{
	if (!GetWorld())
	{
		return 0.f;
	}

	return FMath::Max(0.f, GetWorld()->GetRealTimeSeconds() - LevelStartTime);
}

FString AChronosGameMode::GetCurrentLevelName() const
{
	if (LevelList.IsValidIndex(CurrentLevelIndex))
	{
		return LevelList[CurrentLevelIndex].GetAssetName();
	}

	// 不在列表里（单关模式 / 菜单场景）：退回世界名，至少保证存档键非空且稳定
	return GetWorld() ? GetWorld()->GetName() : FString();
}

void AChronosGameMode::RegisterEnemy(AChronosEnemy* Enemy)
{
	if (!Enemy || ActiveEnemies.Contains(Enemy))
	{
		return;
	}

	Enemy->OnCharacterDeath.AddDynamic(this, &AChronosGameMode::HandleEnemyDeath);
	ActiveEnemies.Add(Enemy);
	BroadcastCounts();
}

void AChronosGameMode::HandleEnemyDeath(AChronosCharacter* Character)
{
	AChronosEnemy* Enemy = Cast<AChronosEnemy>(Character);
	if (!Enemy)
	{
		return;
	}

	ActiveEnemies.Remove(Enemy);
	++KillCount;

	// 击杀瞬间子弹时间（子系统默认 0.05/0.3s）
	if (UTimeDilationSubsystem* Time = GetWorld()->GetSubsystem<UTimeDilationSubsystem>())
	{
		Time->TriggerKillOverride();
	}

	BroadcastCounts();

	if (ActiveEnemies.Num() == 0 && !bLevelCleared)
	{
		bLevelCleared = true;
		OnLevelCleared.Broadcast(IsFinalLevel());
	}
}

void AChronosGameMode::RestartLevel()
{
	UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()));
}

int32 AChronosGameMode::FindLevelIndexForCurrentWorld() const
{
	const FString WorldPath = GetWorld() ? GetWorld()->GetName() : FString();
	if (WorldPath.IsEmpty())
	{
		return INDEX_NONE;
	}

	// ⚠️ PIE 下世界名带 UEDPIE_0_ 前缀（如 UEDPIE_0_Lvl_Arena02），
	// 精确比较会失配，索引停在 INDEX_NONE，于是 LoadNextLevel 用 -1+1=0 又打开第一关 ——
	// 表现为"第二关打完不进第三关"。这里用包含匹配兼容 PIE 前缀。
	const FString WorldName = FPackageName::GetShortName(WorldPath);

	for (int32 Index = 0; Index < LevelList.Num(); ++Index)
	{
		const FString AssetName = LevelList[Index].GetAssetName();
		if (AssetName.IsEmpty())
		{
			continue;
		}

		if (AssetName == WorldName || WorldPath.Contains(AssetName))
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

void AChronosGameMode::LoadNextLevel()
{
	if (LevelList.IsValidIndex(CurrentLevelIndex + 1))
	{
		UGameplayStatics::OpenLevel(this, FName(*LevelList[CurrentLevelIndex + 1].GetLongPackageName()));
	}
}

void AChronosGameMode::BroadcastCounts()
{
	OnEnemyCountChanged.Broadcast(ActiveEnemies.Num());
}
