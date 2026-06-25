// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ARPGHealthComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYGAME_API UARPGHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARPGHealthComponent();

	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category="Health")
	void InitializeHealth(bool bForceReset = false);

	UFUNCTION(BlueprintCallable, Category="Health")
	void SetMaxHealth(float NewMaxHealth, bool bFillCurrentHealth);

	UFUNCTION(BlueprintCallable, Category="Health")
	void ApplyDamage(float DamageAmount);

	UFUNCTION(BlueprintCallable, Category="Health")
	void ApplyLethalDamageIgnoringInvincibility();

	UFUNCTION(BlueprintCallable, Category="Health")
	void RestoreHealth(float Amount);

	UFUNCTION(BlueprintCallable, Category="Health")
	void SetInvincible(bool bNewInvincible);

	UFUNCTION(BlueprintCallable, Category="Health")
	bool IsInvincible() const { return bIsInvincible; }

	UFUNCTION(BlueprintCallable, Category="Health")
	bool IsDead() const { return bHealthInitialized && CurrentHealth <= 0.f; }

	UFUNCTION(BlueprintCallable, Category="Health")
	bool IsHealthInitialized() const { return bHealthInitialized; }

	UFUNCTION(BlueprintCallable, Category="Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintCallable, Category="Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintCallable, Category="Health")
	float GetHealthPercent() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health")
	float MaxHealth = 100.f;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category="Health")
	float CurrentHealth = 100.f;

private:
	bool bIsDead = false;
	bool bIsInvincible = false;
	bool bHealthInitialized = false;
};
