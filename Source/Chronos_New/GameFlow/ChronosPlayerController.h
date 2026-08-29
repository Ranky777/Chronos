#pragma once

#include "GameFramework/PlayerController.h"
#include "ChronosPlayerController.generated.h"

class AChronosCharacter;
class UUserWidget;

/**
 * 玩家控制器：创建常驻 HUD 与流程界面（清关过场/胜负），
 * 监听 GameMode 清关与玩家死亡事件；R 键重开当前关（含死亡后）。
 */
UCLASS(Blueprintable)
class CHRONOS_NEW_API AChronosPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chronos|UI")
	TSubclassOf<UUserWidget> HUDClass;

	/** 清关过场（SUPERHOT 闪烁字）。其内部动画/计时结束后应调用 FinishLevelTransition */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chronos|UI")
	TSubclassOf<UUserWidget> LevelTransitionClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chronos|UI")
	TSubclassOf<UUserWidget> VictoryClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chronos|UI")
	TSubclassOf<UUserWidget> DefeatClass;

	/** 过场结束：由 WBP_LevelTransition 蓝图调用 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Flow")
	void FinishLevelTransition();

protected:
	/** 参数名避开 AController::Character（UHT 禁止 UFUNCTION 参数遮蔽基类成员） */
	UFUNCTION()
	void HandlePlayerDeath(AChronosCharacter* DeadCharacter);

	UFUNCTION()
	void HandleLevelCleared(bool bFinalLevel);

	void ShowOverlay(TSubclassOf<UUserWidget> WidgetClass);

	void RestartLevel_IfPermitted();

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> HUD;

	UPROPERTY()
	TObjectPtr<UUserWidget> ActiveOverlay;

	bool bLevelFlowing = false;
};
