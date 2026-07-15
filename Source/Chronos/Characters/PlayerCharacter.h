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
	APlayerCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	UFUNCTION(BlueprintCallable, Category = "Player")
	UHealthComponent* GetHealthComponent() const { return HealthComponent; }
	
	UFUNCTION(BlueprintCallable, Category = "Player")
	UWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }
	
	UFUNCTION(BlueprintCallable, Category = "Player")
	UCameraComponent* GetPlayerCamera() const { return CameraComponent; }
	
	UFUNCTION(BlueprintCallable, Category = "Player")
	bool IsAlive() const;
	
	UFUNCTION(BlueprintCallable, Category = "Player")
	void OnPlayerDeath(AActor* Killer);
	
	UFUNCTION(BlueprintCallable, Category = "Player")
	void RespawnPlayer();
	
protected:
	void HandleMoveInput(const FInputActionValue& Value);
	void HandleLookInput(const FInputActionValue& Value);
	void HandleFireInput();
	void HandleThrowInput();
	void BindDeathEvent();
	void UpdateInputActivity();
	
	void SetGameInputMode();
	void SetUIInputMode();
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWeaponComponent> WeaponComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> SpringArmComponent;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ThrowAction;
	
	UPROPERTY(EditAnywhere, Category = "Player", meta = (ClampMin = 0.1f, ClampMax = 10.0f))
	float MouseSensitivity = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Player")
	bool bInvertYAxis = false;

	UPROPERTY(EditAnywhere, Category = "Player", meta = (ClampMin = 0.0f, ClampMax = 180.0f))
	float MaxPitchAngle = 89.0f;
	
	float CurrentPitch = 0.0f;
	bool bHasInputActivity = false;
	bool bIsFiring = false;
	bool bIsDead = false;
};
