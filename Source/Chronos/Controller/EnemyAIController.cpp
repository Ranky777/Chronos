// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyAIController.h"

#include "Perception/AISenseConfig_Sight.h"
#include "Runtime/AIModule/Classes/BehaviorTree/BehaviorTreeComponent.h"
#include "Runtime/AIModule/Classes/BehaviorTree/BlackboardComponent.h"
#include "Runtime/AIModule/Classes/Perception/AIPerceptionComponent.h"
#include "Chronos/Characters/EnemyCharacter.h"


// Sets default values
AEnemyAIController::AEnemyAIController()
{
}

// Called when the game starts or when spawned
void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();
	
	InitializePerception();
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	if (BlackboardAsset && GetBlackboardComponent())
	{
		GetBlackboardComponent()->InitializeBlackboard(*BlackboardAsset);
	}
	
	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}
}

void AEnemyAIController::InitializePerception()
{
	UAIPerceptionComponent* LocalPerceptionComponent = GetComponentByClass<UAIPerceptionComponent>();
	
	if (!LocalPerceptionComponent)
	{
		LocalPerceptionComponent = NewObject<UAIPerceptionComponent>(this, TEXT("AIPerceptionComponent"));
		LocalPerceptionComponent->RegisterComponent();
	}
	
	float DetectionRange = 2000.0f;
	float LoseDetectionRange = 2500.0f;
	
	if (AEnemyCharacter* EnemyCharacter = Cast<AEnemyCharacter>(GetPawn()))
	{
		const FEnemyData& EnemyData = EnemyCharacter->GetEnemyData();
		DetectionRange = EnemyData.DetectionRange;
		LoseDetectionRange = EnemyData.DetectionRange * 1.25f;
	}
	
	UAISenseConfig_Sight* SightConfig = NewObject<UAISenseConfig_Sight>(LocalPerceptionComponent, TEXT("SightConfig"));
	SightConfig->SightRadius = DetectionRange;
	SightConfig->LoseSightRadius = LoseDetectionRange;
	SightConfig->PeripheralVisionAngleDegrees = 90.0f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
	
	LocalPerceptionComponent->ConfigureSense(*SightConfig);
	LocalPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
}
