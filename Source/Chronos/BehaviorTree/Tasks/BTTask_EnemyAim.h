// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EnemyAim.generated.h"

/**
 * 
 */
UCLASS()
class CHRONOS_API UBTTask_EnemyAim : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_EnemyAim();
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	
protected:
	/** 旋转速度系数 */
	UPROPERTY(EditAnywhere, Category = "Aim", meta = (ClampMin = 1.0f, ClampMax = 20.0f))
	float RotationSpeed = 5.0f;

	/** 瞄准容差角度（度）- 达到这个角度认为已经瞄准 */
	UPROPERTY(EditAnywhere, Category = "Aim", meta = (ClampMin = 0.0f, ClampMax = 30.0f))
	float AimTolerance = 5.0f;

	/** Blackboard Key名称 - 目标玩家 */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName TargetActorKey = FName("TargetActor");
};
