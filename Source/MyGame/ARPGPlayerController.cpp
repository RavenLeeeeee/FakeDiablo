// Copyright Epic Games, Inc. All Rights Reserved.

#include "ARPGPlayerController.h"
#include "ARPGEnemyBase.h"
#include "ARPGPlayerCharacter.h"
#include "DrawDebugHelpers.h"
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
	InputMode.SetConsumeCaptureMouseDown(false);
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

	AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter();
	if (!ARPGCharacter)
	{
		UpdateActionInput();
		return;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (bIsBasicAttackLocked)
	{
		UpdateActionInput();

		if (CurrentTime >= BasicAttackLockEndTime)
		{
			bIsBasicAttackLocked = false;
			UE_LOG(LogMyGame, Log, TEXT("BasicAttack movement lock ended"));
		}

		return;
	}

	if (WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		HandleLeftClickPressed();
	}

	const FVector2D KeyboardMovementInput = GetKeyboardMovementInput();
	if (ARPGCharacter && !KeyboardMovementInput.IsNearlyZero())
	{
		if (bHasClickMoveTarget)
		{
			bHasClickMoveTarget = false;
			StopARPGCharacterMovement();
			UE_LOG(LogTemp, Warning, TEXT("[ClickMove] Cancelled by WASD"));
		}

		const FVector WASDDirection = FVector(KeyboardMovementInput.X, KeyboardMovementInput.Y, 0.f).GetSafeNormal();
		ARPGCharacter->AddMovementInput(WASDDirection, 1.f);
		SmoothFaceDirection(WASDDirection, DeltaTime);

		UpdateActionInput();
		return;
	}

	if (bHasClickMoveTarget)
	{
		UpdateClickMoveMovement(DeltaTime);
		UpdateActionInput();
		return;
	}

	UpdateMouseFacing(DeltaTime);
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
	if (LastHandledLeftClickFrame == GFrameCounter)
	{
		return;
	}
	LastHandledLeftClickFrame = GFrameCounter;

	UE_LOG(LogTemp, Warning, TEXT("[ClickMove] Pressed frame=%llu"), static_cast<unsigned long long>(GFrameCounter));

	FVector WorldOrigin;
	FVector WorldDirection;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ClickMove] ClickMove hit failed"));
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
		DrawDebugSphere(GetWorld(), ClickMoveTarget, 20.f, 16, FColor::Green, false, 1.f);
		UE_LOG(LogTemp, Warning, TEXT("[ClickMove] Target refreshed: %s"), *ClickMoveTarget.ToString());
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[ClickMove] ClickMove hit failed"));
}

void AARPGPlayerController::UpdateClickMoveMovement(float DeltaTime)
{
	AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter();
	if (!ARPGCharacter)
	{
		bHasClickMoveTarget = false;
		return;
	}

	FVector PawnLocation = ARPGCharacter->GetActorLocation();
	FVector TargetLocation = ClickMoveTarget;
	PawnLocation.Z = 0.f;
	TargetLocation.Z = 0.f;

	const float DistanceToTarget = FVector::Dist2D(PawnLocation, TargetLocation);
	FVector Direction = ClickMoveTarget - PawnLocation;
	Direction.Z = 0.f;

	if (DistanceToTarget <= ClickMoveAcceptanceRadius)
	{
		bHasClickMoveTarget = false;
		StopARPGCharacterMovement();
		UE_LOG(LogTemp, Warning, TEXT("[ClickMove] Arrived"));
		return;
	}

	const FVector MoveDirection = Direction.GetSafeNormal();
	ARPGCharacter->AddMovementInput(MoveDirection, 1.f);
	SmoothFaceDirection(MoveDirection, DeltaTime);
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

void AARPGPlayerController::SmoothFaceDirection(const FVector& Direction, float DeltaTime)
{
	AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter();
	if (!ARPGCharacter)
	{
		return;
	}

	FVector FlatDirection(Direction.X, Direction.Y, 0.f);
	if (FlatDirection.IsNearlyZero())
	{
		return;
	}

	const FRotator NewRotation = FlatDirection.Rotation();
	const FRotator TargetRotation(0.f, NewRotation.Yaw, 0.f);
	const FRotator CurrentRotation = ARPGCharacter->GetActorRotation();
	const FRotator SmoothedRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, FacingInterpSpeed);
	ARPGCharacter->SetActorRotation(FRotator(0.f, SmoothedRotation.Yaw, 0.f));
}

