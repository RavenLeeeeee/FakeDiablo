// Copyright Epic Games, Inc. All Rights Reserved.

#include "ARPGEnemyBase.h"
#include "ARPGHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "ARPGMissionManager.h"
#include "MyGame.h"

AARPGEnemyBase::AARPGEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	HealthComponent = CreateDefaultSubobject<UARPGHealthComponent>(TEXT("HealthComponent"));
}

void AARPGEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = EnemyMoveSpeed;
		MovementComponent->SetMovementMode(MOVE_Walking);
	}
}

void AARPGEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateSimpleAI(DeltaTime);
}

void AARPGEnemyBase::ApplyDamageToEnemy(float DamageAmount)
{
	ReceiveAttackHit(DamageAmount);
}

void AARPGEnemyBase::ReceiveAttackHit(float DamageAmount)
{
	if (bIsDead || !HealthComponent || HealthComponent->IsDead())
	{
		return;
	}

	UE_LOG(LogMyGame, Log, TEXT("Enemy hit: %s for %.1f damage"), *GetName(), DamageAmount);
	PlayHitFeedback();
	HealthComponent->ApplyDamage(DamageAmount);

	if (HealthComponent->IsDead())
	{
		Die();
	}
}

void AARPGEnemyBase::UpdateSimpleAI(float DeltaTime)
{
	if (bIsDead || !bEnableSimpleAI || !GetWorld())
	{
		return;
	}

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!PlayerPawn)
	{
		return;
	}

	if (bIsPreparingAttack)
	{
		AActor* FacingTarget = PendingAttackTarget.IsValid() ? PendingAttackTarget.Get() : PlayerPawn;
		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}

		if (FacingTarget)
		{
			FVector FacingDirection = FacingTarget->GetActorLocation() - GetActorLocation();
			FacingDirection.Z = 0.f;
			FaceDirection(FacingDirection, DeltaTime);
		}

		DrawEnemyAttackRangeDebug(0.05f);

		if (GetWorld()->GetTimeSeconds() >= EnemyAttackResolveTime)
		{
			ResolveEnemyAttack();
		}

		return;
	}

	FVector EnemyLocation = GetActorLocation();
	FVector PlayerLocation = PlayerPawn->GetActorLocation();
	EnemyLocation.Z = 0.f;
	PlayerLocation.Z = 0.f;

	const float DistanceToPlayer = FVector::Dist2D(EnemyLocation, PlayerLocation);
	if (DistanceToPlayer > AggroRange)
	{
		return;
	}

	FVector Direction = PlayerLocation - EnemyLocation;
	Direction.Z = 0.f;
	if (Direction.IsNearlyZero())
	{
		return;
	}

	const FVector MoveDirection = Direction.GetSafeNormal();
	if (DistanceToPlayer > AttackRange)
	{
		AddMovementInput(MoveDirection, 1.f);
		FaceDirection(MoveDirection, DeltaTime);
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	FaceDirection(MoveDirection, DeltaTime);
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastEnemyAttackTime >= EnemyAttackCooldown)
	{
		StartEnemyAttack(PlayerPawn, DeltaTime);
	}
}

void AARPGEnemyBase::StartEnemyAttack(AActor* TargetActor, float DeltaTime)
{
	if (bIsDead || bIsPreparingAttack || !TargetActor || !GetWorld())
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastEnemyAttackTime < EnemyAttackCooldown)
	{
		return;
	}

	LastEnemyAttackTime = CurrentTime;
	bIsPreparingAttack = true;
	PendingAttackTarget = TargetActor;
	EnemyAttackResolveTime = CurrentTime + EnemyAttackWindup;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	FVector AttackDirection = TargetActor->GetActorLocation() - GetActorLocation();
	AttackDirection.Z = 0.f;
	FaceDirection(AttackDirection, DeltaTime);

	UE_LOG(LogMyGame, Log, TEXT("Enemy attack windup started: %s"), *GetName());
	DrawEnemyAttackRangeDebug(EnemyAttackDebugDuration);
}

