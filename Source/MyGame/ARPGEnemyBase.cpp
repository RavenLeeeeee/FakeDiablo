// Copyright Epic Games, Inc. All Rights Reserved.

#include "ARPGEnemyBase.h"
#include "ARPGHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MyGame.h"

AARPGEnemyBase::AARPGEnemyBase()
{
	PrimaryActorTick.bCanEverTick = false;

	HealthComponent = CreateDefaultSubobject<UARPGHealthComponent>(TEXT("HealthComponent"));

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->DisableMovement();
	}
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
