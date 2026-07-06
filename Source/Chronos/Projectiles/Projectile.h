// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Projectile.generated.h"

UCLASS()
class CHRONOS_API AProjectile : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AProjectile();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void Initialize(float Speed, AActor* OwnerActorParam);
	
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void RecycleToPool();
	
private:
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void OnCollisionHit(class UPrimitiveComponent* HitComponent, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
private:
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;
	
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> MeshComponent;
	
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> MovementComponent;
	
	UPROPERTY(EditAnywhere, Category = "Projectile")
	float DamageAmount = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Projectile")
	float MaxDistance = 2000.0f;
	
	TObjectPtr<AActor> OwnerActor;
	FVector SpawnLocation;
	bool bIsActive = false;
};
