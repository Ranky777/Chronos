// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EnemyPatrol.generated.h"

/**
 * 
 */
UCLASS()
class CHRONOS_API UBTTask_EnemyPatrol : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_EnemyPatrol();
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	
protected:
	/** 巡逻半径 */
	UPROPERTY(EditAnywhere, Category = "Patrol")
	float PatrolRadius = 500.0f;

	/** 到达目标点的容差 */
	UPROPERTY(EditAnywhere, Category = "Patrol")
	float AcceptanceRadius = 50.0f;
	
	/** Blackboard Key名称 - 巡逻位置 */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName PatrolLocationKey = FName("PatrolLocation");
	
	bool bReachedDestination = false;
};
