// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyAIController.h"

#include "Perception/AISenseConfig_Sight.h"
#include "Runtime/AIModule/Classes/BehaviorTree/BehaviorTreeComponent.h"
#include "Runtime/AIModule/Classes/BehaviorTree/BlackboardComponent.h"
#include "Runtime/AIModule/Classes/Perception/AIPerceptionComponent.h"


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

void AEnemyAIController::OnProcess(APawn* InPawn)
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
		LocalPerceptionComponent = NewObject<UAIPerceptionComponent>(this);
		LocalPerceptionComponent->RegisterComponent();
	}
	
	UAISenseConfig_Sight* SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 2000.0f;
	SightConfig->LoseSightRadius = 2500.0f;
	SightConfig->PeripheralVisionAngleDegrees = 90.0f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
	
	LocalPerceptionComponent->ConfigureSense(*SightConfig);
	LocalPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
}
