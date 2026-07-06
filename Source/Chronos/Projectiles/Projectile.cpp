// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"

#include "Chronos/Chronos.h"
#include "Chronos/Interfaces/Damageable.h"
#include "Chronos/Subsystems/ProjectilePoolSubsystem.h"
#include "GameFramework/ProjectileMovementComponent.h"


// Sets default values
AProjectile::AProjectile()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->SetCollisionProfileName(TEXT("Projectile"));
	CollisionComponent->OnComponentHit.AddDynamic(this, &AProjectile::OnCollisionHit);
	
	RootComponent = CollisionComponent;
	
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	
	MovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComponent"));
	MovementComponent->SetIsReplicated(false);
	MovementComponent->bRotationFollowsVelocity = true;
	MovementComponent->bShouldBounce = false;
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	SpawnLocation = GetActorLocation();
}

void AProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (!bIsActive)
	{
		return;
	}
	
	float DistanceTraveled = FVector::Distance(GetActorLocation(), SpawnLocation);
	if (DistanceTraveled >= MaxDistance)
	{
		RecycleToPool();
	}
}

void AProjectile::Initialize(float Speed, AActor* OwnerActorParam)
{
	OwnerActor = OwnerActorParam;
	
	if (MovementComponent)
	{
		MovementComponent->InitialSpeed = Speed;
		MovementComponent->MaxSpeed = Speed;
	}
	
	SpawnLocation = GetActorLocation();
	bIsActive = true;
	
	SetActorHiddenInGame(false);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void AProjectile::RecycleToPool()
{
	bIsActive = false;
	
	if (UProjectilePoolSubsystem* PoolSubsystem = ChronosSubsystems::GetProjectilePoolSubsystem(GetWorld()))
	{
		PoolSubsystem->ReturnProjectile(this);
	}
}

void AProjectile::OnCollisionHit(class UPrimitiveComponent* HitComponent, class AActor* OtherActor,
	class UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!bIsActive || !OtherActor)
	{
		return;
	}
	
	if (OtherActor == OwnerActor)
	{
		return;
	}
	
	if (IDamageable* DamageableActor = Cast<IDamageable>(OtherActor))
	{
		DamageableActor->TakeDamage(DamageAmount, OwnerActor);
	}
	
	RecycleToPool();
}


