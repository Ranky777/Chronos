#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ChronosGameMode.generated.h"

class AChronosCharacter;
class AChronosEnemy;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChronosEnemyCountChanged, int32, RemainingEnemies);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChronosLevelCleared, bool, bIsFinalLevel);

/**
 * SUPERHOT 多小关卡推进：敌人自行注册、死亡注销，计数归零即清关；
 * 关卡列表由 BP 子类配置，OpenLevel 切换后 GameMode 重新实例化时按世界名恢复进度。
 */
UCLASS(Blueprintable)
class CHRONOS_NEW_API AChronosGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AChronosGameMode();

	virtual void BeginPlay() override;

	/** 顺序关卡列表（软引用，在 BP 子类中配置）；为空表示单关模式 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chronos|Flow")
	TArray<TSoftObjectPtr<UWorld>> LevelList;

	UFUNCTION(BlueprintPure, Category = "Chronos|Flow")
	int32 GetRemainingEnemies() const { return ActiveEnemies.Num(); }

	/** 关卡列表为空（单关模式）视为末关；否则按当前索引判断（测试断言"单关模式视为末关"） */
	UFUNCTION(BlueprintPure, Category = "Chronos|Flow")
	bool IsFinalLevel() const { return LevelList.Num() == 0 || CurrentLevelIndex == LevelList.Num() - 1; }

	/**
	 * 当前世界是否是关卡列表中的正式关卡。
	 * 不在列表里的（如主菜单场景）需要显示菜单而不是 HUD —— PlayerController 据此判断。
	 */
	UFUNCTION(BlueprintPure, Category = "Chronos|Flow")
	bool IsConfiguredLevel() const { return FindLevelIndexForCurrentWorld() != INDEX_NONE; }

	/** 本关已用时（秒）。用 RealTimeSeconds，不受慢动作时间缩放影响 */
	UFUNCTION(BlueprintPure, Category = "Chronos|Flow")
	float GetLevelElapsedSeconds() const;

	/** 本关击杀数（通关结算显示用） */
	UFUNCTION(BlueprintPure, Category = "Chronos|Flow")
	int32 GetKillCount() const { return KillCount; }

	/**
	 * 当前关卡名（存档记录的键）。
	 * 取 LevelList 里的资产名而不是世界名 —— PIE 下世界名带 UEDPIE_0_ 前缀，
	 * 直接拿它当键会导致"编辑器里打的成绩"和"打包后打的成绩"对不上。
	 */
	UFUNCTION(BlueprintPure, Category = "Chronos|Flow")
	FString GetCurrentLevelName() const;

	/** 当前关卡在 LevelList 中的索引；不在列表里（菜单场景）时为 INDEX_NONE */
	UFUNCTION(BlueprintPure, Category = "Chronos|Flow")
	int32 GetCurrentLevelIndex() const { return CurrentLevelIndex; }

	UFUNCTION(BlueprintCallable, Category = "Chronos|Flow")
	void RestartLevel();

	/** 过场 UI 播放完毕后由 PlayerController 调用，加载列表中的下一关 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Flow")
	void LoadNextLevel();

	UPROPERTY(BlueprintAssignable, Category = "Chronos|Flow")
	FOnChronosEnemyCountChanged OnEnemyCountChanged;

	UPROPERTY(BlueprintAssignable, Category = "Chronos|Flow")
	FOnChronosLevelCleared OnLevelCleared;

	/** 敌人 BeginPlay 时自行注册并订阅死亡 */
	void RegisterEnemy(AChronosEnemy* Enemy);

private:
	/** 按世界名在 LevelList 中查找索引；兼容 PIE 的 UEDPIE_0_ 前缀 */
	int32 FindLevelIndexForCurrentWorld() const;

	UFUNCTION()
	void HandleEnemyDeath(AChronosCharacter* Character);

	void BroadcastCounts();

	UPROPERTY()
	TArray<TObjectPtr<AChronosEnemy>> ActiveEnemies;

	int32 CurrentLevelIndex = INDEX_NONE;
	bool bLevelCleared = false;

	/** 关卡开始时刻（RealTimeSeconds 基准），用于结算显示用时 */
	float LevelStartTime = 0.f;

	int32 KillCount = 0;
};
