#include "GameFlow/ChronosPlayerController.h"

#include "Audio/ChronosAudioSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Characters/ChronosCharacter.h"
#include "Components/CombatComponent.h"
#include "Feedback/ChronosFeedbackSubsystem.h"
#include "GameFlow/ChronosGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Save/ChronosSaveSubsystem.h"
#include "Settings/ChronosGameUserSettings.h"
#include "Stats/ChronosStatsSubsystem.h"
#include "UI/ChronosHUD.h"
#include "UI/ChronosSettingsWidget.h"
#include "UI/ChronosVictoryWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "Visuals/ChronosVisualSubsystem.h"
#include "Weapons/ChronosWeapon.h"
#include "Weapons/WeaponDataAsset.h"

AChronosPlayerController::AChronosPlayerController()
{
	// 项目里没有 PlayerController 的蓝图子类，HUDClass 若只在编辑器里改 CDO，
	// 重启后就丢了（非 config 属性的 CDO 值不会持久化）。这里给个默认值兜底，
	// 需要换 HUD 时在蓝图子类或关卡 WorldSettings 里覆盖即可。
	static ConstructorHelpers::FClassFinder<UUserWidget> HudFinder(TEXT("/Game/Blueprints/UI/WBP_HUD"));
	if (HudFinder.Succeeded())
	{
		HUDClass = HudFinder.Class;
	}

	// 设置界面默认用纯 C++ 生成的那一版：不需要任何 Widget 蓝图资产。
	// 想在编辑器里改排版就建一个 UChronosSettingsWidget 的蓝图子类，
	// 然后在 BP_ShooterPlayerController 的类默认值里覆盖 SettingsMenuClass。
	SettingsMenuClass = UChronosSettingsWidget::StaticClass();
}

void AChronosPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 设置下发：引擎启动时只自动应用分辨率那一半，
	// CHRONOS 自己的玩法/音频字段必须等世界与子系统就绪后才能生效。
	if (UChronosGameUserSettings* Settings = UChronosGameUserSettings::GetChronosSettings())
	{
		Settings->ApplyVideoSettings();
		Settings->ApplyAudioSettings();
		Settings->ApplyGameplaySettings();
	}

	// 菜单场景（当前世界不在 GameMode 的关卡列表里）：不建 HUD，改显示主菜单，
	// 并把输入切成 UI 模式、显示鼠标，否则按钮点不到。
	if (const AChronosGameMode* GM = GetWorld()->GetAuthGameMode<AChronosGameMode>())
	{
		if (!GM->IsConfiguredLevel() && MainMenuClass)
		{
			ShowOverlay(MainMenuClass);
			bShowMouseCursor = true;
			SetInputMode(FInputModeUIOnly());
			return;
		}
		// 菜单关但没配 MainMenuClass：不 return，继续建 HUD，避免启动后一片黑
	}

	// 正式关卡：强制恢复游戏输入并隐藏鼠标。
	// 从主菜单 OpenLevel 过来时，若上一场景的 UI 输入模式/鼠标可见状态有残留，
	// 表现就是"进了游戏却连视角都转不动"。这里无论来源一律还原。
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;

	if (HUDClass)
	{
		HUD = CreateWidget<UUserWidget>(this, HUDClass);
		if (HUD)
		{
			HUD->AddToViewport();
			BindHUDToFeedback();

			// OnPossess 可能早于 BeginPlay，那时 HUD 还是nullptr、状态推送会被丢弃，
			// 于是 HUD 一直停在蓝图里的默认文本（"TextBlock"）上。这里补推一次。
			if (const AChronosCharacter* ChronosPawn = Cast<AChronosCharacter>(GetPawn()))
			{
				PushWeaponStateToHUD(ChronosPawn->GetCombatComponent());
			}
		}
	}

	if (AChronosGameMode* GM = GetWorld()->GetAuthGameMode<AChronosGameMode>())
	{
		GM->OnLevelCleared.AddDynamic(this, &AChronosPlayerController::HandleLevelCleared);
	}
}

void AChronosPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 玩家Pawn即 AChronosCharacter，死亡即失败（一击必杀规则）
	if (AChronosCharacter* ChronosPawn = Cast<AChronosCharacter>(InPawn))
	{
		ChronosPawn->OnCharacterDeath.AddUniqueDynamic(this, &AChronosPlayerController::HandlePlayerDeath);

		// 把战斗数据桥接到 HUD：C++ 管来源，蓝图管排版
		if (UCombatComponent* Combat = ChronosPawn->GetCombatComponent())
		{
			Combat->OnWeaponChanged.AddUniqueDynamic(this, &AChronosPlayerController::HandleWeaponChanged);
			Combat->OnAmmoChanged.AddUniqueDynamic(this, &AChronosPlayerController::HandleAmmoChanged);

			PushWeaponStateToHUD(Combat);
		}

		if (ChronosPawn->OnInteractionFocusChanged.IsAlreadyBound(this,
			&AChronosPlayerController::HandleInteractionFocusChanged))
		{
			ChronosPawn->OnInteractionFocusChanged.RemoveDynamic(this,
				&AChronosPlayerController::HandleInteractionFocusChanged);
		}
		ChronosPawn->OnInteractionFocusChanged.AddDynamic(this,
			&AChronosPlayerController::HandleInteractionFocusChanged);
	}
}

void AChronosPlayerController::BindHUDToFeedback()
{
	UChronosHUD* HUDWidget = Cast<UChronosHUD>(HUD);
	if (!HUDWidget)
	{
		return;
	}

	UWorld* World = GetWorld();

	if (UChronosFeedbackSubsystem* Feedback = World->GetSubsystem<UChronosFeedbackSubsystem>())
	{
		Feedback->OnHitMarker.AddUniqueDynamic(HUDWidget, &UChronosHUD::OnHitMarkerReceived);
		Feedback->OnDamageDirection.AddUniqueDynamic(HUDWidget, &UChronosHUD::OnDamageDirectionReceived);
	}

	if (UChronosStatsSubsystem* Stats = World->GetSubsystem<UChronosStatsSubsystem>())
	{
		Stats->OnComboChanged.AddUniqueDynamic(HUDWidget, &UChronosHUD::OnComboChanged);
	}
}

void AChronosPlayerController::HandleWeaponChanged(TScriptInterface<IWeaponUser> OldWeapon,
	TScriptInterface<IWeaponUser> NewWeapon)
{
	if (UChronosHUD* HUDWidget = Cast<UChronosHUD>(HUD))
	{
		FText WeaponName = FText::GetEmpty();
		if (const AChronosWeapon* Weapon = Cast<AChronosWeapon>(NewWeapon.GetObject()))
		{
			if (Weapon->WeaponData)
			{
				WeaponName = Weapon->WeaponData->WeaponName;
			}
		}

		HUDWidget->OnWeaponChanged(WeaponName, NewWeapon.GetObject() != nullptr);
	}

	// 换枪后立刻同步一次弹药，避免 HUD 停留在上一把的读数
	if (const AChronosCharacter* ChronosPawn = Cast<AChronosCharacter>(GetPawn()))
	{
		if (UCombatComponent* Combat = ChronosPawn->GetCombatComponent())
		{
			PushWeaponStateToHUD(Combat);
		}
	}
}

void AChronosPlayerController::HandleInteractionFocusChanged(bool bHasFocus, FText InteractionText)
{
	if (UChronosHUD* HUDWidget = Cast<UChronosHUD>(HUD))
	{
		HUDWidget->OnInteractionFocusChanged(bHasFocus, InteractionText);
	}
}

void AChronosPlayerController::HandleAmmoChanged(int32 RemainingAmmo, int32 MaxAmmo)
{
	if (UChronosHUD* HUDWidget = Cast<UChronosHUD>(HUD))
	{
		HUDWidget->OnAmmoChanged(RemainingAmmo, MaxAmmo);
	}
}

void AChronosPlayerController::PushWeaponStateToHUD(const UCombatComponent* Combat)
{
	if (!Combat || !HUD)
	{
		return;
	}

	UChronosHUD* HUDWidget = Cast<UChronosHUD>(HUD);
	if (!HUDWidget)
	{
		return;
	}

	const AChronosWeapon* Weapon = Cast<AChronosWeapon>(Combat->GetCurrentWeapon().GetObject());
	if (!Weapon)
	{
		HUDWidget->OnWeaponChanged(FText::GetEmpty(), false);
		HUDWidget->OnAmmoChanged(-1, 0);
		return;
	}

	const FText WeaponName = Weapon->WeaponData ? Weapon->WeaponData->WeaponName : FText::GetEmpty();
	HUDWidget->OnWeaponChanged(WeaponName, true);
	HUDWidget->OnAmmoChanged(
		IWeaponUser::Execute_GetCurrentAmmo(const_cast<AChronosWeapon*>(Weapon)),
		IWeaponUser::Execute_GetMaxAmmo(const_cast<AChronosWeapon*>(Weapon)));
}

void AChronosPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// R 重开是流程级输入，与 Enhanced Input 的 IMC 解耦，直接用原始键绑定
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AChronosPlayerController::RestartLevel_IfPermitted);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AChronosPlayerController::TogglePauseMenu);
}

void AChronosPlayerController::RestartLevel_IfPermitted()
{
	if (AChronosGameMode* GM = GetWorld()->GetAuthGameMode<AChronosGameMode>())
	{
		GM->RestartLevel();
	}
}

void AChronosPlayerController::HandlePlayerDeath(AChronosCharacter* DeadCharacter)
{
	UWorld* World = GetWorld();

	if (UChronosStatsSubsystem* Stats = World->GetSubsystem<UChronosStatsSubsystem>())
	{
		Stats->RegisterDeath();
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UChronosSaveSubsystem* Save = GameInstance->GetSubsystem<UChronosSaveSubsystem>())
		{
			Save->RegisterDeath();
		}
	}

	// 死亡瞬间：暗角转红 + 死亡音，让"被打到"这件事有明确收尾（随后才是 DEFEAT 界面）
	if (UChronosVisualSubsystem* Visuals = World->GetSubsystem<UChronosVisualSubsystem>())
	{
		Visuals->TriggerDamageFlash(1.f);
	}

	if (UChronosAudioSubsystem* Audio = World->GetSubsystem<UChronosAudioSubsystem>())
	{
		Audio->PlaySfx(EChronosSfx::PlayerDeath);
	}

	if (DefeatClass)
	{
		ShowOverlay(DefeatClass);
	}
}

void AChronosPlayerController::SubmitCurrentLevelResult()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AChronosGameMode* GM = World->GetAuthGameMode<AChronosGameMode>();
	UChronosStatsSubsystem* Stats = World->GetSubsystem<UChronosStatsSubsystem>();
	if (!GM || !Stats)
	{
		return;
	}

	LastRunResult = Stats->BuildRunResult(GM->GetCurrentLevelName());

	UChronosSaveSubsystem* Save = nullptr;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		Save = GameInstance->GetSubsystem<UChronosSaveSubsystem>();
	}

	if (Save)
	{
		bLastRunNewRecord = Save->SubmitRunResult(LastRunResult);
		Save->UnlockLevel(GM->GetCurrentLevelIndex() + 1);
	}
	else
	{
		bLastRunNewRecord = false;
	}
}

void AChronosPlayerController::HandleLevelCleared(bool bFinalLevel)
{
	// 每清一关都结算一次：统计子系统是 World 级的，换关就没了，
	// 而每关都该有自己的最佳成绩（SUPERHOT 的关卡就是拿来刷时间的）。
	SubmitCurrentLevelResult();

	if (bFinalLevel)
	{
		if (VictoryClass)
		{
			ShowOverlay(VictoryClass);

			// 推送结算数据。Widget 自己不去捞 GameMode / Stats，保持无状态。
			if (UChronosVictoryWidget* VictoryWidget = Cast<UChronosVictoryWidget>(ActiveOverlay))
			{
				VictoryWidget->SetRunResult(LastRunResult, bLastRunNewRecord);
			}
		}
		return;
	}

	bLevelFlowing = true;
	if (LevelTransitionClass)
	{
		ShowOverlay(LevelTransitionClass);

		// 兜底：过场 UI 应当在动画结束后自行调用 FinishLevelTransition，
		// 但它没配好或没调时流程会永远卡在过场上。定时兜底，宁可快一点也不卡死。
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(TransitionFallbackTimer, this,
				&AChronosPlayerController::FinishLevelTransition,
				FMath::Max(0.1f, LevelTransitionFallbackSeconds), /*bLoop=*/false);
		}
	}
	else
	{
		FinishLevelTransition(); // 未配置过场则直接进下一关
	}
}

