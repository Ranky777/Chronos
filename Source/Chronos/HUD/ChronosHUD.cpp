// Fill out your copyright notice in the Description page of Project Settings.


#include "ChronosHUD.h"

#include "Chronos/Chronos.h"
#include "Chronos/Characters/PlayerCharacter.h"
#include "Chronos/Components/WeaponComponent.h"
#include "Chronos/GameModes/ChronosGameMode.h"
#include "Chronos/Subsystems/TimeDilationSubsystem.h"
#include "Components/TextBlock.h"

void UChronosHUD::UpdateTimeDilation(float NewTimeDilation, float OldTimeDilation)
{
	if (TimeDilationText)
	{
		int32 Percentage = FMath::RoundToInt(NewTimeDilation * 100.0f);
		TimeDilationText->SetText(FText::Format(NSLOCTEXT("ChronosHUD", "TimeDilationFormat", "TIME: {0}%"), Percentage));
	}
}

void UChronosHUD::UpdateAmmo(int32 RemainingAmmo, int32 TotalAmmo)
{
	if (AmmoText)
	{
		if (TotalAmmo > 0)
		{
			// SUPERHOT风格：只显示剩余子弹数，无弹匣概念
			AmmoText->SetText(FText::Format(NSLOCTEXT("ChronosHUD", "AmmoFormat", "AMMO: {0}"), RemainingAmmo));
		}
		else
		{
			// 无武器时显示为空或提示
			AmmoText->SetText(FText::GetEmpty());
		}
	}
}

void UChronosHUD::UpdateEnemyCount(int32 Remaining, int32 Total)
{
	if (EnemyCountText)
	{
		EnemyCountText->SetText(FText::Format(NSLOCTEXT("ChronosHUD", "EnemyCountFormat", "ENEMIES: {0}/{1}"), Remaining, Total));
	}
}

void UChronosHUD::ShowGameOver()
{
	if (GameOverWidget)
	{
		GameOverWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void UChronosHUD::ShowVictory()
{
	if (VictoryWidget)
	{
		VictoryWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void UChronosHUD::HideAll()
{
	if (GameOverWidget)
	{
		GameOverWidget->SetVisibility(ESlateVisibility::Hidden);
	}
    
	if (VictoryWidget)
	{
		VictoryWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UChronosHUD::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 绑定游戏模式事件
	BindGameModeEvents();
	
	// 绑定时间 dilation 事件
	BindTimeDilationEvents();
	
	// 绑定武器事件
	BindWeaponEvents();
	
	// 初始化显示
	UpdateTimeDilation(1.0f, 1.0f);
	UpdateAmmo(0, 0);
	UpdateEnemyCount(0, 0);
}

void UChronosHUD::BindGameModeEvents()
{
	if (AChronosGameMode* GameMode = Cast<AChronosGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GameMode->OnGameStateChanged.AddDynamic(this, &UChronosHUD::HandleGameStateChanged);
		
		GameMode->OnEnemyCountChanged.AddDynamic(this, &UChronosHUD::UpdateEnemyCount);
	}
}

void UChronosHUD::BindTimeDilationEvents()
{
	if (UTimeDilationSubsystem* TimeSubsystem = ChronosSubsystems::GetTimeDilationSubsystem(GetWorld()))
	{
		TimeSubsystem->OnTimeDilationChanged.AddDynamic(this, &UChronosHUD::UpdateTimeDilation);
	}
}

void UChronosHUD::BindWeaponEvents()
{
	APlayerCharacter* PlayerCharacter = nullptr;
	if (APlayerController* PlayerController = Cast<APlayerController>(GetOwningPlayer()))
	{
		PlayerCharacter = Cast<APlayerCharacter>(PlayerController->GetPawn());
	}
	
	// 绑定武器组件事件
	if (PlayerCharacter && PlayerCharacter->GetWeaponComponent())
	{
		UWeaponComponent* WeaponComponent = PlayerCharacter->GetWeaponComponent();
		
		WeaponComponent->OnWeaponChanged.AddDynamic(this, &UChronosHUD::OnWeaponChanged);
        
		// 更新初始弹药显示
		int32 RemainingAmmo = WeaponComponent->GetRemainingAmmo();
		int32 TotalAmmo = WeaponComponent->GetCurrentWeapon_Implementation() 
			? WeaponComponent->GetCurrentWeapon_Implementation()->GetTotalAmmo() 
			: 0;
		UpdateAmmo(RemainingAmmo, TotalAmmo);
	}
}

void UChronosHUD::HandleGameStateChanged(EGameState NewState)
{
	switch (NewState)
	{
	case EGameState::GameOver:
		ShowGameOver();
		break;
	case EGameState::Victory:
		ShowVictory();
		break;
	case EGameState::Playing:
		HideAll();
		break;
	default:
		break;
	}
}

void UChronosHUD::OnWeaponChanged(UWeaponDataAsset* NewWeapon)
{
	APlayerCharacter* PlayerCharacter = nullptr;
	if (APlayerController* PlayerController = Cast<APlayerController>(GetOwningPlayer()))
	{
		PlayerCharacter = Cast<APlayerCharacter>(PlayerController->GetPawn());
	}
	
	if (PlayerCharacter && PlayerCharacter->GetWeaponComponent())
	{
		UWeaponComponent* WeaponComponent = PlayerCharacter->GetWeaponComponent();
		
		int32 RemainingAmmo = WeaponComponent->GetRemainingAmmo();
		int32 TotalAmmo = NewWeapon ? NewWeapon->GetTotalAmmo() : 0;
		UpdateAmmo(RemainingAmmo, TotalAmmo);
	}
	else
	{
		UpdateAmmo(0, 0);
	}
}

void UChronosHUD::OnWeaponFired()
{
	APlayerCharacter* PlayerCharacter = nullptr;
	if (APlayerController* PlayerController = Cast<APlayerController>(GetOwningPlayer()))
	{
		PlayerCharacter = Cast<APlayerCharacter>(PlayerController->GetPawn());
	}
	
	if (PlayerCharacter && PlayerCharacter->GetWeaponComponent())
	{
		UWeaponComponent* WeaponComponent = PlayerCharacter->GetWeaponComponent();
		
		int32 RemainingAmmo = WeaponComponent->GetRemainingAmmo();
		int32 TotalAmmo = WeaponComponent->GetCurrentWeapon_Implementation() 
			? WeaponComponent->GetCurrentWeapon_Implementation()->GetTotalAmmo() 
			: 0;
		UpdateAmmo(RemainingAmmo, TotalAmmo);
	}
}
