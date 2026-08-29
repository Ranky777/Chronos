#include "GameFlow/ChronosPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Characters/ChronosCharacter.h"
#include "GameFlow/ChronosGameMode.h"

void AChronosPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (HUDClass)
	{
		HUD = CreateWidget<UUserWidget>(this, HUDClass);
		if (HUD)
		{
			HUD->AddToViewport();
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
	}
}

void AChronosPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// R 重开是流程级输入，与 Enhanced Input 的 IMC 解耦，直接用原始键绑定
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AChronosPlayerController::RestartLevel_IfPermitted);
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
	if (DefeatClass)
	{
		ShowOverlay(DefeatClass);
	}
}

void AChronosPlayerController::HandleLevelCleared(bool bFinalLevel)
{
	if (bFinalLevel)
	{
		if (VictoryClass)
		{
			ShowOverlay(VictoryClass);
		}
		return;
	}

	bLevelFlowing = true;
	if (LevelTransitionClass)
	{
		ShowOverlay(LevelTransitionClass);
	}
	else
	{
		FinishLevelTransition(); // 未配置过场则直接进下一关
	}
}

void AChronosPlayerController::FinishLevelTransition()
{
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
