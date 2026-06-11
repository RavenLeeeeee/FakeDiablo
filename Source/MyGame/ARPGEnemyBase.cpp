// Copyright Epic Games, Inc. All Rights Reserved.

#include "ARPGEnemyBase.h"
#include "ARPGHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
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
	TryAttackPlayer(PlayerPawn);
}

void AARPGEnemyBase::TryAttackPlayer(AActor* PlayerActor)
{
	if (bIsDead || !PlayerActor || !GetWorld())
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastEnemyAttackTime < EnemyAttackCooldown)
	{
		return;
	}

	UARPGHealthComponent* PlayerHealthComponent = PlayerActor->FindComponentByClass<UARPGHealthComponent>();
	if (!PlayerHealthComponent || PlayerHealthComponent->IsDead())
	{
		return;
	}

	LastEnemyAttackTime = CurrentTime;
	UE_LOG(LogMyGame, Log, TEXT("Enemy attack player: %s"), *GetName());

	PlayerHealthComponent->ApplyDamage(EnemyAttackDamage);
	UE_LOG(LogMyGame, Log, TEXT("Player took damage: %.1f"), EnemyAttackDamage);
	if (PlayerHealthComponent->IsDead())
	{
		UE_LOG(LogMyGame, Log, TEXT("Player died"));
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
	UE_LOG(LogMyGame, Log, TEXT("Enemy died: %s"), *GetName());

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
