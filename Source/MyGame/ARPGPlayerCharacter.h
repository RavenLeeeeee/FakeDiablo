// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ARPGPlayerCharacter.generated.h"

class UCameraComponent;
class UARPGHealthComponent;
class USpringArmComponent;

/**
 * Minimal top-down ARPG player character prototype.
 */
UCLASS()
class AARPGPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AARPGPlayerCharacter();

	/** Applies normalized world-space movement input on the XY plane. */
	void MoveInWorldDirection(const FVector2D& MovementInput);

	/** Rotates the character yaw to face the supplied world-space point. */
	void FaceWorldPoint(const FVector& WorldPoint);

	void BasicAttack();
	void Dodge();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	UCameraComponent* TopDownCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	UARPGHealthComponent* HealthComponent;

public:
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetTopDownCamera() const { return TopDownCamera; }
	FORCEINLINE UARPGHealthComponent* GetHealthComponent() const { return HealthComponent; }
};
