// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EnemyAttack.generated.h"

/**
 * 
 */
UCLASS()
class CHRONOS_API UBTTask_EnemyAttack : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_EnemyAttack();
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	
protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName LastAttackTimeKey = FName("LastAttackTime");
	
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName TargetActorKey = FName("TargetActor");
	
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = 0.1f, ClampMax = 10.0f))
	float MaxAttackDistance = 1500.0f;
};
