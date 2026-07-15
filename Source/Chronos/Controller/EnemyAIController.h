// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/AIModule/Classes/AIController.h"
#include "EnemyAIController.generated.h"

struct FAIStimulus;
class UBehaviorTreeComponent;

UCLASS()
class CHRONOS_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AEnemyAIController();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

protected:
	virtual void OnPossess(APawn* InPawn) override;
	void InitializePerception();
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (DisplayPriority = "1"))
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;
	
	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (DisplayPriority = "2"))
	TObjectPtr<UBlackboardData> BlackboardAsset;
};
