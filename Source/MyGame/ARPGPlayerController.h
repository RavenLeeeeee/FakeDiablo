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

private:
	FVector2D GetKeyboardMovementInput() const;
	void HandleLeftClickPressed();
	void UpdateClickMoveMovement(float DeltaTime);
	void StopARPGCharacterMovement();
	void SmoothFaceDirection(const FVector& Direction, float DeltaTime);
	void UpdateMouseFacing(float DeltaTime);
	void UpdateActionInput();
	bool GetCursorWorldHit(FHitResult& OutHitResult);

	AARPGPlayerCharacter* GetARPGCharacter() const;

	FVector ClickMoveTarget = FVector::ZeroVector;
	bool bHasClickMoveTarget = false;
	float ClickMoveAcceptanceRadius = 35.f;
	float FacingInterpSpeed = 12.f;
	uint64 LastHandledLeftClickFrame = 0;

	bool bWasBasicAttackPressed = false;
	bool bWasDodgePressed = false;
};
