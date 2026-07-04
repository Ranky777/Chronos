// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TimeDilationListener.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType)
class UTimeDilationListener : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class CHRONOS_API ITimeDilationListener
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Time Dilation")
	void OnTimeDilationChanged(float NewTimeDilation, float OldTimeDilation);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Time Dilation")
	void OnTimeDilationPaused();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Time Dilation")
	void OnTimeDilationResumed();
};
