// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ARPGBasicEquipmentTypes.generated.h"

UENUM(BlueprintType)
enum class EARPGEquipmentSlot : uint8
{
	Weapon,
	Helmet,
	Armor,
	Legs,
	Boots,
	Amulet,
	Ring1,
	Ring2
};

USTRUCT(BlueprintType)
struct FARPGSimpleEquipmentItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EARPGEquipmentSlot Slot = EARPGEquipmentSlot::Weapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DamageMultiplierBonus = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHealthBonus = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MoveSpeedBonus = 0.f;
};
