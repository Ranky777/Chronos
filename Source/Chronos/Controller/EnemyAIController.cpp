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
	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
	PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyAIController::OnTargetPerceptionUpdated);
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
	if (!PerceptionComponent)
	{
		return;
	}

	float DetectionRange = 2000.0f;
	float LoseDetectionRange = 2500.0f;

	if (AEnemyCharacter *EnemyCharacter = Cast<AEnemyCharacter>(GetPawn()))
	{
		const FEnemyData &EnemyData = EnemyCharacter->GetEnemyData();
		DetectionRange = EnemyData.DetectionRange;
		LoseDetectionRange = EnemyData.DetectionRange * 1.25f;
	}

	UAISenseConfig_Sight *SightConfig = NewObject<UAISenseConfig_Sight>(PerceptionComponent, TEXT("SightConfig"));
	SightConfig->SightRadius = DetectionRange;
	SightConfig->LoseSightRadius = LoseDetectionRange;
	SightConfig->PeripheralVisionAngleDegrees = 180.0f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;

	PerceptionComponent->ConfigureSense(*SightConfig);
	PerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
}

void AEnemyAIController::OnTargetPerceptionUpdated(AActor *Actor, FAIStimulus Stimulus)
{
	if (!Actor || !GetBlackboardComponent())
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		GetBlackboardComponent()->SetValueAsObject("TargetActor", Actor);
		GetBlackboardComponent()->SetValueAsBool("HasLineOfSight", true);
	}
	else
	{
		AActor *CurrentTarget = Cast<AActor>(GetBlackboardComponent()->GetValueAsObject("TargetActor"));
		if (CurrentTarget == Actor)
		{
			GetBlackboardComponent()->SetValueAsBool("HasLineOfSight", false);

			float Distance = FVector::Distance(Actor->GetActorLocation(), GetPawn()->GetActorLocation());
			float LoseRange = 2500.0f;

			if (AEnemyCharacter *EnemyCharacter = Cast<AEnemyCharacter>(GetPawn()))
			{
				const FEnemyData &EnemyData = EnemyCharacter->GetEnemyData();
				LoseRange = EnemyData.DetectionRange * 1.25f;
			}

			if (Distance > LoseRange)
			{
				GetBlackboardComponent()->ClearValue("TargetActor");
			}
		}
	}
}
