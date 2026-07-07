// Fill out your copyright notice in the Description page of Project Settings.


#include "ChronosPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "Chronos/HUD/ChronosHUD.h"

AChronosPlayerController::AChronosPlayerController()
{
	bShowMouseCursor = false;
}

void AChronosPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	InitializeInputSystem();
	
	CreateHUD();
}

void AChronosPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// 输入映射将在PlayerCharacter中绑定
		// 这里不需要绑定具体的输入动作
	}
}

void AChronosPlayerController::InitializeInputSystem()
{
	if (!InputMappingContext)
	{
		UE_LOG(LogTemp, Warning, TEXT("ChronosPlayerController: InputMappingContext not set"));
		
		return;
	}
	
	// 获取增强输入本地玩家子系统
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!InputSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("ChronosPlayerController: EnhancedInputLocalPlayerSubsystem not found"));
		
		return;
	}
	
	// 推送输入映射上下文到子系统
	InputSubsystem->AddMappingContext(InputMappingContext, InputMappingPriority);
}

void AChronosPlayerController::CreateHUD()
{
	if (!ChronosHUDClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ChronosPlayerController: ChronosHUDClass not set"));
		
		return;
	}
	
	// 创建HUD Widget实例
	ChronosHUD = CreateWidget<UChronosHUD>(this, ChronosHUDClass);
	if (ChronosHUD)
	{
		// 将HUD添加到视口
		ChronosHUD->AddToViewport();
		UE_LOG(LogTemp, Log, TEXT("ChronosPlayerController: HUD created successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ChronosPlayerController: Failed to create HUD"));
	}
}