void AARPGEnemyBase::ResolveEnemyAttack()
{
	if (bIsDead)
	{
		bIsPreparingAttack = false;
		PendingAttackTarget = nullptr;
		return;
	}

	bIsPreparingAttack = false;

	AActor* TargetActor = PendingAttackTarget.Get();
	PendingAttackTarget = nullptr;
	if (!TargetActor)
	{
		return;
	}

	FVector EnemyLocation = GetActorLocation();
	FVector TargetLocation = TargetActor->GetActorLocation();
	EnemyLocation.Z = 0.f;
	TargetLocation.Z = 0.f;

	const float DistanceToTarget = FVector::Dist2D(EnemyLocation, TargetLocation);
	FVector AttackDirection = TargetLocation - EnemyLocation;
	AttackDirection.Z = 0.f;
	FaceDirection(AttackDirection, 0.1f);

	if (DistanceToTarget > AttackRange + 30.f)
	{
		UE_LOG(LogMyGame, Log, TEXT("Enemy attack missed: %s"), *GetName());
		return;
	}

	UARPGHealthComponent* PlayerHealthComponent = TargetActor->FindComponentByClass<UARPGHealthComponent>();
	if (!PlayerHealthComponent || PlayerHealthComponent->IsDead())
	{
		UE_LOG(LogMyGame, Log, TEXT("Enemy attack missed: %s"), *GetName());
		return;
	}

	PlayerHealthComponent->ApplyDamage(EnemyAttackDamage);
	UE_LOG(LogMyGame, Log, TEXT("Enemy attack hit player: %s"), *GetName());
	UE_LOG(LogMyGame, Log, TEXT("Player took damage: %.1f"), EnemyAttackDamage);
	DrawDebugSphere(GetWorld(), TargetActor->GetActorLocation(), 55.f, 16, FColor::Red, false, EnemyAttackDebugDuration);

	if (PlayerHealthComponent->IsDead())
	{
		UE_LOG(LogMyGame, Log, TEXT("Player died"));
	}
}

void AARPGEnemyBase::DrawEnemyAttackRangeDebug(float Duration) const
{
	if (GetWorld())
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), AttackRange, 32, FColor::Orange, false, Duration, 0, 2.f);
	}
}

void AARPGEnemyBase::FaceDirection(const FVector& Direction, float DeltaTime)
{
	FVector FlatDirection(Direction.X, Direction.Y, 0.f);
	if (FlatDirection.IsNearlyZero())
	{
		return;
	}

	const FRotator TargetRotation(0.f, FlatDirection.Rotation().Yaw, 0.f);
	const FRotator SmoothedRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, EnemyFacingInterpSpeed);
	SetActorRotation(FRotator(0.f, SmoothedRotation.Yaw, 0.f));
}

void AARPGEnemyBase::PlayHitFeedback()
{
	if (GetWorld())
	{
		DrawDebugSphere(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 80.f), 40.f, 16, FColor::Orange, false, 1.f);
	}
}

void AARPGEnemyBase::Die()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	bIsPreparingAttack = false;
	PendingAttackTarget = nullptr;
	UE_LOG(LogMyGame, Log, TEXT("Enemy died: %s"), *GetName());

	TArray<AActor*> MissionManagers;
	UGameplayStatics::GetAllActorsOfClass(this, AARPGMissionManager::StaticClass(), MissionManagers);
	AARPGMissionManager* MissionManager = nullptr;
	for (AActor* MissionManagerActor : MissionManagers)
	{
		MissionManager = Cast<AARPGMissionManager>(MissionManagerActor);
		if (MissionManager)
		{
			break;
		}
	}

	if (MissionManager)
	{
		MissionManager->NotifyEnemyKilled(this);
	}
	else
	{
		UE_LOG(LogMyGame, Warning, TEXT("MissionManager not found"));
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (USkeletalMeshComponent* MeshComponent = GetMesh())
	{
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->SetRelativeRotation(MeshComponent->GetRelativeRotation() + DeathMeshRotationOffset);
		MeshComponent->AddLocalOffset(DeathMeshLocationOffset);
	}

	SetLifeSpan(CorpseLifeSpan);
}
