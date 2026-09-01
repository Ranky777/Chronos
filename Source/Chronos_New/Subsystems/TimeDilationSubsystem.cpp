#include "Subsystems/TimeDilationSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"

UTimeDilationSubsystem::UTimeDilationSubsystem()
	: bPlayerDead(0)
{
}

void UTimeDilationSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	const double Now = FPlatformTime::Seconds();
	if (!bInitialized)
	{
		LastRealTime = Now;
		// LastInputRealTime 保持构造初值 0：PIE 开局视为"从未输入"，直接落入慢门
		//（SUPERHOT 语义：世界等你动）。CurrentDilation 构造值为 1.f，
		// 必须在此显式 ApplyDilation 才能把 SlowDilation 写进 WorldSettings 并广播。
		ApplyDilation(SlowDilation);
		bInitialized = true;
		return;
	}

	// 使用真实时间差做插值：时间膨胀会缩短游戏帧时长，
	// 若直接用 DilatedDelta 插值，减速时恢复速度也会被同步拖慢
	float RealDelta = static_cast<float>(Now - LastRealTime);
	LastRealTime = Now;
	RealDelta = FMath::Clamp(RealDelta, 0.f, 0.1f);

	float TargetDilation;
	if (bPlayerDead)
	{
		// 玩家死亡：时间恢复正常，保证死亡表现与重开流程不被拖慢
		KillOverrideRemaining = 0.f;
		TargetDilation = 1.f;
	}
	else if (KillOverrideRemaining > 0.f)
	{
		// 击杀子弹时间优先于普通输入判定
		KillOverrideRemaining -= RealDelta;
		TargetDilation = KillOverrideDilation;
	}
	else if (Now - LastInputRealTime <= InputActiveWindow)
	{
		TargetDilation = 1.f;
	}
	else
	{
		TargetDilation = SlowDilation;
	}

	const float SpeedTo = (TargetDilation > CurrentDilation) ? (1.f / TimeToSpeedUp) : (1.f / TimeToSlow);
	const float NewDilation = FMath::FInterpConstantTo(CurrentDilation, TargetDilation, RealDelta, SpeedTo);
	ApplyDilation(NewDilation);
}

void UTimeDilationSubsystem::ApplyDilation(float NewDilation)
{
	if (FMath::IsNearlyEqual(CurrentDilation, NewDilation, KINDA_SMALL_NUMBER))
	{
		return;
	}

	CurrentDilation = NewDilation;

	if (UWorld* World = GetWorld())
	{
		if (AWorldSettings* Settings = World->GetWorldSettings())
		{
			Settings->SetTimeDilation(CurrentDilation);
		}
	}

	OnDilationChanged.Broadcast(CurrentDilation);
}

void UTimeDilationSubsystem::NotifyPlayerInput()
{
	LastInputRealTime = FPlatformTime::Seconds();
}

void UTimeDilationSubsystem::SetPlayerDead(bool bDead)
{
	bPlayerDead = bDead ? 1 : 0;
	if (bDead)
	{
		LastInputRealTime = 0.0;
	}
}

void UTimeDilationSubsystem::TriggerKillOverride(float Dilation, float Duration)
{
	KillOverrideDilation = Dilation;
	KillOverrideRemaining = Duration;
}

TStatId UTimeDilationSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTimeDilationSubsystem, STATGROUP_Tickables);
}

bool UTimeDilationSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}
