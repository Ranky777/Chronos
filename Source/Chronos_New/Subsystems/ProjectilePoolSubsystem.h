#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProjectilePoolSubsystem.generated.h"

class AChronosProjectile;

/** 单一投射物类的空闲对象队列 */
USTRUCT()
struct FChronosProjectilePool
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TArray<TObjectPtr<AChronosProjectile>> InactiveProjectiles;
};

/** 活跃子弹的实时时间戳记录，用于超时回收 */
USTRUCT()
struct FChronosActiveProjectile
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TWeakObjectPtr<AChronosProjectile> Projectile = nullptr;

	/** 发射时刻（真实秒），慢动作下游戏时间几乎不流动，必须用真实时间判定超时 */
	double ActivatedRealTime = 0.0;
};

/**
 * 子弹对象池子系统。
 *
 * SUPERHOT 玩法中子弹在时间冻结时会长时间悬停在场，
 * 频繁 Spawn/Destroy 会造成卡顿与 GC 压力，因此所有子弹统一走池化生命周期。
 */
UCLASS()
class CHRONOS_NEW_API UChronosProjectilePoolSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	/** 从池中取出一颗子弹并立即发射。返回 nullptr 表示类配置非法 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Pool")
	AChronosProjectile* FireProjectile(TSubclassOf<AChronosProjectile> ProjectileClass, AActor* Instigator,
		const FVector& Location, const FRotator& Rotation);

	/** 子弹命中或超时后由投射物回调，放回空闲队列 */
	void ReturnProjectile(AChronosProjectile* Projectile);

	/** 预热：提前为某个投射物类生成 Count 个实例 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Pool")
	void PreallocateProjectiles(TSubclassOf<AChronosProjectile> ProjectileClass, int32 Count);

	/** 每颗活跃子弹的最长存活时间（真实秒）。冻结的悬停子弹最终也会被回收 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Chronos|Pool")
	float MaxRealLifetime = 30.f;

private:
	AChronosProjectile* GetOrCreateProjectile(TSubclassOf<AChronosProjectile> ProjectileClass);

	UPROPERTY(Transient)
	TMap<TSubclassOf<AChronosProjectile>, FChronosProjectilePool> Pools;

	UPROPERTY(Transient)
	TArray<FChronosActiveProjectile> ActiveProjectiles;
};
