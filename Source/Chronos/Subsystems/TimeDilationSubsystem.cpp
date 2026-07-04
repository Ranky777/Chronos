// Fill out your copyright notice in the Description page of Project Settings.


#include "TimeDilationSubsystem.h"

UTimeDilationSubsystem::UTimeDilationSubsystem()
{
	CurrentTimeDilation = MaxTimeDilation;
	TargetTimeDilation = MaxTimeDilation;
}

void UTimeDilationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UTimeDilationSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UTimeDilationSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (bPlayerDead)
	{
		CurrentTimeDilation = MaxTimeDilation;
		TargetTimeDilation = MaxTimeDilation;
		
		if (GetWorld())
		{
			GetWorld()->GetWorldSettings()->SetTimeDilation(CurrentTimeDilation);
		}
		
		return;
	}
	
	if (!bInputActive)
	{
		IdleTime += DeltaTime;
		
		if (IdleTime >= TimeToSlow)
		{
			TargetTimeDilation = MinTimeDilation;
		}
	}
	else
	{
		IdleTime = 0.0f;
		TargetTimeDilation = MaxTimeDilation;
	}
	
	UpdateTimeDilation(DeltaTime);
}

void UTimeDilationSubsystem::UpdateTimeDilation(float DeltaTime)
{
	float OldTimeDilation = CurrentTimeDilation;
	
	float InterpSpeed = (TargetTimeDilation < CurrentTimeDilation) ? (MaxTimeDilation - MinTimeDilation) / TimeToSlow : (MaxTimeDilation - MinTimeDilation) / TimeToSpeedUp;
	
	CurrentTimeDilation = FMath::FInterpTo(CurrentTimeDilation, TargetTimeDilation, DeltaTime, InterpSpeed);
	
	CurrentTimeDilation = FMath::Clamp(CurrentTimeDilation, MinTimeDilation, MaxTimeDilation);
	
	if (GetWorld())
	{
		GetWorld()->GetWorldSettings()->SetTimeDilation(CurrentTimeDilation);
	}
	
	if (FMath::Abs(CurrentTimeDilation - OldTimeDilation) > 0.001f)
	{
		OnTimeDilationChanged.Broadcast(CurrentTimeDilation, OldTimeDilation);
	}
}

void UTimeDilationSubsystem::SetInputActive(bool bActive)
{
	bInputActive = bActive;
}

void UTimeDilationSubsystem::SetPlayerDead(bool bDead)
{
	bPlayerDead = bDead;
}

