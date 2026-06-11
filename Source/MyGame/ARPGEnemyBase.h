// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ARPGEnemyBase.generated.h"

class UARPGHealthComponent;

UCLASS(Blueprintable)
class MYGAME_API AARPGEnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	AARPGEnemyBase();

	UFUNCTION(BlueprintCallable, Category="Combat")
	void ApplyDamageToEnemy(float DamageAmount);

	UFUNCTION(BlueprintCallable, Category="Combat")
	void ReceiveAttackHit(float DamageAmount);

	UFUNCTION(BlueprintCallable, Category="Combat")
	UARPGHealthComponent* GetHealthComponent() const { return HealthComponent; }

protected:
	void Die();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	UARPGHealthComponent* HealthComponent;
};
