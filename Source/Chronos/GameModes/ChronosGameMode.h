// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ChronosGameMode.generated.h"

UENUM(BlueprintType)
enum class EGameState : uint8
{
	/** 游戏未开始 */
	None,
	/** 游戏进行中 */
	Playing,
	/** 玩家胜利 */
	Victory,
	/** 玩家失败 */
	GameOver
};

/** 游戏状态变化事件 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameStateChanged, EGameState, NewState);
/** 敌人数量变化事件 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnemyCountChanged, int32, Remaining, int32, Total);
/** 通关事件 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLevelComplete);
/** 游戏结束事件 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameOver);

/**
 * 
 */
UCLASS()
class CHRONOS_API AChronosGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	AChronosGameMode();
	
protected:
	virtual void BeginPlay() override;
	
public:
	/**
	 * 获取当前游戏状态
	 * @return 当前游戏状态
	 */
	UFUNCTION(BlueprintCallable, Category = "Game Mode")
	EGameState GetCurrentGameState() const { return CurrentGameState; }
	
	/**
	 * 获取剩余敌人数目
	 * @return 存活的敌人数量
	 */
	UFUNCTION(BlueprintCallable, Category = "Game Mode")
	int32 GetRemainingEnemies() const { return RemainingEnemies; }
	
	/**
	* 获取总敌人数目
	* @return 关卡中的总敌人数量
	*/
	UFUNCTION(BlueprintCallable, Category = "Game Mode")
	int32 GetTotalEnemies() const { return TotalEnemies; }
	
	/**
	 * 重新开始当前关卡
	 */
	UFUNCTION(BlueprintCallable, Category = "Game Mode")
	void RestartLevel();

	/**
	* 处理玩家死亡（与HealthComponent::OnDeath委托签名匹配）
	* @param Killer 杀死玩家的Actor
	*/
	UFUNCTION(BlueprintCallable, Category = "Game Mode")
	void HandlePlayerDeath(AActor* Killer);

	/**
	 * 处理敌人死亡（与HealthComponent::OnDeath委托签名匹配）
	 * @param Killer 杀死敌人的Actor
	 */
	UFUNCTION(BlueprintCallable, Category = "Game Mode")
	void HandleEnemyDeath(AActor* Killer);
	
protected:
	/**
	* 设置游戏状态
	* @param NewState 新的游戏状态
	*/
	void SetGameState(EGameState NewState);

	/**
	 * 统计关卡中的敌人数量
	 */
	void CountEnemies();

	/**
	 * 绑定死亡事件
	 */
	void BindDeathEvents();

	/**
	 * 检查通关条件
	 * 当所有敌人死亡时触发通关
	 */
	void CheckWinCondition();
	
private:
	/** 当前游戏状态 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game State", meta = (AllowPrivateAccess = "true"))
	EGameState CurrentGameState = EGameState::None;

	/** 剩余敌人数量 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game State", meta = (AllowPrivateAccess = "true"))
	int32 RemainingEnemies = 0;

	/** 总敌人数量 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game State", meta = (AllowPrivateAccess = "true"))
	int32 TotalEnemies = 0;
	
	/** 玩家重生延迟（秒） */
	UPROPERTY(EditAnywhere, Category = "Game Mode", meta = (ClampMin = 0.0f, ClampMax = 10.0f))
	float RespawnDelay = 2.0f;
	
	FTimerHandle RespawnTimerHandle;
	
	UPROPERTY(BlueprintAssignable, Category = "Game Mode Events")
	FOnGameStateChanged OnGameStateChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Game Mode Events")
	FOnEnemyCountChanged OnEnemyCountChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Game Mode Events")
	FOnLevelComplete OnLevelComplete;
	
	UPROPERTY(BlueprintAssignable, Category = "Game Mode Events")
	FOnGameOver OnGameOver;
};
