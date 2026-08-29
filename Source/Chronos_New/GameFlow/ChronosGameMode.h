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
	UFUNCTION()
	void HandleEnemyDeath(AChronosCharacter* Character);

	void BroadcastCounts();

	UPROPERTY()
	TArray<TObjectPtr<AChronosEnemy>> ActiveEnemies;

	int32 CurrentLevelIndex = INDEX_NONE;
	bool bLevelCleared = false;
};
