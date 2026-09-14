#pragma once

#include "GameFramework/PlayerController.h"
#include "Save/ChronosSaveGame.h"
#include "TimerManager.h"
#include "Weapons/WeaponUser.h"
#include "ChronosPlayerController.generated.h"

class AChronosCharacter;
class UCombatComponent;
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
	AChronosPlayerController();

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

	/**
	 * 主菜单。当当前世界不在 GameMode 的关卡列表里（即菜单场景）时显示它，
	 * 同时切成 UI 输入模式并显示鼠标。正式关卡里不会创建。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Chronos|UI")
	TSubclassOf<UUserWidget> MainMenuClass;

	/**
	 * 过场兜底时长（秒）。过场 UI 理应在自己的动画结束后调用 FinishLevelTransition，
	 * 但它没调（或没配）时流程会永久卡在过场上 —— 用定时器兜底，宁可快一点也不卡死。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Chronos|UI")
	float LevelTransitionFallbackSeconds = 2.f;

	/** 过场结束：由 WBP_LevelTransition 蓝图调用，未被调用时由兜底定时器触发 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Flow")
	void FinishLevelTransition();

	/** 暂停菜单（ESC） */
	UPROPERTY(EditDefaultsOnly, Category = "Chronos|UI")
	TSubclassOf<UUserWidget> PauseMenuClass;

	/** "返回主菜单"要打开的关卡 */
	UPROPERTY(EditDefaultsOnly, Category = "Chronos|UI")
	TSoftObjectPtr<UWorld> MainMenuLevel;

	/** ESC：在暂停与继续之间切换 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Flow")
	void TogglePauseMenu();

	/** 关闭暂停菜单并恢复游戏；由暂停 Widget 的"继续"调用 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Flow")
	void ClosePauseMenu();

	/** 返回主菜单；由暂停 Widget 调用 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Flow")
	void ReturnToMainMenu();

	/**
	 * 设置界面。
	 * 默认是纯 C++ 生成的 UChronosSettingsWidget（零资产依赖），
	 * 想换成蓝图版就在 BP_ShooterPlayerController 的类默认值里覆盖。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Chronos|UI")
	TSubclassOf<UUserWidget> SettingsMenuClass;

	/**
	 * 打开设置界面。
	 * @param bFromMainMenu true = 关闭后回主菜单；false = 关闭后回暂停菜单
	 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Flow")
	void OpenSettingsMenu(bool bFromMainMenu);

	FTimerHandle TransitionFallbackTimer;

	bool bIsPaused = false;

protected:
	/** 参数名避开 AController::Character（UHT 禁止 UFUNCTION 参数遮蔽基类成员） */
	UFUNCTION()
	void HandlePlayerDeath(AChronosCharacter* DeadCharacter);

	UFUNCTION()
	void HandleLevelCleared(bool bFinalLevel);

	/** 战斗数据 → HUD 的桥接 */
	UFUNCTION()
	void HandleWeaponChanged(TScriptInterface<IWeaponUser> OldWeapon, TScriptInterface<IWeaponUser> NewWeapon);

	UFUNCTION()
	void HandleAmmoChanged(int32 RemainingAmmo, int32 MaxAmmo);

	/**
	 * 交互焦点 → HUD 的转发层。
	 * 用 FText 按值传参：动态委托要求签名逐字匹配，而 HUD 的原生事件参数是 const FText&。
	 */
	UFUNCTION()
	void HandleInteractionFocusChanged(bool bHasFocus, FText InteractionText);

	/** 换枪/初次 Possess 时把当前状态推给 HUD */
	void PushWeaponStateToHUD(const class UCombatComponent* Combat);

	void ShowOverlay(TSubclassOf<UUserWidget> WidgetClass);

	void RestartLevel_IfPermitted();

	/** 关掉设置界面：恢复来源界面（主菜单或暂停菜单） */
	void HandleSettingsClosed(bool bSettingsChanged);

	/** HUD 创建后把打击感事件（命中标记 / 连杀 / 受伤方向）接到它上面 */
	void BindHUDToFeedback();

	/** 把本关成绩写进存档并解锁下一关 */
	void SubmitCurrentLevelResult();

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> HUD;

	UPROPERTY()
	TObjectPtr<UUserWidget> ActiveOverlay;

	UPROPERTY()
	TObjectPtr<UUserWidget> SettingsWidget;

	bool bLevelFlowing = false;

	/** 设置界面是从主菜单打开的（关闭时要回主菜单而不是暂停菜单） */
	bool bSettingsFromMainMenu = false;

	/** 最近一次提交的成绩，结算界面用 */
	FChronosRunResult LastRunResult;

	bool bLastRunNewRecord = false;
};
