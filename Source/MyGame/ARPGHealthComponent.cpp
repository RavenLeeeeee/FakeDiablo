// Copyright Epic Games, Inc. All Rights Reserved.

#include "ARPGHealthComponent.h"
#include "MyGame.h"

UARPGHealthComponent::UARPGHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UARPGHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	bIsDead = CurrentHealth <= 0.f;

	UE_LOG(LogMyGame, Log, TEXT("%s Health initialized: %.1f / %.1f"), *GetOwner()->GetName(), CurrentHealth, MaxHealth);
}

void UARPGHealthComponent::ApplyDamage(float DamageAmount)
{
	if (bIsDead || DamageAmount <= 0.f)
	{
		return;
	}

	CurrentHealth = FMath::Max(0.f, CurrentHealth - DamageAmount);
	bIsDead = CurrentHealth <= 0.f;

	UE_LOG(LogMyGame, Log, TEXT("%s took %.1f damage. Health: %.1f / %.1f"), *GetOwner()->GetName(), DamageAmount, CurrentHealth, MaxHealth);
}
