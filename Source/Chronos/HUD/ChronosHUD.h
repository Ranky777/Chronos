// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChronosHUD.generated.h"

enum class EGameState : uint8;
class UTextBlock;
/**
 * 
 */
UCLASS()
class CHRONOS_API UChronosHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	* 更新时间流速显示
	*/
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateTimeDilation(float NewTimeDilation, float OldTimeDilation);
	
	/**
	 * 更新弹药显示
	 * @param CurrentAmmo 当前弹药数
	 * @param MaxAmmo 最大弹药数
	 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateAmmo(int32 CurrentAmmo, int32 MaxAmmo);

	/**
	 * 更新敌人数量显示
	 * @param Remaining 剩余敌人数
	 * @param Total 总敌人数
	 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateEnemyCount(int32 Remaining, int32 Total);

	/**
	 * 显示游戏结束界面
	 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowGameOver();

	/**
	 * 显示通关界面
	 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowVictory();

	/**
	 * 隐藏所有界面
	 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void HideAll();
	
protected:
	virtual void NativeConstruct() override;
	
	/**
	 * 绑定游戏模式事件
	 */
	void BindGameModeEvents();

	/**
	 * 绑定时间子系统事件
	 */
	void BindTimeDilationEvents();

	/**
	 * 绑定武器组件事件
	 */
	void BindWeaponEvents();
	
private:
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void HandleGameStateChanged(EGameState NewState);
	
private:
	/** 时间流速文本 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TimeDilationText;

	/** 弹药文本 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AmmoText;

	/** 敌人数量文本 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> EnemyCountText;

	/** 游戏结束界面 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> GameOverWidget;

	/** 通关界面 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> VictoryWidget;
};
