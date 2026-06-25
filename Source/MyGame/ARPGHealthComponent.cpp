// Copyright Epic Games, Inc. All Rights Reserved.

#include "ARPGHealthComponent.h"
#include "ARPGPlayerController.h"
#include "GameFramework/Pawn.h"
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

void UARPGHealthComponent::SetMaxHealth(float NewMaxHealth, bool bFillCurrentHealth)
{
	AActor* OwnerActor = GetOwner();
	const FString OwnerName = OwnerActor ? OwnerActor->GetName() : TEXT("UnknownOwner");
	if (OwnerActor && (OwnerActor->IsTemplate() || OwnerName.StartsWith(TEXT("Default__"))))
	{
		UE_LOG(LogMyGame, Error, TEXT("%s Health max set skipped on class default object"), *OwnerName);
		return;
	}

	MaxHealth = FMath::Max(1.f, NewMaxHealth);
	if (bFillCurrentHealth)
	{
		CurrentHealth = MaxHealth;
		bIsDead = false;
	}
	else
	{
		CurrentHealth = FMath::Clamp(CurrentHealth, 0.f, MaxHealth);
		bIsDead = CurrentHealth <= 0.f;
	}

	bHealthInitialized = true;
	UE_LOG(LogMyGame, Log, TEXT("Health max set: %s MaxHealth=%.1f"), *OwnerName, MaxHealth);
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

	float FinalDamage = DamageAmount;
	if (APawn* OwnerPawn = Cast<APawn>(OwnerActor))
	{
		if (AARPGPlayerController* ARPGPlayerController = Cast<AARPGPlayerController>(OwnerPawn->GetController()))
		{
			if (ARPGPlayerController->IsShocked())
			{
				FinalDamage *= ARPGPlayerController->GetDamageTakenMultiplier();
				UE_LOG(LogMyGame, Warning, TEXT("Player shocked damage amplified: %.1f -> %.1f"), DamageAmount, FinalDamage);
			}
		}
	}

	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Max(0.f, CurrentHealth - FinalDamage);
	bIsDead = CurrentHealth <= 0.f;

	UE_LOG(LogMyGame, Warning, TEXT("%s took %.1f damage, HP: %.1f -> %.1f / %.1f"), *OwnerName, FinalDamage, OldHealth, CurrentHealth, MaxHealth);
}

void UARPGHealthComponent::ApplyLethalDamageIgnoringInvincibility()
{
	AActor* OwnerActor = GetOwner();
	const FString OwnerName = OwnerActor ? OwnerActor->GetName() : TEXT("UnknownOwner");

	if (OwnerActor && (OwnerActor->IsTemplate() || OwnerName.StartsWith(TEXT("Default__"))))
	{
		UE_LOG(LogMyGame, Error, TEXT("%s Do not apply runtime lethal damage to class default object"), *OwnerName);
		return;
	}

	if (!bHealthInitialized)
	{
		InitializeHealth(false);
	}

	bIsDead = bHealthInitialized && CurrentHealth <= 0.f;
	if (bIsDead)
	{
		return;
	}

	const float OldHealth = CurrentHealth;
	CurrentHealth = 0.f;
	bIsDead = true;

	UE_LOG(LogMyGame, Warning, TEXT("%s lethal damage applied ignoring invincibility, HP: %.1f -> %.1f / %.1f"),
		*OwnerName,
		OldHealth,
		CurrentHealth,
		MaxHealth);
}

void UARPGHealthComponent::RestoreHealth(float Amount)
{
	if (Amount <= 0.f)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	const FString OwnerName = OwnerActor ? OwnerActor->GetName() : TEXT("UnknownOwner");
	if (OwnerActor && (OwnerActor->IsTemplate() || OwnerName.StartsWith(TEXT("Default__"))))
	{
		return;
	}

	if (!bHealthInitialized)
	{
		InitializeHealth(false);
	}

	if (IsDead())
	{
		return;
	}

	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.f, MaxHealth);
	bIsDead = CurrentHealth <= 0.f;

	UE_LOG(LogMyGame, Log, TEXT("%s restored health: %.1f -> %.1f / %.1f"), *OwnerName, OldHealth, CurrentHealth, MaxHealth);
}

void UARPGHealthComponent::SetInvincible(bool bNewInvincible)
{
	bIsInvincible = bNewInvincible;
}

float UARPGHealthComponent::GetHealthPercent() const
{
	return MaxHealth > 0.f ? FMath::Clamp(CurrentHealth / MaxHealth, 0.f, 1.f) : 0.f;
}
