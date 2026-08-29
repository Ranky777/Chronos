#include "Components/CombatComponent.h"

#include "Characters/ChronosCharacter.h"
#include "Weapons/ChronosWeapon.h"
#include "Weapons/WeaponUser.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UCombatComponent::EquipWeapon(TScriptInterface<IWeaponUser> NewWeapon)
{
	AChronosCharacter* Holder = Cast<AChronosCharacter>(GetOwner());
	if (!Holder || !NewWeapon.GetObject())
	{
		return;
	}

	if (CurrentWeapon == NewWeapon)
	{
		return;
	}

	TScriptInterface<IWeaponUser> OldWeapon = CurrentWeapon;
	CurrentWeapon = NewWeapon;

	// 单武器槽：新武器入手前先放下旧武器
	if (OldWeapon.GetObject())
	{
		IWeaponUser::Execute_DropWeapon(OldWeapon.GetObject());
	}

	// 订阅弹药变化（通过具体类型绑定，因为接口不带 UPROPERTY 委托）
	if (AChronosWeapon* ConcreteWeapon = Cast<AChronosWeapon>(CurrentWeapon.GetObject()))
	{
		if (!ConcreteWeapon->OnAmmoChanged.IsAlreadyBound(this, &UCombatComponent::HandleAmmoChanged))
		{
			ConcreteWeapon->OnAmmoChanged.AddDynamic(this, &UCombatComponent::HandleAmmoChanged);
		}
	}

	// 修复：接口只登记指针；"附加到手上 + 切换 Equipped 状态"必须由具体武器类执行，
	// 否则交互拾取后武器仍停留在世界状态（无碰撞表现错误且不能开火）
	if (AChronosWeapon* ConcreteWeapon = Cast<AChronosWeapon>(CurrentWeapon.GetObject()))
	{
		ConcreteWeapon->EquipTo(Holder);
	}

	OnWeaponChanged.Broadcast(OldWeapon, CurrentWeapon);
}

void UCombatComponent::ThrowCurrentWeapon(const FVector& Direction)
{
	AChronosCharacter* Holder = Cast<AChronosCharacter>(GetOwner());
	TScriptInterface<IWeaponUser> WeaponToThrow = CurrentWeapon;
	if (!Holder || !WeaponToThrow.GetObject())
	{
		return;
	}

	CurrentWeapon = nullptr;
	IWeaponUser::Execute_ThrowWeapon(WeaponToThrow.GetObject(), Direction);
	OnWeaponChanged.Broadcast(WeaponToThrow, nullptr);
}

void UCombatComponent::DropCurrentWeapon()
{
	TScriptInterface<IWeaponUser> WeaponToDrop = CurrentWeapon;
	if (!WeaponToDrop.GetObject())
	{
		return;
	}

	CurrentWeapon = nullptr;
	IWeaponUser::Execute_DropWeapon(WeaponToDrop.GetObject());
	OnWeaponChanged.Broadcast(WeaponToDrop, nullptr);
}

void UCombatComponent::HandleAmmoChanged(int32 RemainingAmmo, int32 MaxAmmo)
{
	// SUPERHOT 规则：弹药耗尽的武器自动向前投掷，空枪即攻击
	if (RemainingAmmo <= 0 && CurrentWeapon.GetObject())
	{
		if (AChronosCharacter* Holder = Cast<AChronosCharacter>(GetOwner()))
		{
			const FVector ThrowDirection = Holder->GetAimDirection();
			ThrowCurrentWeapon(ThrowDirection);
		}
	}
}