void AChronosPlayerController::TogglePauseMenu()
{
	if (bIsPaused)
	{
		ClosePauseMenu();
		return;
	}

	// 过场/结算/死亡界面期间不允许暂停：暂停会顶掉 ActiveOverlay，恢复后界面就没了
	if (!PauseMenuClass || bLevelFlowing || (ActiveOverlay != nullptr))
	{
		return;
	}

	bIsPaused = true;
	ShowOverlay(PauseMenuClass);
	bShowMouseCursor = true;
	SetInputMode(FInputModeUIOnly());
	UGameplayStatics::SetGamePaused(this, true);
}

void AChronosPlayerController::ClosePauseMenu()
{
	if (!bIsPaused)
	{
		return;
	}

	bIsPaused = false;

	if (ActiveOverlay)
	{
		ActiveOverlay->RemoveFromParent();
		ActiveOverlay = nullptr;
	}

	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
	UGameplayStatics::SetGamePaused(this, false);
}

void AChronosPlayerController::ReturnToMainMenu()
{
	// 必须先解除暂停：SetGamePaused 是全局的，带着暂停进菜单会一片死寂
	if (bIsPaused)
	{
		bIsPaused = false;
		UGameplayStatics::SetGamePaused(this, false);
	}

	if (MainMenuLevel.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Chronos] 未配置 MainMenuLevel，无法返回主菜单"));
		return;
	}

	UGameplayStatics::OpenLevelBySoftObjectPtr(this, MainMenuLevel);
}

void AChronosPlayerController::OpenSettingsMenu(bool bFromMainMenu)
{
	if (SettingsWidget || !SettingsMenuClass)
	{
		return;
	}

	bSettingsFromMainMenu = bFromMainMenu;

	// 设置界面顶掉当前覆盖层（暂停菜单 / 主菜单），关闭时再把它放回来
	if (ActiveOverlay)
	{
		ActiveOverlay->RemoveFromParent();
		ActiveOverlay = nullptr;
	}

	SettingsWidget = CreateWidget<UUserWidget>(this, SettingsMenuClass);
	if (!SettingsWidget)
	{
		return;
	}

	if (UChronosSettingsWidget* Settings = Cast<UChronosSettingsWidget>(SettingsWidget))
	{
		Settings->OnClosed.BindUObject(this, &AChronosPlayerController::HandleSettingsClosed);
		Settings->RefreshFromSettings();
	}

	SettingsWidget->AddToViewport(20);
	bShowMouseCursor = true;
	SetInputMode(FInputModeUIOnly());
}

void AChronosPlayerController::HandleSettingsClosed(bool bSettingsChanged)
{
	if (SettingsWidget)
	{
		SettingsWidget->RemoveFromParent();
		SettingsWidget = nullptr;
	}

	if (bSettingsFromMainMenu)
	{
		bSettingsFromMainMenu = false;
		if (MainMenuClass)
		{
			ShowOverlay(MainMenuClass);
		}
		bShowMouseCursor = true;
		SetInputMode(FInputModeUIOnly());
		return;
	}

	// 从暂停菜单进来：把暂停菜单放回去（游戏仍然是 SetGamePaused 状态）
	if (PauseMenuClass)
	{
		ShowOverlay(PauseMenuClass);
		bShowMouseCursor = true;
		SetInputMode(FInputModeUIOnly());
		return;
	}

	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}

void AChronosPlayerController::FinishLevelTransition()
{
	// 无论是蓝图主动调用还是定时器兜底触发，都清掉定时器，避免重复推进
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TransitionFallbackTimer);
	}

	if (ActiveOverlay)
	{
		ActiveOverlay->RemoveFromParent();
		ActiveOverlay = nullptr;
	}

	if (bLevelFlowing)
	{
		bLevelFlowing = false;
		if (AChronosGameMode* GM = GetWorld()->GetAuthGameMode<AChronosGameMode>())
		{
			GM->LoadNextLevel();
		}
	}
}

void AChronosPlayerController::ShowOverlay(TSubclassOf<UUserWidget> WidgetClass)
{
	if (ActiveOverlay)
	{
		ActiveOverlay->RemoveFromParent();
	}
	ActiveOverlay = CreateWidget<UUserWidget>(this, WidgetClass);
	if (ActiveOverlay)
	{
		ActiveOverlay->AddToViewport(10);
	}
}
