#include "Projectiles/Projectile.h"

#include "Components/HealthComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Subsystems/ProjectilePoolSubsystem.h"
#include "Subsystems/TimeDilationSubsystem.h"

AChronosProjectile::AChronosProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(6.f);
	// 复用项目配置中的 "Projectile" 碰撞预设（ECC_GameTraceChannel1）
	CollisionComponent->SetCollisionProfileName(TEXT("Projectile"));
	CollisionComponent->OnComponentHit.AddDynamic(this, &AChronosProjectile::OnHit);
	RootComponent = CollisionComponent;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->bAutoActivate = false;
	ProjectileMovement->InitialSpeed = BulletSpeed;
	ProjectileMovement->MaxSpeed = BulletSpeed;
	// SUPERHOT 子弹直线飞行，冻结时悬停在空中
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->OnProjectileStop.AddDynamic(this, &AChronosProjectile::OnProjectileStop);

	TrailComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailComponent"));
	TrailComponent->SetupAttachment(RootComponent);
	TrailComponent->bAutoActivate = false;

	bActive = 0;
}

void AChronosProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (TrailSystem)
	{
		TrailComponent->SetAsset(TrailSystem);
	}
	else
	{
		TrailComponent->Deactivate();
	}
}

void AChronosProjectile::ActivateProjectile(AActor* NewInstigator, const FVector& Location, const FRotator& Rotation)
{
	SetActorLocationAndRotation(Location, Rotation);

	// Instigator 必须是 Pawn（伤害来源），Owner 可以是任意 Actor
	if (APawn* PawnInstigator = Cast<APawn>(NewInstigator))
	{
		SetInstigator(PawnInstigator);
	}
	SetOwner(NewInstigator);

	// 清空上一次发射的忽略列表后忽略本次发射者，避免长期累积
	CollisionComponent->ClearMoveIgnoreActors();
	CollisionComponent->IgnoreActorWhenMoving(NewInstigator, true);

	ProjectileMovement->InitialSpeed = BulletSpeed;
	ProjectileMovement->MaxSpeed = BulletSpeed;
	ProjectileMovement->Velocity = Rotation.Vector() * BulletSpeed;

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	CollisionComponent->SetCollisionProfileName(TEXT("Projectile"));

	if (TrailSystem)
	{
		TrailComponent->Activate(true);
	}

	bActive = 1;
	ProjectileMovement->Activate(true);

	if (UTimeDilationSubsystem* Time = GetWorld()->GetSubsystem<UTimeDilationSubsystem>())
	{
		if (!Time->OnDilationChanged.IsAlreadyBound(this, &AChronosProjectile::HandleTimeDilationChanged))
		{
			Time->OnDilationChanged.AddDynamic(this, &AChronosProjectile::HandleTimeDilationChanged);
		}
		HandleTimeDilationChanged(Time->GetCurrentDilation());
	}
}

void AChronosProjectile::DeactivateProjectile()
{
	if (!bActive)
	{
		return;
	}

	bActive = 0;
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->Deactivate();
	TrailComponent->Deactivate();
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	OnProjectileDeactivated.Broadcast(this);
}

void AChronosProjectile::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UTimeDilationSubsystem* Time = GetWorld()->GetSubsystem<UTimeDilationSubsystem>())
	{
		Time->OnDilationChanged.RemoveDynamic(this, &AChronosProjectile::HandleTimeDilationChanged);
	}
	Super::EndPlay(Reason);
}

void AChronosProjectile::HandleTimeDilationChanged(float NewDilation)
{
	// 慢门（值越小越慢）时尾迹更亮，强化"时间凝固的子弹"读感
	TrailComponent->SetVariableFloat(TEXT("User.TimeDilation"), NewDilation);
}

void AChronosProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	HandleImpact(OtherActor, OtherComp, Hit);
}

void AChronosProjectile::OnProjectileStop(const FHitResult& Impact)
{
	HandleImpact(Impact.GetActor(), Impact.GetComponent(), Impact);
}

void AChronosProjectile::HandleImpact(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FHitResult& Hit)
{
	if (!bActive || !OtherActor || OtherActor == GetInstigator())
	{
		return;
	}

	// 一击必杀：命中任何持有生命组件的实体直接致命
	if (UHealthComponent* TargetHealth = OtherActor->FindComponentByClass<UHealthComponent>())
	{
		TargetHealth->TakeDamage(GetInstigator());
	}

	// 对可击碎的物理体施加冲量，增强打击感
	if (OtherComp && OtherComp->IsSimulatingPhysics())
	{
		OtherComp->AddImpulseAtLocation(GetVelocity().GetClampedToMaxSize(DamageImpulseStrength), Hit.ImpactPoint, Hit.BoneName);
	}

	ReturnToPool();
}

void AChronosProjectile::ReturnToPool()
{
	if (UWorld* World = GetWorld())
	{
		if (UChronosProjectilePoolSubsystem* Pool = World->GetSubsystem<UChronosProjectilePoolSubsystem>())
		{
			Pool->ReturnProjectile(this);
			return;
		}
	}

	// 池子系统不存在（例如已退出 PIE），直接隐藏即可
	DeactivateProjectile();
}
