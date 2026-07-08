// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class CHRONOS_API IInteractable
{
	GENERATED_BODY()
	
public:
	/**
	 * 执行交互操作
	 * @param Interactor 发起交互的Actor（通常是玩家）
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnInteract(AActor* Interactor);
	
	/**
	 * 检查是否可以进行交互
	 * @return 如果可以交互返回true，否则返回false
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract() const;
	
	/**
	* 获取交互名称（用于UI提示）
	* @return 交互名称文本
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetInteractionName() const;

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
};