void AARPGPlayerController::UpdateMouseFacing(float DeltaTime)
{
	AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter();
	if (!ARPGCharacter)
	{
		return;
	}

	FHitResult HitResult;
	if (GetCursorWorldHit(HitResult))
	{
		const FVector ToMouse = HitResult.ImpactPoint - ARPGCharacter->GetActorLocation();
		SmoothFaceDirection(ToMouse, DeltaTime);
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
		const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		if (CurrentTime - LastBasicAttackTime < BasicAttackCooldown)
		{
			UE_LOG(LogMyGame, Log, TEXT("BasicAttack on cooldown"));
		}
		else
		{
			LastBasicAttackTime = CurrentTime;
			bHasClickMoveTarget = false;
			ClickMoveTarget = FVector::ZeroVector;
			StopARPGCharacterMovement();
			bIsBasicAttackLocked = true;
			BasicAttackLockEndTime = CurrentTime + BasicAttackLockDuration;
			ARPGCharacter->BasicAttack();
			PerformBasicAttack();
		}
	}
	bWasBasicAttackPressed = bIsBasicAttackPressed;

	const bool bIsDodgePressed = IsInputKeyDown(EKeys::SpaceBar);
	if (bIsDodgePressed && !bWasDodgePressed)
	{
		ARPGCharacter->Dodge();
	}
	bWasDodgePressed = bIsDodgePressed;
}

void AARPGPlayerController::PerformBasicAttack()
{
	AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter();
	if (!ARPGCharacter || !GetWorld())
	{
		UE_LOG(LogMyGame, Log, TEXT("BasicAttack missed"));
		return;
	}

	const FVector AttackDirection = GetBasicAttackDirection(ARPGCharacter);
	SmoothFaceDirection(AttackDirection, 0.1f);

	const FVector PlayerLocation = ARPGCharacter->GetActorLocation();
	const FVector Start = PlayerLocation + (AttackDirection * 60.f);
	const FVector End = PlayerLocation + (AttackDirection * BasicAttackRange);
	const FCollisionShape AttackShape = FCollisionShape::MakeSphere(BasicAttackRadius);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ARPGBasicAttackSweep), false);
	QueryParams.AddIgnoredActor(ARPGCharacter);

	TArray<FHitResult> HitResults;
	const bool bHit = GetWorld()->SweepMultiByChannel(HitResults, Start, End, FQuat::Identity, ECC_Pawn, AttackShape, QueryParams);

	DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1.f, 0, 2.f);
	DrawDebugSphere(GetWorld(), Start, BasicAttackRadius, 16, FColor::Red, false, 1.f);
	DrawDebugSphere(GetWorld(), End, BasicAttackRadius, 16, FColor::Red, false, 1.f);

	TSet<AARPGEnemyBase*> HitEnemies;
	if (bHit)
	{
		for (const FHitResult& HitResult : HitResults)
		{
			AARPGEnemyBase* Enemy = Cast<AARPGEnemyBase>(HitResult.GetActor());
			if (!Enemy || Enemy->IsDead() || HitEnemies.Contains(Enemy))
			{
				continue;
			}

			HitEnemies.Add(Enemy);
			DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 24.f, 12, FColor::Yellow, false, 1.f);
			UE_LOG(LogMyGame, Log, TEXT("BasicAttack hit enemy: %s"), *Enemy->GetName());
			Enemy->ApplyDamageToEnemy(BasicAttackDamage);
		}
	}

	if (HitEnemies.Num() == 0)
	{
		UE_LOG(LogMyGame, Log, TEXT("BasicAttack missed"));
	}
}

FVector AARPGPlayerController::GetBasicAttackDirection(const AARPGPlayerCharacter* ARPGCharacter) const
{
	if (!ARPGCharacter)
	{
		return FVector::ForwardVector;
	}

	FVector WorldOrigin;
	FVector WorldDirection;
	if (DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ARPGBasicAttackMouseTrace), false);
		QueryParams.AddIgnoredActor(GetPawn());

		FHitResult HitResult;
		const FVector TraceEnd = WorldOrigin + (WorldDirection * 100000.f);
		if (GetWorld() && GetWorld()->LineTraceSingleByChannel(HitResult, WorldOrigin, TraceEnd, ECC_Visibility, QueryParams))
		{
			FVector AttackDirection = HitResult.ImpactPoint - ARPGCharacter->GetActorLocation();
			AttackDirection.Z = 0.f;
			if (!AttackDirection.IsNearlyZero())
			{
				return AttackDirection.GetSafeNormal();
			}
		}
	}

	FVector ForwardDirection = ARPGCharacter->GetActorForwardVector();
	ForwardDirection.Z = 0.f;
	return ForwardDirection.IsNearlyZero() ? FVector::ForwardVector : ForwardDirection.GetSafeNormal();
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
