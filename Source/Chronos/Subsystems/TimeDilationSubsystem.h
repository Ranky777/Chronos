// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TimeDilationSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTimeDilationChanged, float, NewTimeDilation, float, OldTimeDilation);

/**
 * 
 */
UCLASS()
class CHRONOS_API UTimeDilationSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
	
public:
	UTimeDilationSubsystem();
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable, Category = "Time Dilation")
	float GetCurrentTimeDilation() const
	{
		return CurrentTimeDilation;
	}
	
	UFUNCTION(BlueprintCallable, Category = "Time Dilation")
	float GetTargetTimeDilation() const
	{
		return TargetTimeDilation;
	}
	
	UFUNCTION(BlueprintCallable, Category = "Time Dilation")
	bool IsTimeDilated() const
	{
		return CurrentTimeDilation < 1.0f;
	}
	
	UFUNCTION(BlueprintCallable, Category = "Time Dilation")
	void SetInputActive(bool bActive);

	UFUNCTION(BlueprintCallable, Category = "Time Dilation")
	void SetPlayerDead(bool bDead);
	
public:
	UPROPERTY(BlueprintAssignable, Category = "Time Dilation")
	FOnTimeDilationChanged OnTimeDilationChanged;
	
private:
	void UpdateTimeDilation(float DeltaTime);
	
private:
	UPROPERTY(EditAnywhere, Category = "Time Dilation", meta = (AllowPrivateAccess = "true"))
	float MinTimeDilation = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Time Dilation", meta = (AllowPrivateAccess = "true"))
	float MaxTimeDilation = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Time Dilation", meta = (AllowPrivateAccess = "true"))
	float TimeToSlow = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Time Dilation", meta = (AllowPrivateAccess = "true"))
	float TimeToSpeedUp = 0.3f;

	// UPROPERTY(EditAnywhere, Category = "Time Dilation", meta = (AllowPrivateAccess = "true"))
	// float InputIdleThreshold = 0.1f;

	float CurrentTimeDilation = 1.0f;
	float TargetTimeDilation = 1.0f;
	bool bInputActive = false;
	bool bPlayerDead = false;
	float IdleTime = 0.0f;

	FTimerHandle TickTimerHandle;
};
