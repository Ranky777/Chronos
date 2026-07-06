// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_EnemyPatrol.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Chronos/Characters/EnemyCharacter.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_EnemyPatrol::UBTTask_EnemyPatrol()
{
	NodeName = "Enemy Patrol";
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_EnemyPatrol::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}
	
	AEnemyCharacter* EnemyCharacter = Cast<AEnemyCharacter>(AIController->GetPawn());
	
	if (!EnemyCharacter)
	{
		return EBTNodeResult::Failed;
	}
	
	FVector CurrentLocation = EnemyCharacter->GetActorLocation();
	
	float RandomX = FMath::FRandRange(-PatrolRadius, PatrolRadius);
	float RandomY = FMath::FRandRange(-PatrolRadius, PatrolRadius);
	FVector PatrolLocation = CurrentLocation + FVector(RandomX, RandomY, 0.0f);
	
	OwnerComp.GetBlackboardComponent()->SetValueAsVector(PatrolLocationKey, PatrolLocation);
	
	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalLocation(PatrolLocation);
	MoveRequest.SetAcceptanceRadius(AcceptanceRadius);
	AIController->MoveTo(MoveRequest);
	
	bReachedDestination = false;
	return EBTNodeResult::InProgress;
}

void UBTTask_EnemyPatrol::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	
	if (!AIController)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		
		return;
	}
	
	AEnemyCharacter* EnemyCharacter = Cast<AEnemyCharacter>(AIController->GetPawn());
	
	if (!EnemyCharacter)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		
		return;
	}
	
	FVector PatrolLocation = OwnerComp.GetBlackboardComponent()->GetValueAsVector(PatrolLocationKey);
	float Distance = FVector::Distance(EnemyCharacter->GetActorLocation(), PatrolLocation);
	
	if (Distance <= AcceptanceRadius)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
