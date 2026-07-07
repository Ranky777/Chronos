// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ChronosPlayerController.generated.h"

class UInputMappingContext;
class UChronosHUD;
/**
 * 
 */
UCLASS()
class CHRONOS_API AChronosPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AChronosPlayerController();
	
protected:
	virtual void BeginPlay() override;
	
	virtual void SetupInputComponent() override;
	
public:
	/**
	 * 获取当前HUD
	 * @return ChronosHUD指针
	 */
	UFUNCTION(BlueprintCallable, Category = "Player Controller")
	UChronosHUD* GetChronosHUD() const { return ChronosHUD; }
	
protected:
	/**
	* 初始化输入系统
	* 将InputMappingContext推送到增强输入子系统
	*/
	void InitializeInputSystem();
	
	void CreateHUD();
	
private:
	/** 输入映射上下文资产 */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	/** 输入映射优先级 */
	UPROPERTY(EditAnywhere, Category = "Input", meta = (ClampMin = 0, ClampMax = 100))
	int32 InputMappingPriority = 0;

	/** HUD类模板 */
	UPROPERTY(EditAnywhere, Category = "HUD")
	TSubclassOf<UChronosHUD> ChronosHUDClass;

	/** HUD实例 */
	TObjectPtr<UChronosHUD> ChronosHUD;
};
