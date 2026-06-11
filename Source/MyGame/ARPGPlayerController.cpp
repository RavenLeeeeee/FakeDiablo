// Copyright Epic Games, Inc. All Rights Reserved.

#include "ARPGPlayerController.h"
#include "ARPGPlayerCharacter.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputCoreTypes.h"
#include "MyGame.h"

AARPGPlayerController::AARPGPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AARPGPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}

void AARPGPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AARPGPlayerController::HandleLeftClickPressed);

	UE_LOG(LogMyGame, Log, TEXT("AARPGPlayerController SetupInputComponent"));
}

void AARPGPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	const FVector2D KeyboardMovementInput = GetKeyboardMovementInput();

	AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter();
	if (ARPGCharacter && !KeyboardMovementInput.IsNearlyZero())
	{
		bHasClickMoveTarget = false;
		StopARPGCharacterMovement();
		ARPGCharacter->MoveInWorldDirection(KeyboardMovementInput.GetSafeNormal());

		UpdateMouseFacing();
		UpdateActionInput();
		return;
	}

	if (bHasClickMoveTarget)
	{
		UpdateClickMoveMovement();
	}

	UpdateMouseFacing();
	UpdateActionInput();
}

FVector2D AARPGPlayerController::GetKeyboardMovementInput() const
{
	FVector2D MovementInput = FVector2D::ZeroVector;

	if (IsInputKeyDown(EKeys::W))
	{
		MovementInput.X += 1.f;
	}
	if (IsInputKeyDown(EKeys::S))
	{
		MovementInput.X -= 1.f;
	}
	if (IsInputKeyDown(EKeys::D))
	{
		MovementInput.Y += 1.f;
	}
	if (IsInputKeyDown(EKeys::A))
	{
		MovementInput.Y -= 1.f;
	}

	return MovementInput;
}

void AARPGPlayerController::HandleLeftClickPressed()
{
	FVector WorldOrigin;
	FVector WorldDirection;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		UE_LOG(LogMyGame, Warning, TEXT("ClickMove hit failed"));
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ARPGClickMoveTrace), false);
	QueryParams.AddIgnoredActor(GetPawn());

	FHitResult HitResult;
	const FVector TraceEnd = WorldOrigin + (WorldDirection * 100000.f);
	if (GetWorld() && GetWorld()->LineTraceSingleByChannel(HitResult, WorldOrigin, TraceEnd, ECC_Visibility, QueryParams))
	{
		ClickMoveTarget = HitResult.ImpactPoint;
		bHasClickMoveTarget = true;
		UE_LOG(LogMyGame, Log, TEXT("ClickMove target updated: %s"), *ClickMoveTarget.ToString());
		return;
	}

	UE_LOG(LogMyGame, Warning, TEXT("ClickMove hit failed"));
}

void AARPGPlayerController::UpdateClickMoveMovement()
{
	AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter();
	if (!ARPGCharacter)
	{
		bHasClickMoveTarget = false;
		return;
	}

	const FVector PawnLocation = ARPGCharacter->GetActorLocation();
	FVector Direction = ClickMoveTarget - PawnLocation;
	Direction.Z = 0.f;

	if (Direction.Size2D() <= ClickMoveAcceptanceRadius)
	{
		bHasClickMoveTarget = false;
		StopARPGCharacterMovement();
		return;
	}

	ARPGCharacter->MoveInWorldDirection(FVector2D(Direction.X, Direction.Y).GetSafeNormal());
}

void AARPGPlayerController::StopARPGCharacterMovement()
{
	if (AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter())
	{
		if (UCharacterMovementComponent* MovementComponent = ARPGCharacter->GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}
	}
}

void AARPGPlayerController::UpdateMouseFacing()
{
	AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter();
	if (!ARPGCharacter)
	{
		return;
	}

	FHitResult HitResult;
	if (GetCursorWorldHit(HitResult))
	{
		ARPGCharacter->FaceWorldPoint(HitResult.ImpactPoint);
	}
}

void AARPGPlayerController::UpdateActionInput()
{
	AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter();
	if (!ARPGCharacter)
	{
		return;
	}

	const bool bIsBasicAttackPressed = IsInputKeyDown(EKeys::RightMouseButton);
	if (bIsBasicAttackPressed && !bWasBasicAttackPressed)
	{
		ARPGCharacter->BasicAttack();
	}
	bWasBasicAttackPressed = bIsBasicAttackPressed;

	const bool bIsDodgePressed = IsInputKeyDown(EKeys::SpaceBar);
	if (bIsDodgePressed && !bWasDodgePressed)
	{
		ARPGCharacter->Dodge();
	}
	bWasDodgePressed = bIsDodgePressed;
}

bool AARPGPlayerController::GetCursorWorldHit(FHitResult& OutHitResult)
{
	FHitResult HitResult;
	if (GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), false, HitResult))
	{
		if (HitResult.GetActor() != GetPawn())
		{
			OutHitResult = HitResult;
			return true;
		}
	}

	FVector WorldLocation;
	FVector WorldDirection;
	if (!DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ARPGCursorTrace), false);
	QueryParams.AddIgnoredActor(GetPawn());

	const FVector TraceEnd = WorldLocation + (WorldDirection * 100000.f);
	if (GetWorld() && GetWorld()->LineTraceSingleByChannel(OutHitResult, WorldLocation, TraceEnd, ECC_Visibility, QueryParams))
	{
		return true;
	}

	return false;
}

AARPGPlayerCharacter* AARPGPlayerController::GetARPGCharacter() const
{
	return Cast<AARPGPlayerCharacter>(GetPawn());
}
