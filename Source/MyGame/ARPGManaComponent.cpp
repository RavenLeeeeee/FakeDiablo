// Copyright Epic Games, Inc. All Rights Reserved.

#include "ARPGManaComponent.h"
#include "MyGame.h"

UARPGManaComponent::UARPGManaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	MaxMana = 100.f;
	ManaRegenPerSecond = 3.f;
	CurrentMana = MaxMana;
	bCanRegenMana = true;
}

void UARPGManaComponent::BeginPlay()
{
	Super::BeginPlay();

	MaxMana = FMath::Max(MaxMana, 1.f);
	CurrentMana = MaxMana;
}

void UARPGManaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bCanRegenMana && CurrentMana < MaxMana)
	{
		CurrentMana = FMath::Min(MaxMana, CurrentMana + ManaRegenPerSecond * DeltaTime);
	}
}

bool UARPGManaComponent::ConsumeMana(float Amount)
{
	if (Amount <= 0.f)
	{
		return true;
	}

	if (CurrentMana < Amount)
	{
		return false;
	}

	CurrentMana = FMath::Max(0.f, CurrentMana - Amount);
	return true;
}

void UARPGManaComponent::RestoreMana(float Amount)
{
	if (Amount <= 0.f)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	const FString OwnerName = OwnerActor ? OwnerActor->GetName() : TEXT("UnknownOwner");
	const float OldMana = CurrentMana;
	CurrentMana = FMath::Clamp(CurrentMana + Amount, 0.f, MaxMana);

	UE_LOG(LogMyGame, Log, TEXT("%s restored mana: %.1f -> %.1f / %.1f"), *OwnerName, OldMana, CurrentMana, MaxMana);
}

void UARPGManaComponent::SetCanRegenMana(bool bNewCanRegenMana)
{
	bCanRegenMana = bNewCanRegenMana;
}

float UARPGManaComponent::GetManaPercent() const
{
	return MaxMana > 0.f ? FMath::Clamp(CurrentMana / MaxMana, 0.f, 1.f) : 0.f;
}
