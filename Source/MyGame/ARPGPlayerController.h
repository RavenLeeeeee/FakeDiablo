// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ARPGPlayerController.generated.h"

class AARPGPlayerCharacter;

/**
 * Minimal top-down ARPG player controller prototype.
 */
UCLASS()
class AARPGPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AARPGPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dodge")
	float DodgeDistance = 520.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dodge")
	float DodgeDuration = 0.34f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dodge")
	float DodgeCooldown = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill")
	float AreaSkillRadius = 260.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill")
	float AreaSkillDamage = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill")
	float AreaSkillCooldown = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill")
	float AreaSkillCastLockDuration = 0.35f;

private:
	FVector2D GetKeyboardMovementInput() const;
	void HandleLeftClickPressed();
	void HandleDodgePressed();
	void HandleAreaSkillPressed();
	void HandleDodgeTick(float DeltaTime);
	void EndDodge();
	FVector GetCurrentDodgeDirection();
	void UpdateClickMoveMovement(float DeltaTime);
	void StopARPGCharacterMovement();
	void SmoothFaceDirection(const FVector& Direction, float DeltaTime);
	void UpdateMouseFacing(float DeltaTime);
	void UpdateActionInput();
	void PerformBasicAttack();
	FVector GetBasicAttackDirection(const AARPGPlayerCharacter* ARPGCharacter) const;
	FVector GetAreaSkillCenter(const AARPGPlayerCharacter* ARPGCharacter);
	void PerformAreaSkill(const FVector& AreaCenter);
	bool GetCursorWorldHit(FHitResult& OutHitResult);

	AARPGPlayerCharacter* GetARPGCharacter() const;

	FVector ClickMoveTarget = FVector::ZeroVector;
	bool bHasClickMoveTarget = false;
	float ClickMoveAcceptanceRadius = 35.f;
	float FacingInterpSpeed = 12.f;
	float BasicAttackRange = 220.f;
	float BasicAttackRadius = 80.f;
	float BasicAttackDamage = 25.f;
	float BasicAttackCooldown = 0.6f;
	float BasicAttackLockDuration = 0.35f;
	float LastBasicAttackTime = -1000.f;
	float BasicAttackLockEndTime = 0.f;
	uint64 LastHandledLeftClickFrame = 0;

	bool bIsDodging = false;
	float DodgeStartTime = 0.f;
	float DodgeEndTime = 0.f;
	float LastDodgeTime = -999.f;
	float ActiveDodgeSpeed = 0.f;
	FVector DodgeDirection = FVector::ZeroVector;
	ECollisionResponse SavedPawnCollisionResponse = ECR_Block;
	bool bSavedPawnCollisionResponseValid = false;

	bool bIsBasicAttackLocked = false;
	bool bWasBasicAttackPressed = false;

	bool bIsAreaSkillCasting = false;
	float AreaSkillCastEndTime = 0.f;
	float LastAreaSkillTime = -999.f;
};
