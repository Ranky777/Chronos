// Fill out your copyright notice in the Description page of Project Settings.


#include "BTService_CheckPlayer.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Chronos/Characters/EnemyCharacter.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"

UBTService_CheckPlayer::UBTService_CheckPlayer()
{
	NodeName = "Check Player";
	bNotifyTick = true;
}

void UBTService_CheckPlayer::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return;
	}
	
	AEnemyCharacter* EnemyCharacter = Cast<AEnemyCharacter>(AIController->GetPawn());
	if (!EnemyCharacter)
	{
		return;
	}
	
	UAIPerceptionComponent* PerceptionComponent = AIController->GetComponentByClass<UAIPerceptionComponent>();
	if (!PerceptionComponent)
	{
		return;
	}
	
	TArray<AActor*> PerceivedActors;
	PerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
	
	if (PerceivedActors.Num() > 0)
	{
		for (AActor* Actor : PerceivedActors)
		{
			if (APawn* PlayerPawn = Cast<APawn>(Actor))
			{
				OwnerComp.GetBlackboardComponent()->SetValueAsObject(TargetActorKey, PlayerPawn);
				OwnerComp.GetBlackboardComponent()->SetValueAsBool(HasLineOfSightKey, true);
				
				return;
			}
		}
	}
	
	// 如果没有感知到玩家，清空目标
	OwnerComp.GetBlackboardComponent()->ClearValue(TargetActorKey);
	OwnerComp.GetBlackboardComponent()->SetValueAsBool(HasLineOfSightKey, false);
}
