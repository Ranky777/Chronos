// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class UTimeDilationSubsystem;
class UProjectilePoolSubsystem;

DECLARE_LOG_CATEGORY_EXTERN(LogChronos, Log, All);

class CHRONOS_API FChronosModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	static inline FChronosModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FChronosModule>("Chronos");
	}
};

namespace ChronosSubsystems
{
	CHRONOS_API UTimeDilationSubsystem* GetTimeDilationSubsystem(const UWorld* World);
	CHRONOS_API UProjectilePoolSubsystem* GetProjectilePoolSubsystem(const UWorld* World);
}
