// Copyright Epic Games, Inc. All Rights Reserved.

#include "ARPGHealthComponent.h"
#include "MyGame.h"

UARPGHealthComponent::UARPGHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;

	MaxHealth = 100.f;
	CurrentHealth = MaxHealth;
	bIsDead = false;
	bIsInvincible = false;
	bHealthInitialized = false;
}

void UARPGHealthComponent::InitializeComponent()
{
	Super::InitializeComponent();

	InitializeHealth(false);
}

void UARPGHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeHealth(false);
}

void UARPGHealthComponent::InitializeHealth(bool bForceReset)
{
	AActor* OwnerActor = GetOwner();
	const FString OwnerName = OwnerActor ? OwnerActor->GetName() : TEXT("UnknownOwner");
	if (OwnerActor && (OwnerActor->IsTemplate() || OwnerName.StartsWith(TEXT("Default__"))))
	{
		UE_LOG(LogMyGame, Error, TEXT("%s Health initialization skipped on class default object"), *OwnerName);
		return;
	}

	if (bHealthInitialized && !bForceReset)
	{
		return;
	}

	MaxHealth = FMath::Max(MaxHealth, 1.f);
	CurrentHealth = MaxHealth;
	bIsDead = false;
	bIsInvincible = false;
	bHealthInitialized = true;

	UE_LOG(LogMyGame, Warning, TEXT("%s Health initialized: %.1f / %.1f"),
		*OwnerName,
		CurrentHealth,
		MaxHealth);
}

void UARPGHealthComponent::ApplyDamage(float DamageAmount)
{
	AActor* OwnerActor = GetOwner();
	const FString OwnerName = OwnerActor ? OwnerActor->GetName() : TEXT("UnknownOwner");
	UE_LOG(LogMyGame, Warning, TEXT("%s ApplyDamage requested: %.1f"), *OwnerName, DamageAmount);

	if (OwnerActor && (OwnerActor->IsTemplate() || OwnerName.StartsWith(TEXT("Default__"))))
	{
		UE_LOG(LogMyGame, Error, TEXT("%s Do not apply runtime damage to class default object"), *OwnerName);
		return;
	}

	if (!bHealthInitialized)
	{
		InitializeHealth(false);
	}

	if (DamageAmount <= 0.f)
	{
		return;
	}

	bIsDead = bHealthInitialized && CurrentHealth <= 0.f;
	if (bIsDead)
	{
		UE_LOG(LogMyGame, Warning, TEXT("%s ignored %.1f damage because already dead, HP: %.1f / %.1f"), *OwnerName, DamageAmount, CurrentHealth, MaxHealth);
		return;
	}

	if (bIsInvincible)
	{
		UE_LOG(LogMyGame, Warning, TEXT("%s damage ignored due to invincibility"), *OwnerName);
		return;
	}

	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Max(0.f, CurrentHealth - DamageAmount);
	bIsDead = CurrentHealth <= 0.f;

	UE_LOG(LogMyGame, Warning, TEXT("%s took %.1f damage, HP: %.1f -> %.1f / %.1f"), *OwnerName, DamageAmount, OldHealth, CurrentHealth, MaxHealth);
}

void UARPGHealthComponent::SetInvincible(bool bNewInvincible)
{
	bIsInvincible = bNewInvincible;
}
