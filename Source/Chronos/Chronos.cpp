// Copyright Epic Games, Inc. All Rights Reserved.

#include "Chronos.h"
#include "Modules/ModuleManager.h"
#include "Subsystems/TimeDilationSubsystem.h"
#include "Subsystems/ProjectilePoolSubsystem.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FChronosModule, Chronos, "Chronos" );

DEFINE_LOG_CATEGORY(LogChronos);

void FChronosModule::StartupModule()
{
	
}

void FChronosModule::ShutdownModule()
{
	
}

namespace ChronosSubsystems
{
	UTimeDilationSubsystem* GetTimeDilationSubsystem(const UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}
		
		return World->GetSubsystem<UTimeDilationSubsystem>();
	}
	
	UProjectilePoolSubsystem* GetProjectilePoolSubsystem(const UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}
		
		return World->GetSubsystem<UProjectilePoolSubsystem>();
	}
}

