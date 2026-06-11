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

	UFUNCTION(BlueprintCallable, Category="Combat")
	bool IsDead() const { return bIsDead; }

protected:
	void PlayHitFeedback();
	void Die();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	UARPGHealthComponent* HealthComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Death")
	FRotator DeathMeshRotationOffset = FRotator(0.f, 0.f, 90.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Death")
	FVector DeathMeshLocationOffset = FVector(0.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Death")
	float CorpseLifeSpan = 10.f;

	bool bIsDead = false;
};
