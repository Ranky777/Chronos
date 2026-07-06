// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_EnemyAttack.h"
#include "AIController.h"
#include "Chronos/Characters/EnemyCharacter.h"
#include "Chronos/Components/WeaponComponent.h"

UBTTask_EnemyAttack::UBTTask_EnemyAttack()
{
	NodeName = "Enemy Attack";
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
	
	if (WeaponComponent && WeaponComponent->CanFire())
	{
		WeaponComponent->Fire();
		
		return EBTNodeResult::Succeeded;
	}
	
	return EBTNodeResult::Failed;
}
