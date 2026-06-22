// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ARPGManaComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYGAME_API UARPGManaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARPGManaComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category="Mana")
	bool ConsumeMana(float Amount);

	UFUNCTION(BlueprintCallable, Category="Mana")
	void RestoreMana(float Amount);

	UFUNCTION(BlueprintCallable, Category="Mana")
	void SetCanRegenMana(bool bNewCanRegenMana);

	UFUNCTION(BlueprintCallable, Category="Mana")
	bool CanRegenMana() const { return bCanRegenMana; }

	UFUNCTION(BlueprintCallable, Category="Mana")
	float GetCurrentMana() const { return CurrentMana; }

	UFUNCTION(BlueprintCallable, Category="Mana")
	float GetMaxMana() const { return MaxMana; }

	UFUNCTION(BlueprintCallable, Category="Mana")
	float GetManaPercent() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mana")
	float MaxMana = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mana")
	float ManaRegenPerSecond = 3.f;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category="Mana")
	float CurrentMana = 100.f;

private:
	bool bCanRegenMana = true;
};
