// Copyright Epic Games, Inc. All Rights Reserved.

#include "ARPGEnemyBase.h"
#include "ARPGHealthComponent.h"
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
	if (!HealthComponent || HealthComponent->IsDead())
	{
		return;
	}

	UE_LOG(LogMyGame, Log, TEXT("%s received hit for %.1f damage"), *GetName(), DamageAmount);
	HealthComponent->ApplyDamage(DamageAmount);

	if (HealthComponent->IsDead())
	{
		Die();
	}
}

void AARPGEnemyBase::Die()
{
	UE_LOG(LogMyGame, Log, TEXT("Enemy died: %s"), *GetName());

	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
}
