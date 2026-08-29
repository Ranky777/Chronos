#pragma once

#include "InputMappingContext.h"
#include "GameFramework/Character.h"
#include "Weapons/WeaponUser.h"
#include "ChronosCharacter.generated.h"

class AChronosWeapon;
class UCameraComponent;
class UCombatComponent;
class UHealthComponent;
class USkeletalMeshComponent;
class UInputAction;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractionFocusChanged, bool, bHasFocus, FText, InteractionText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterDeath, AChronosCharacter*, Character);

/**
 * 玩家与敌人共用的第一人称角色基类。
 *
 * 职责边界（C++ 系统 / 蓝图表现）：
 * - C++：Enhanced Input 绑定、时间上报、瞄准射线、开火/投掷/交互/近战的流程控制；
 * - 蓝图：网格、相机、动画等表现配置（模版 BP_FirstPersonCharacter 重父类化到本类）。
 *
 * 每个输入处理函数的第一行都调用时间子系统的 NotifyPlayerInput()，
 * 这是 SUPERHOT"有输入才推进时间"机制的唯一入口。
 */
UCLASS(Abstract)
class CHRONOS_NEW_API AChronosCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AChronosCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;

	//~ 输入动作资产，由蓝图配置
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chronos|Input")
	TObjectPtr<const UInputAction> IA_Move;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chronos|Input")
	TObjectPtr<const UInputAction> IA_Look;

	/** 鼠标视角动作（模版的 IA_MouseLook 已含正确的 Negate 配置），与 IA_Look（手柄）等价处理 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chronos|Input")
	TObjectPtr<const UInputAction> IA_LookMouse;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chronos|Input")
	TObjectPtr<const UInputAction> IA_Jump;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chronos|Input")
	TObjectPtr<const UInputAction> IA_Fire;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chronos|Input")
	TObjectPtr<const UInputAction> IA_Throw;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chronos|Input")
	TObjectPtr<const UInputAction> IA_Interact;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Chronos|Input", meta = (ClampMin = "0.01"))
	float MouseSensitivity = 1.f;

	/**
	 * 本角色需要的输入映射上下文列表，BeginPlay 时按 MappingPriority 依次推送。
	 * 输入上下文跟随 Pawn 而非 Controller，保证任何控制器下移动/视角都可用。
	 * 在蓝图默认值中配置资产引用，保持"C++ 系统 / 蓝图配置"的分工
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chronos|Input")
	TArray<TSoftObjectPtr<const UInputMappingContext>> DefaultMappingContexts;

	UFUNCTION(BlueprintPure, Category = "Chronos|Combat")
	UCombatComponent* GetCombatComponent() const { return CombatComponent; }

	UFUNCTION(BlueprintPure, Category = "Chronos|Health")
	UHealthComponent* GetHealthComponent() const { return HealthComponent; }

	/** 武器应附加到的骨骼网格组件：优先第一人称手臂网格 */
	UFUNCTION(BlueprintPure, Category = "Chronos|Combat")
	USkeletalMeshComponent* GetHeldWeaponAttachComponent() const;

	/** 武器附加插槽名 */
	FName GetHeldWeaponSocketName() const;

	/** 当前视线方向（相机朝向） */
	UFUNCTION(BlueprintPure, Category = "Chronos|Aim")
	FVector GetAimDirection() const;

	/** 视线落点：沿视线做距离 100m 的射线检测，未命中返回视线远端点 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Aim")
	FVector GetAimTargetPoint() const;

	UPROPERTY(BlueprintAssignable, Category = "Chronos|Interaction")
	FOnInteractionFocusChanged OnInteractionFocusChanged;

	UPROPERTY(BlueprintAssignable, Category = "Chronos|Health")
	FOnCharacterDeath OnCharacterDeath;

protected:
	virtual void BeginPlay() override;

	//~ 输入处理。全部先上报时间活动再执行具体逻辑
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnJumpStarted(const FInputActionValue& Value);
	void OnJumpEnded(const FInputActionValue& Value);
	void OnFireStarted(const FInputActionValue& Value);
	void OnThrowStarted(const FInputActionValue& Value);
	void OnInteractStarted(const FInputActionValue& Value);

	UFUNCTION()
	void HandleDeath(AActor* DamagedActor, AActor* Killer);

	void UpdateInteractionFocus();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCombatComponent> CombatComponent;

private:
	UCameraComponent* CachedCamera = nullptr;

	/** 当前准星焦点交互物（弱引用，防止悬停对象被销毁后悬挂） */
	TWeakObjectPtr<UObject> FocusedInteractable;
};
