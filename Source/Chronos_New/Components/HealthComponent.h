#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FChronosOnDamageTaken, AActor*, DamagedActor, AActor*, InstigatorActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FChronosOnDeath, AActor*, DamagedActor, AActor*, Killer);

/**
 * SUPERHOT 式"一击必杀"生命组件。
 *
 * 刻意不持有 HP 数值：受到任何一次有效伤害即死亡。
 * 生存状态由 GameplayTag（State.Alive / State.Dead）承载，
 * 对外只暴露委托，死亡后的具体表现（布娃娃、掉落武器、结算等）由拥有者自行监听处理。
 */
UCLASS(ClassGroup = (Chronos), meta = (BlueprintSpawnableComponent))
class CHRONOS_NEW_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	/** 受到一次致命伤害。重复调用在死亡后会被忽略 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Health")
	void TakeDamage(AActor* InstigatorActor);

	UFUNCTION(BlueprintPure, Category = "Chronos|Health")
	bool IsAlive() const;

	/** 复活：重置状态标签并广播 OnRevived。是否可复活由调用方（GameMode/关卡逻辑）决定 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Health")
	void Revive();

	UPROPERTY(BlueprintAssignable, Category = "Chronos|Health")
	FChronosOnDamageTaken OnDamageTaken;

	UPROPERTY(BlueprintAssignable, Category = "Chronos|Health")
	FChronosOnDeath OnDeath;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChronosOnRevived, AActor*, Actor);
	UPROPERTY(BlueprintAssignable, Category = "Chronos|Health")
	FChronosOnRevived OnRevived;

private:
	UPROPERTY(VisibleAnywhere, Category = "Chronos|Health")
	FGameplayTagContainer StateTags;
};
