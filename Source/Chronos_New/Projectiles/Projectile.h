#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class USphereComponent;
class UProjectileMovementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectileDeactivated, AChronosProjectile*, Projectile);

/**
 * 池化子弹投射物。
 *
 * 复用项目已有的 "Projectile" 碰撞通道（ECC_GameTraceChannel1），
 * 生命周期由对象池管理（激活/回收而非生成/销毁），伤害走一击必杀的 UHealthComponent。
 * 子弹为直线飞行（无重力），符合 SUPERHOT 中"时间冻结时悬停的子弹"的视觉预期。
 */
UCLASS()
class CHRONOS_NEW_API AChronosProjectile : public AActor
{
	GENERATED_BODY()

public:
	AChronosProjectile();

	/** 从池中取出后调用：摆放位姿、绑定发射者并进入飞行状态 */
	void ActivateProjectile(AActor* NewInstigator, const FVector& Location, const FRotator& Rotation);

	/** 回收到池中：停用移动与特效、清理计时器 */
	void DeactivateProjectile();

	UFUNCTION(BlueprintPure, Category = "Chronos|Projectile")
	bool IsProjectileActive() const { return bActive; }

	/** 子弹初速。慢动作下子弹的相对运动感完全取决于该速度与时间流速的比例 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Chronos|Projectile")
	float BulletSpeed = 6000.f;

	/** 命中时对可击碎物理体施加的冲量强度 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Chronos|Projectile")
	float DamageImpulseStrength = 50000.f;

	/** 飞行尾迹特效，可为空 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chronos|Projectile")
	TObjectPtr<UNiagaraSystem> TrailSystem;

	UPROPERTY(BlueprintAssignable, Category = "Chronos|Projectile")
	FOnProjectileDeactivated OnProjectileDeactivated;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	/** ProjectileMovement 停止时触发（速度归零/被阻挡），与组件 Hit 事件共用处理逻辑 */
	UFUNCTION()
	void OnProjectileStop(const FHitResult& Impact);

private:
	void ReturnToPool();
	void HandleImpact(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UNiagaraComponent> TrailComponent;

	uint32 bActive : 1;
};
