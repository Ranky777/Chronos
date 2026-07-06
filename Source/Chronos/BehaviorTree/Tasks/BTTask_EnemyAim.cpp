// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_EnemyAim.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Chronos/Characters/EnemyCharacter.h"

UBTTask_EnemyAim::UBTTask_EnemyAim()
{
	NodeName = "Enemy Aim";
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_EnemyAim::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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
	
	AActor* TargetActor = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(TargetActorKey));
	if (!TargetActor)
	{
		return EBTNodeResult::Failed;
	}
	
	return EBTNodeResult::InProgress;
}

void UBTTask_EnemyAim::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);
	
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
	
	AActor* TargetActor = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(TargetActorKey));
	if (!TargetActor)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		
		return;
	}
	
	// 计算朝向玩家的旋转
	FVector DirectionToPlayer = (TargetActor->GetActorLocation() - EnemyCharacter->GetActorLocation()).GetSafeNormal();
	FRotator TargetRotation = DirectionToPlayer.Rotation(); 
	
	// 获取当前旋转（只使用Yaw轴进行水平旋转）
	FRotator CurrentRotation = EnemyCharacter->GetActorRotation();
	FRotator NewRotation = FRotator(CurrentRotation.Pitch, TargetRotation.Yaw, CurrentRotation.Roll);
	
	FRotator InterpolatedRotation = FMath::RInterpTo(CurrentRotation, NewRotation, DeltaSeconds, RotationSpeed);
	EnemyCharacter->SetActorRotation(InterpolatedRotation);
	
	float AngleDiff = FMath::Abs(CurrentRotation.Yaw - TargetRotation.Yaw);
	
	if (AngleDiff <= AimTolerance)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
