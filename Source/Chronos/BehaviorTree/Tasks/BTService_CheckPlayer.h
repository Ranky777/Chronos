// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_CheckPlayer.generated.h"

/**
 * 
 */
UCLASS()
class CHRONOS_API UBTService_CheckPlayer : public UBTService
{
	GENERATED_BODY()
	
public:
	UBTService_CheckPlayer();
	
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	
public:
	/** 检测半径 */
	UPROPERTY(EditAnywhere, Category = "Detection")
	float DetectionRadius = 2000.0f;

	/** Blackboard Key名称 - 目标玩家 */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName TargetActorKey = FName("TargetActor");

	/** Blackboard Key名称 - 是否有视线 */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName HasLineOfSightKey = FName("HasLineOfSight");
};
