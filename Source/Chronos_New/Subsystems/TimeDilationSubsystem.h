#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimeDilationSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeDilationChanged, float, NewDilation);

/**
 * SUPERHOT 式全局时间控制子系统。
 *
 * 相比"每帧输入标志位"方案的核心改进：
 * 1. 用真实时间戳 (FPlatformTime::Seconds) 记录玩家最近一次输入时刻，
 *    输入活动窗口内的任意时刻（含鼠标微动）都会立即推进时间，无帧粒度误差；
 * 2. 插值使用真实时间差而非被膨胀后的游戏帧时长，避免"越慢越难变快"的正反馈失真；
 * 3. 支持击杀瞬间子弹时间 Override 与玩家死亡强制恢复正常流速。
 */
UCLASS()
class CHRONOS_NEW_API UTimeDilationSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	//~ UTickableWorldSubsystem
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	/** 玩家产生任何输入（移动/视角/开火/投掷等）时调用，立即推进时间 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Time")
	void NotifyPlayerInput();

	/** 玩家死亡后时间恢复正常流速；复活时传 false 解除 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Time")
	void SetPlayerDead(bool bDead);

	/** 击杀瞬间的子弹时间：在 Duration 秒真实时间内将时间压到 Dilation，结束后平滑恢复 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Time")
	void TriggerKillOverride(float Dilation = 0.05f, float Duration = 0.3f);

	UFUNCTION(BlueprintPure, Category = "Chronos|Time")
	float GetCurrentDilation() const { return CurrentDilation; }

	/** 当前是否处于明显减速状态（供 HUD / AI / 特效判断） */
	UFUNCTION(BlueprintPure, Category = "Chronos|Time")
	bool IsTimeSlowed() const { return CurrentDilation <= SlowDilation * 2.f; }

	UPROPERTY(BlueprintAssignable, Category = "Chronos|Time")
	FOnTimeDilationChanged OnDilationChanged;

	/** 无输入时的时间流速 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Chronos|Time", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float SlowDilation = 0.05f;

	/** 从满速减速到目标值的耗时（真实秒）。SUPERHOT 手感要求减速非常干脆 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Chronos|Time", meta = (ClampMin = "0.01"))
	float TimeToSlow = 0.1f;

	/** 从慢速恢复到满速的耗时（真实秒） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Chronos|Time", meta = (ClampMin = "0.01"))
	float TimeToSpeedUp = 0.25f;

	/** 输入活动窗口：距最近一次输入小于该真实秒数时视为"输入活跃"，时间维持满速 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Chronos|Time", meta = (ClampMin = "0.0"))
	float InputActiveWindow = 0.1f;

	UTimeDilationSubsystem();

private:
	void ApplyDilation(float NewDilation);

	double LastInputRealTime = 0.0;
	double LastRealTime = 0.0;
	bool bInitialized = false;

	float CurrentDilation = 1.f;

	float KillOverrideDilation = 0.05f;
	float KillOverrideRemaining = 0.f;

	uint32 bPlayerDead : 1;
};
