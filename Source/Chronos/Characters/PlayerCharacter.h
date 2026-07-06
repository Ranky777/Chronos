// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PlayerCharacter.generated.h"

struct FInputActionValue;
class UWeaponComponent;
class UHealthComponent;
class UInputMappingContext;
class UInputAction;
class UCameraComponent;
class USpringArmComponent;

UCLASS()
class CHRONOS_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APlayerCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	/**
	 * 获取生命值组件
	 * @return 生命值组件指针
	 */
	UFUNCTION(BlueprintCallable, Category = "Player")
	UHealthComponent* GetHealthComponent() const { return HealthComponent; }
	
	/**
	 * 获取武器组件
	 * @return 武器组件指针
	 */
	UFUNCTION(BlueprintCallable, Category = "Player")
	UWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }
	
	/**
	* 获取玩家相机
	* @return 相机组件指针
	*/
	UFUNCTION(BlueprintCallable, Category = "Player")
	UCameraComponent* GetPlayerCamera() const { return CameraComponent; }
	
	/**
	 * 检查玩家是否存活
	 * @return 如果存活返回true，否则返回false
	 */
	UFUNCTION(BlueprintCallable, Category = "Player")
	bool IsAlive() const;
	
	/**
	 * 玩家死亡处理
	 * 禁用输入、停止武器系统、通知游戏模式
	 * @param Killer 杀死玩家的Actor
	 */
	UFUNCTION(BlueprintCallable, Category = "Player")
	void OnPlayerDeath(AActor* Killer);
	
	/**
	 * 重生玩家
	 * 恢复生命值、重置武器状态、启用输入
	 */
	UFUNCTION(BlueprintCallable, Category = "Player")
	void RespawnPlayer();
	
protected:
	/**
	 * 移动输入处理
	 * 根据输入向量移动玩家角色
	 * @param Value 移动输入值（Axis2D）
	 */
	void HandleMoveInput(const FInputActionValue& Value);
	
	/**
	* 视角输入处理
	* 根据输入向量旋转玩家和相机
	* @param Value 视角输入值（Axis2D）
	*/
	void HandleLookInput(const FInputActionValue& Value);
	
	/**
	* 射击输入处理
	* 触发武器射击
	*/
	void HandleFireInput();

	/**
	 * 换弹输入处理
	 * 触发武器换弹
	 */
	void HandleReloadInput();

	/**
	 * 绑定死亡事件
	 * 将HealthComponent的OnDeath事件绑定到OnPlayerDeath方法
	 */
	void BindDeathEvent();

	/**
	 * 更新输入活动状态
	 * 将玩家的输入状态通知给TimeDilationSubsystem
	 */
	void UpdateInputActivity();
	
private:
	/** 生命值组件 - 管理玩家的生命状态 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHealthComponent> HealthComponent;

	/** 武器组件 - 管理玩家的武器装备和射击 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWeaponComponent> WeaponComponent;

	/** 相机组件 - 提供第一人称视角 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> CameraComponent;

	/** 弹簧臂组件 - 用于相机跟随 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> SpringArmComponent;
	
	/** 输入映射上下文 - 定义输入动作到按键的映射 */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	/** 移动输入动作 */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	/** 视角输入动作 */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	/** 射击输入动作 */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> FireAction;

	/** 换弹输入动作 */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ReloadAction;
	
	/** 鼠标灵敏度 - 控制视角旋转速度 */
	UPROPERTY(EditAnywhere, Category = "Player", meta = (ClampMin = 0.1f, ClampMax = 10.0f))
	float MouseSensitivity = 1.0f;

	/** 是否反转Y轴 - 控制鼠标上下移动的方向 */
	UPROPERTY(EditAnywhere, Category = "Player")
	bool bInvertYAxis = false;

	/** 最大视角俯仰角度 - 防止视角过度旋转 */
	UPROPERTY(EditAnywhere, Category = "Player", meta = (ClampMin = 0.0f, ClampMax = 180.0f))
	float MaxPitchAngle = 89.0f;
	
	/** 当前视角俯仰角度 */
	float CurrentPitch = 0.0f;

	/** 是否有输入活动 */
	bool bHasInputActivity = false;

	/** 是否正在射击 */
	bool bIsFiring = false;

	/** 是否正在换弹 */
	bool bIsReloading = false;

	/** 是否已死亡 */
	bool bIsDead = false;
};
