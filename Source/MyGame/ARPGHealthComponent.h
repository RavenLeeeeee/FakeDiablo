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

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category="Health")
	void ApplyDamage(float DamageAmount);

	UFUNCTION(BlueprintCallable, Category="Health")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintCallable, Category="Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Health")
	float CurrentHealth = 100.f;

private:
	bool bIsDead = false;
};
