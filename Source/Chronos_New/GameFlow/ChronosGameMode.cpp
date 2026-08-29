#include "GameFlow/ChronosGameMode.h"

#include "Characters/ChronosCharacter.h"
#include "Characters/ChronosEnemy.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystems/TimeDilationSubsystem.h"

AChronosGameMode::AChronosGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AChronosGameMode::BeginPlay()
{
	Super::BeginPlay();

	// OpenLevel 之后 GameMode 会重新实例化，用世界名在列表中恢复进度
	const FString WorldName = FPackageName::GetShortName(GetWorld()->GetName());
	for (int32 Index = 0; Index < LevelList.Num(); ++Index)
	{
		if (LevelList[Index].GetAssetName() == WorldName)
		{
			CurrentLevelIndex = Index;
			break;
		}
	}
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
