// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_EnemyAttack.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Chronos/Characters/EnemyCharacter.h"
#include "Chronos/Components/WeaponComponent.h"

UBTTask_EnemyAttack::UBTTask_EnemyAttack()
{
	NodeName = "Enemy Attack";
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_EnemyAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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
	
	UWeaponComponent* WeaponComponent = EnemyCharacter->GetWeaponComponent();
	
	if (!WeaponComponent || !WeaponComponent->HasWeapon())
	{
		return EBTNodeResult::Failed;
	}
	
	return EBTNodeResult::InProgress;
}

void UBTTask_EnemyAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);
	
	AAIController* AIController = OwnerComp.GetAIOwner();
	
	if (!AIController)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	
	AEnemyCharacter* EnemyCharacter = Cast<AEnemyCharacter>(AIController->GetPawn());
	
	if (!EnemyCharacter || !EnemyCharacter->IsAlive())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	
	UWeaponComponent* WeaponComponent = EnemyCharacter->GetWeaponComponent();
	
	if (!WeaponComponent || !WeaponComponent->HasWeapon())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	
	AActor* TargetActor = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(TargetActorKey));
	if (!TargetActor)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	
	float Distance = FVector::Distance(EnemyCharacter->GetActorLocation(), TargetActor->GetActorLocation());
	if (Distance > MaxAttackDistance)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	
	float CurrentTime = GetWorld()->GetTimeSeconds();
	float LastAttackTime = OwnerComp.GetBlackboardComponent()->GetValueAsFloat(LastAttackTimeKey);
	float ShootingFrequency = EnemyCharacter->GetEnemyData().ShootingFrequency;
	
	if (CurrentTime - LastAttackTime >= ShootingFrequency)
	{
		if (WeaponComponent->CanFire_Implementation())
		{
			WeaponComponent->Fire_Implementation();
			OwnerComp.GetBlackboardComponent()->SetValueAsFloat(LastAttackTimeKey, CurrentTime);
		}
	}
}
