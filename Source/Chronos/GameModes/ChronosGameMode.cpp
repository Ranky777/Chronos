// Fill out your copyright notice in the Description page of Project Settings.


#include "ChronosGameMode.h"

#include "Chronos/Characters/EnemyCharacter.h"
#include "Chronos/Characters/PlayerCharacter.h"
#include "Chronos/Components/HealthComponent.h"
#include "Kismet/GameplayStatics.h"

AChronosGameMode::AChronosGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
	
	DefaultPawnClass = APlayerCharacter::StaticClass();
}

void AChronosGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	// 统计关卡中的敌人数量
	CountEnemies();

	// 绑定死亡事件
	BindDeathEvents();

	// 设置游戏状态为进行中
	SetGameState(EGameState::Playing);
}

void AChronosGameMode::RestartLevel()
{
	UGameplayStatics::OpenLevel(GetWorld(), FName(*GetWorld()->GetName()));
}

void AChronosGameMode::HandlePlayerDeath(AActor* Killer)
{
	// 获取玩家角色
	APlayerCharacter* PlayerCharacter = nullptr;
	TArray<AActor*> PlayerActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerCharacter::StaticClass(), PlayerActors);
    
	if (PlayerActors.Num() > 0)
	{
		PlayerCharacter = Cast<APlayerCharacter>(PlayerActors[0]);
	}
	
	SetGameState(EGameState::GameOver);
	
	// 延迟重生玩家
	if (PlayerCharacter)
	{
		GetWorldTimerManager().SetTimer(
			RespawnTimerHandle, 
			[this, PlayerCharacter]()
			{
				PlayerCharacter->RespawnPlayer();
				SetGameState(EGameState::Playing);
			},
			RespawnDelay,
			false
		);
	}
}

void AChronosGameMode::HandleEnemyDeath(AActor* Killer)
{
	if (RemainingEnemies > 0)
	{
		RemainingEnemies--;
		
		// 触发敌人数量变化事件
		OnEnemyCountChanged.Broadcast(RemainingEnemies, TotalEnemies);

		// 检查通关条件
		CheckWinCondition();
	}
}

void AChronosGameMode::SetGameState(EGameState NewState)
{
	// 如果状态没有变化，不做处理
	if (CurrentGameState == NewState)
	{
		return;
	}

	// 更新状态
	CurrentGameState = NewState;

	// 触发状态变化事件
	OnGameStateChanged.Broadcast(NewState);
	
	// 根据新状态执行相应逻辑
	switch (NewState)
	{
	case EGameState::Victory:
		// 触发通关事件
		OnLevelComplete.Broadcast();
		break;

	case EGameState::GameOver:
		// 触发游戏结束事件
		OnGameOver.Broadcast();
		break;

	default:
		break;
	}
}

void AChronosGameMode::CountEnemies()
{
	TArray<AActor*> EnemyActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyCharacter::StaticClass(), EnemyActors);
	
	TotalEnemies = EnemyActors.Num();
	RemainingEnemies = TotalEnemies;
	
	// 触发敌人数量变化事件
	OnEnemyCountChanged.Broadcast(RemainingEnemies, TotalEnemies);
}

void AChronosGameMode::BindDeathEvents()
{
	APlayerCharacter* PlayerCharacter = nullptr;
	// 获取玩家角色
	TArray<AActor*> PlayerActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerCharacter::StaticClass(), PlayerActors);
	
	if (PlayerActors.Num() > 0)
	{
		PlayerCharacter = Cast<APlayerCharacter>(PlayerActors[0]);
        
		if (PlayerCharacter && PlayerCharacter->GetHealthComponent())
		{
			// 绑定玩家的死亡事件（签名匹配：void(AActor*)）
			PlayerCharacter->GetHealthComponent()->OnDeath.AddDynamic(this, &AChronosGameMode::HandlePlayerDeath);
		}
	}
	
	
	TArray<AActor*> EnemyActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyCharacter::StaticClass(), EnemyActors);
	
	for (AActor* Actor : EnemyActors)
	{
		if (AEnemyCharacter* EnemyCharacter = Cast<AEnemyCharacter>(Actor))
		{
			if (UHealthComponent* HealthComponent = EnemyCharacter->GetHealthComponent())
			{
				HealthComponent->OnDeath.AddDynamic(this, &AChronosGameMode::HandleEnemyDeath);
			}
		}
	}
}

void AChronosGameMode::CheckWinCondition()
{
	if (RemainingEnemies <= 0)
	{
		SetGameState(EGameState::Victory);
	}
}
