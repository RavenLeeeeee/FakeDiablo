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

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category="Combat")
	void ApplyDamageToEnemy(float DamageAmount);

	UFUNCTION(BlueprintCallable, Category="Combat")
	void ReceiveAttackHit(float DamageAmount);

	UFUNCTION(BlueprintCallable, Category="Combat")
	UARPGHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UFUNCTION(BlueprintCallable, Category="Combat")
	bool IsDead() const { return bIsDead; }

protected:
	void UpdateSimpleAI(float DeltaTime);
	void StartEnemyAttack(AActor* TargetActor, float DeltaTime);
	void ResolveEnemyAttack();
	void DrawEnemyAttackRangeDebug(float Duration) const;
	void FaceDirection(const FVector& Direction, float DeltaTime);
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
	bool bEnableSimpleAI = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
	float AggroRange = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
	float AttackRange = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
	float EnemyMoveSpeed = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
	float EnemyAttackDamage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
	float EnemyAttackCooldown = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
	float EnemyAttackWindup = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
	float EnemyAttackDebugDuration = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
	float EnemyFacingInterpSpeed = 8.f;

	bool bIsDead = false;
	bool bIsPreparingAttack = false;
	float EnemyAttackResolveTime = 0.f;
	float LastEnemyAttackTime = -999.f;
	TWeakObjectPtr<AActor> PendingAttackTarget;
};
