// Copyright Epic Games, Inc. All Rights Reserved.

#include "ARPGPlayerController.h"
#include "ARPGEnemyBase.h"
#include "ARPGHealthComponent.h"
#include "ARPGManaComponent.h"
#include "ARPGPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"
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
	InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AARPGPlayerController::HandleDodgePressed);
	InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AARPGPlayerController::HandleAreaSkillPressed);
	InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AARPGPlayerController::HandleEmpowerPressed);
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AARPGPlayerController::HandleWhirlwindPressed);
	InputComponent->BindKey(EKeys::T, IE_Pressed, this, &AARPGPlayerController::HandlePiercingSkillPressed);
	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AARPGPlayerController::HandleHealthPotionPressed);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AARPGPlayerController::HandleManaPotionPressed);

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
	if (bIsEmpowered)
	{
		UARPGHealthComponent* HealthComponent = ARPGCharacter->GetHealthComponent();
		if ((HealthComponent && HealthComponent->IsDead()) || CurrentTime >= EmpowerEndTime)
		{
			EndEmpower();
		}
	}

	if (bIsWhirlwinding)
	{
		UARPGHealthComponent* HealthComponent = ARPGCharacter->GetHealthComponent();
		if (HealthComponent && HealthComponent->IsDead())
		{
			StopWhirlwind(TEXT("Player died"));
		}
		else
		{
			HandleWhirlwindTick(DeltaTime);
		}
	}

	if (bIsDodging)
	{
		HandleDodgeTick(DeltaTime);
		return;
	}

	if (bIsAreaSkillCasting)
	{
		bWasBasicAttackPressed = IsInputKeyDown(EKeys::RightMouseButton);

		if (CurrentTime >= AreaSkillCastEndTime)
		{
			bIsAreaSkillCasting = false;
			UE_LOG(LogMyGame, Log, TEXT("AreaSkill cast ended"));
		}
		else
		{
			return;
		}
	}

	if (bIsPiercingSkillCasting)
	{
		bWasBasicAttackPressed = IsInputKeyDown(EKeys::RightMouseButton);

		if (CurrentTime >= PiercingSkillCastEndTime)
		{
			bIsPiercingSkillCasting = false;
			UE_LOG(LogMyGame, Log, TEXT("PiercingSkill cast ended"));
		}
		else
		{
			return;
		}
	}

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
	if (bIsDodging || bIsAreaSkillCasting || bIsPiercingSkillCasting)
	{
		return;
	}

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

void AARPGPlayerController::HandleDodgePressed()
{
	AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter();
	if (!ARPGCharacter || !GetWorld())
	{
		return;
	}

	if (bIsDodging)
	{
		return;
	}

	if (bIsAreaSkillCasting)
	{
		return;
	}

	if (bIsPiercingSkillCasting)
	{
		return;
	}

	if (bIsWhirlwinding)
	{
		UE_LOG(LogMyGame, Warning, TEXT("Dodge blocked during Whirlwind"));
		return;
	}

	if (bIsBasicAttackLocked)
	{
		UE_LOG(LogMyGame, Log, TEXT("Dodge blocked during BasicAttack"));
		return;
	}

	UARPGHealthComponent* HealthComponent = ARPGCharacter->GetHealthComponent();
	if (HealthComponent && HealthComponent->IsDead())
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime < LastDodgeTime + DodgeCooldown)
	{
		UE_LOG(LogMyGame, Log, TEXT("Dodge on cooldown"));
		return;
	}

	DodgeDirection = GetCurrentDodgeDirection();
	if (DodgeDirection.IsNearlyZero())
	{
		DodgeDirection = ARPGCharacter->GetActorForwardVector();
		DodgeDirection.Z = 0.f;
		DodgeDirection = DodgeDirection.IsNearlyZero() ? FVector::ForwardVector : DodgeDirection.GetSafeNormal();
	}

	UCharacterMovementComponent* MovementComponent = ARPGCharacter->GetCharacterMovement();
	ActiveDodgeSpeed = DodgeDuration > 0.f ? DodgeDistance / DodgeDuration : DodgeDistance;
	if (MovementComponent)
	{
		MovementComponent->StopMovementImmediately();
	}

	bIsDodging = true;
	DodgeStartTime = CurrentTime;
	DodgeEndTime = CurrentTime + DodgeDuration;
	LastDodgeTime = CurrentTime;

	bHasClickMoveTarget = false;
	ClickMoveTarget = FVector::ZeroVector;

	if (UCapsuleComponent* Capsule = ARPGCharacter->GetCapsuleComponent())
	{
		SavedPawnCollisionResponse = Capsule->GetCollisionResponseToChannel(ECC_Pawn);
		bSavedPawnCollisionResponseValid = true;
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		UE_LOG(LogMyGame, Log, TEXT("Dodge collision ignore Pawn enabled"));
	}

	if (HealthComponent)
	{
		HealthComponent->SetInvincible(true);
	}

	ARPGCharacter->Dodge();
	SmoothFaceDirection(DodgeDirection, 0.1f);
	UE_LOG(LogMyGame, Log, TEXT("Dodge started"));
	UE_LOG(LogMyGame, Log, TEXT("DodgeDirection: %s"), *DodgeDirection.ToString());
	UE_LOG(LogMyGame, Log, TEXT("DodgeDistance: %.1f"), DodgeDistance);
	UE_LOG(LogMyGame, Log, TEXT("DodgeDuration: %.2f"), DodgeDuration);
	UE_LOG(LogMyGame, Log, TEXT("DodgeSpeed: %.1f"), ActiveDodgeSpeed);
}

void AARPGPlayerController::HandleAreaSkillPressed()
{
	AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter();
	if (!ARPGCharacter || !GetWorld())
	{
		return;
	}

	UARPGHealthComponent* HealthComponent = ARPGCharacter->GetHealthComponent();
	if (HealthComponent && HealthComponent->IsDead())
	{
		return;
	}

	if (bIsDodging)
	{
		UE_LOG(LogMyGame, Log, TEXT("AreaSkill blocked during Dodge"));
		return;
	}

	if (bIsBasicAttackLocked)
	{
		UE_LOG(LogMyGame, Log, TEXT("AreaSkill blocked during BasicAttack"));
		return;
	}

	if (bIsPiercingSkillCasting)
	{
		return;
	}

	if (bIsWhirlwinding)
	{
		UE_LOG(LogMyGame, Warning, TEXT("AreaSkill blocked during Whirlwind"));
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime < LastAreaSkillTime + AreaSkillCooldown)
	{
		UE_LOG(LogMyGame, Log, TEXT("AreaSkill on cooldown"));
		return;
	}

	if (!ConsumeMana(AreaSkillManaCost))
	{
		UE_LOG(LogMyGame, Log, TEXT("Not enough mana for AreaSkill"));
		return;
	}

	const FVector AreaCenter = GetAreaSkillCenter(ARPGCharacter);
	const FVector FaceDirection = AreaCenter - ARPGCharacter->GetActorLocation();

	bHasClickMoveTarget = false;
	ClickMoveTarget = FVector::ZeroVector;
	StopARPGCharacterMovement();

	bIsAreaSkillCasting = true;
	AreaSkillCastEndTime = CurrentTime + AreaSkillCastLockDuration;
	LastAreaSkillTime = CurrentTime;

	SmoothFaceDirection(FaceDirection, 0.1f);
	UE_LOG(LogMyGame, Log, TEXT("AreaSkill cast started"));
	PerformAreaSkill(AreaCenter);
}

void AARPGPlayerController::HandleEmpowerPressed()
{
	AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter();
	if (!ARPGCharacter || !GetWorld())
	{
		return;
	}

	UARPGHealthComponent* HealthComponent = ARPGCharacter->GetHealthComponent();
	if (HealthComponent && HealthComponent->IsDead())
	{
		return;
	}

	if (bIsEmpowered)
	{
		UE_LOG(LogMyGame, Log, TEXT("Empower already active"));
		return;
	}

	if (bIsDodging)
	{
		UE_LOG(LogMyGame, Log, TEXT("Empower blocked during Dodge"));
		return;
	}

	if (bIsAreaSkillCasting)
	{
		UE_LOG(LogMyGame, Log, TEXT("Empower blocked during AreaSkill"));
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime < LastEmpowerTime + EmpowerCooldown)
	{
		UE_LOG(LogMyGame, Log, TEXT("Empower on cooldown"));
		return;
	}

	if (!ConsumeMana(EmpowerManaCost))
	{
		UE_LOG(LogMyGame, Log, TEXT("Not enough mana for Empower"));
		return;
	}

	StartEmpower();
}

void AARPGPlayerController::HandleWhirlwindPressed()
{
	if (bIsPiercingSkillCasting)
	{
		return;
	}

	if (bIsWhirlwinding)
	{
		StopWhirlwind(TEXT("Toggled off"));
		return;
	}

	StartWhirlwind();
}

void AARPGPlayerController::HandlePiercingSkillPressed()
{
	AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter();
	if (!ARPGCharacter || !GetWorld())
	{
		return;
	}

	UARPGHealthComponent* HealthComponent = ARPGCharacter->GetHealthComponent();
	if (HealthComponent && HealthComponent->IsDead())
	{
		return;
	}

	if (bIsDodging)
	{
		UE_LOG(LogMyGame, Log, TEXT("PiercingSkill blocked during Dodge"));
		return;
	}

	if (bIsBasicAttackLocked)
	{
		UE_LOG(LogMyGame, Log, TEXT("PiercingSkill blocked during BasicAttack"));
		return;
	}

	if (bIsAreaSkillCasting)
	{
		UE_LOG(LogMyGame, Log, TEXT("PiercingSkill blocked during AreaSkill"));
		return;
	}

	if (bIsWhirlwinding)
	{
		UE_LOG(LogMyGame, Log, TEXT("PiercingSkill blocked during Whirlwind"));
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime < LastPiercingSkillTime + PiercingSkillCooldown)
	{
		UE_LOG(LogMyGame, Log, TEXT("PiercingSkill on cooldown"));
		return;
	}

	if (!ConsumeMana(PiercingSkillManaCost))
	{
		UE_LOG(LogMyGame, Log, TEXT("Not enough mana for PiercingSkill"));
		return;
	}

	const FVector Direction = GetPiercingSkillDirection(ARPGCharacter);

	bHasClickMoveTarget = false;
	ClickMoveTarget = FVector::ZeroVector;
	StopARPGCharacterMovement();

	bIsPiercingSkillCasting = true;
	PiercingSkillCastEndTime = CurrentTime + PiercingSkillCastLockDuration;
	LastPiercingSkillTime = CurrentTime;

	SmoothFaceDirection(Direction, 0.1f);
	UE_LOG(LogMyGame, Log, TEXT("PiercingSkill cast started"));
	PerformPiercingSkill(Direction);
}

void AARPGPlayerController::HandleHealthPotionPressed()
{
	AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter();
	if (!ARPGCharacter)
	{
		return;
	}

	UARPGHealthComponent* HealthComponent = ARPGCharacter->GetHealthComponent();
	if (!HealthComponent || HealthComponent->IsDead())
	{
		return;
	}

	if (HealthPotionCount <= 0)
	{
		UE_LOG(LogMyGame, Log, TEXT("No health potion left"));
		return;
	}

	if (HealthComponent->GetCurrentHealth() >= HealthComponent->GetMaxHealth())
	{
		UE_LOG(LogMyGame, Log, TEXT("Health already full"));
		return;
	}

	const float RestoreAmount = HealthComponent->GetMaxHealth() * HealthPotionRestorePercent;
	HealthComponent->RestoreHealth(RestoreAmount);
	--HealthPotionCount;

	UE_LOG(LogMyGame, Log, TEXT("Used health potion, remaining: %d"), HealthPotionCount);
}

void AARPGPlayerController::HandleManaPotionPressed()
{
	AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter();
	if (!ARPGCharacter)
	{
		return;
	}

	UARPGHealthComponent* HealthComponent = ARPGCharacter->GetHealthComponent();
	if (HealthComponent && HealthComponent->IsDead())
	{
		return;
	}

	UARPGManaComponent* ManaComponent = GetManaComponent();
	if (!ManaComponent)
	{
		return;
	}

	if (ManaPotionCount <= 0)
	{
		UE_LOG(LogMyGame, Log, TEXT("No mana potion left"));
		return;
	}

	if (ManaComponent->GetCurrentMana() >= ManaComponent->GetMaxMana())
	{
		UE_LOG(LogMyGame, Log, TEXT("Mana already full"));
		return;
	}

	const float RestoreAmount = ManaComponent->GetMaxMana() * ManaPotionRestorePercent;
	ManaComponent->RestoreMana(RestoreAmount);
	--ManaPotionCount;

	UE_LOG(LogMyGame, Log, TEXT("Used mana potion, remaining: %d"), ManaPotionCount);
}

void AARPGPlayerController::StartEmpower()
{
	AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter();
	if (!ARPGCharacter || !GetWorld())
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = ARPGCharacter->GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	bIsEmpowered = true;
	EmpowerEndTime = CurrentTime + EmpowerDuration;
	LastEmpowerTime = CurrentTime;
	SavedEmpowerMaxWalkSpeed = MovementComponent->MaxWalkSpeed;
	MovementComponent->MaxWalkSpeed = SavedEmpowerMaxWalkSpeed + EmpowerMoveSpeedBonus;

	UE_LOG(LogMyGame, Log, TEXT("Empower started"));
}

void AARPGPlayerController::EndEmpower()
{
	if (!bIsEmpowered)
	{
		return;
	}

	bIsEmpowered = false;

	if (AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter())
	{
		if (UCharacterMovementComponent* MovementComponent = ARPGCharacter->GetCharacterMovement())
		{
			MovementComponent->MaxWalkSpeed = SavedEmpowerMaxWalkSpeed;
		}
	}

	UE_LOG(LogMyGame, Log, TEXT("Empower ended"));
}

void AARPGPlayerController::StartWhirlwind()
{
	AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter();
	if (!ARPGCharacter || !GetWorld())
	{
		return;
	}

	UARPGHealthComponent* HealthComponent = ARPGCharacter->GetHealthComponent();
	if (HealthComponent && HealthComponent->IsDead())
	{
		return;
	}

	if (bIsDodging)
	{
		UE_LOG(LogMyGame, Log, TEXT("Whirlwind blocked during Dodge"));
		return;
	}

	if (bIsBasicAttackLocked)
	{
		UE_LOG(LogMyGame, Log, TEXT("Whirlwind blocked during BasicAttack"));
		return;
	}

	if (bIsAreaSkillCasting)
	{
		UE_LOG(LogMyGame, Log, TEXT("Whirlwind blocked during AreaSkill"));
		return;
	}

	UARPGManaComponent* ManaComponent = GetManaComponent();
	if (!ManaComponent || ManaComponent->GetCurrentMana() < WhirlwindStartManaCost)
	{
		UE_LOG(LogMyGame, Log, TEXT("Not enough mana for Whirlwind"));
		return;
	}

	ManaComponent->ConsumeMana(WhirlwindStartManaCost);

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	bIsWhirlwinding = true;
	LastWhirlwindHitTime = CurrentTime - WhirlwindHitInterval;

	ManaComponent->SetCanRegenMana(false);

	UE_LOG(LogMyGame, Log, TEXT("Whirlwind started"));
}

void AARPGPlayerController::StopWhirlwind(const FString& Reason)
{
	if (!bIsWhirlwinding)
	{
		return;
	}

	bIsWhirlwinding = false;
	if (UARPGManaComponent* ManaComponent = GetManaComponent())
	{
		ManaComponent->SetCanRegenMana(true);
	}

	UE_LOG(LogMyGame, Log, TEXT("Whirlwind stopped: %s"), *Reason);
}

void AARPGPlayerController::HandleWhirlwindTick(float DeltaTime)
{
	if (!GetWorld())
	{
		return;
	}

	const float ManaCostThisTick = WhirlwindManaCostPerSecond * DeltaTime;
	if (!ConsumeMana(ManaCostThisTick))
	{
		StopWhirlwind(TEXT("Out of mana"));
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime >= LastWhirlwindHitTime + WhirlwindHitInterval)
	{
		LastWhirlwindHitTime = CurrentTime;
		PerformWhirlwindHit();
	}
}

void AARPGPlayerController::PerformWhirlwindHit()
{
	AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter();
	if (!ARPGCharacter || !GetWorld())
	{
		return;
	}

	const FVector Center = ARPGCharacter->GetActorLocation();
	const FCollisionShape WhirlwindShape = FCollisionShape::MakeSphere(WhirlwindRadius);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ARPGWhirlwindOverlap), false);
	QueryParams.AddIgnoredActor(ARPGCharacter);

	TArray<FOverlapResult> OverlapResults;
	const bool bHit = GetWorld()->OverlapMultiByChannel(OverlapResults, Center, FQuat::Identity, ECC_Pawn, WhirlwindShape, QueryParams);

	DrawDebugSphere(GetWorld(), Center, WhirlwindRadius, 32, FColor::Orange, false, WhirlwindHitInterval, 0, 2.f);

	if (!bHit)
	{
		return;
	}

	TSet<AARPGEnemyBase*> HitEnemies;
	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AARPGEnemyBase* Enemy = Cast<AARPGEnemyBase>(OverlapResult.GetActor());
		if (!Enemy || Enemy->IsDead() || HitEnemies.Contains(Enemy))
		{
			continue;
		}

		HitEnemies.Add(Enemy);
		const float FinalDamage = bIsEmpowered ? WhirlwindDamage * EmpowerDamageMultiplier : WhirlwindDamage;
		UE_LOG(LogMyGame, Log, TEXT("Whirlwind hit enemy: %s"), *Enemy->GetName());
		Enemy->ReceiveAttackHit(FinalDamage);
	}
}

bool AARPGPlayerController::ConsumeMana(float ManaCost)
{
	UARPGManaComponent* ManaComponent = GetManaComponent();
	return ManaComponent ? ManaComponent->ConsumeMana(ManaCost) : false;
}

void AARPGPlayerController::HandleDodgeTick(float DeltaTime)
{
	AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter();
	if (!ARPGCharacter || !GetWorld())
	{
		EndDodge();
		return;
	}

	if (GetWorld()->GetTimeSeconds() >= DodgeEndTime)
	{
		EndDodge();
		return;
	}

	const FVector DeltaMove = DodgeDirection * ActiveDodgeSpeed * DeltaTime;
	ARPGCharacter->AddActorWorldOffset(DeltaMove, true);
	SmoothFaceDirection(DodgeDirection, DeltaTime);
}

void AARPGPlayerController::EndDodge()
{
	if (!bIsDodging)
	{
		return;
	}

	bIsDodging = false;
	DodgeDirection = FVector::ZeroVector;

	if (AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter())
	{
		if (UCapsuleComponent* Capsule = ARPGCharacter->GetCapsuleComponent())
		{
			if (bSavedPawnCollisionResponseValid)
			{
				Capsule->SetCollisionResponseToChannel(ECC_Pawn, SavedPawnCollisionResponse);
				bSavedPawnCollisionResponseValid = false;
				UE_LOG(LogMyGame, Log, TEXT("Dodge collision restored"));
			}
		}

		if (UCharacterMovementComponent* MovementComponent = ARPGCharacter->GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}

		if (UARPGHealthComponent* HealthComponent = ARPGCharacter->GetHealthComponent())
		{
			HealthComponent->SetInvincible(false);
		}
	}

	bSavedPawnCollisionResponseValid = false;
	ActiveDodgeSpeed = 0.f;
	UE_LOG(LogMyGame, Log, TEXT("Dodge ended"));
}

FVector AARPGPlayerController::GetCurrentDodgeDirection()
{
	const AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter();
	if (!ARPGCharacter)
	{
		return FVector::ForwardVector;
	}

	const FVector2D KeyboardMovementInput = GetKeyboardMovementInput();
	if (!KeyboardMovementInput.IsNearlyZero())
	{
		return FVector(KeyboardMovementInput.X, KeyboardMovementInput.Y, 0.f).GetSafeNormal();
	}

	FHitResult HitResult;
	if (GetCursorWorldHit(HitResult))
	{
		FVector Direction = HitResult.ImpactPoint - ARPGCharacter->GetActorLocation();
		Direction.Z = 0.f;
		if (!Direction.IsNearlyZero())
		{
			return Direction.GetSafeNormal();
		}
	}

	FVector ForwardDirection = ARPGCharacter->GetActorForwardVector();
	ForwardDirection.Z = 0.f;
	return ForwardDirection.IsNearlyZero() ? FVector::ForwardVector : ForwardDirection.GetSafeNormal();
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
	if (bIsPiercingSkillCasting)
	{
		bWasBasicAttackPressed = bIsBasicAttackPressed;
		return;
	}

	if (bIsWhirlwinding)
	{
		if (bIsBasicAttackPressed && !bWasBasicAttackPressed)
		{
			UE_LOG(LogMyGame, Warning, TEXT("BasicAttack blocked during Whirlwind"));
		}

		bWasBasicAttackPressed = bIsBasicAttackPressed;
		return;
	}

	if (bIsAreaSkillCasting)
	{
		bWasBasicAttackPressed = bIsBasicAttackPressed;
		return;
	}

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
			const float FinalDamage = bIsEmpowered ? BasicAttackDamage * EmpowerDamageMultiplier : BasicAttackDamage;
			UE_LOG(LogMyGame, Warning, TEXT("BasicAttack applying %.1f damage to enemy: %s"), FinalDamage, *Enemy->GetName());
			Enemy->ReceiveAttackHit(FinalDamage);
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

FVector AARPGPlayerController::GetAreaSkillCenter(const AARPGPlayerCharacter* ARPGCharacter)
{
	if (!ARPGCharacter)
	{
		return FVector::ZeroVector;
	}

	FVector WorldOrigin;
	FVector WorldDirection;
	if (DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ARPGAreaSkillMouseTrace), false);
		QueryParams.AddIgnoredActor(GetPawn());

		FHitResult HitResult;
		const FVector TraceEnd = WorldOrigin + (WorldDirection * 100000.f);
		if (GetWorld() && GetWorld()->LineTraceSingleByChannel(HitResult, WorldOrigin, TraceEnd, ECC_Visibility, QueryParams))
		{
			FVector AreaCenter = HitResult.ImpactPoint;
			AreaCenter.Z = ARPGCharacter->GetActorLocation().Z;
			return AreaCenter;
		}
	}

	FVector ForwardDirection = ARPGCharacter->GetActorForwardVector();
	ForwardDirection.Z = 0.f;
	ForwardDirection = ForwardDirection.IsNearlyZero() ? FVector::ForwardVector : ForwardDirection.GetSafeNormal();
	return ARPGCharacter->GetActorLocation() + (ForwardDirection * 250.f);
}

void AARPGPlayerController::PerformAreaSkill(const FVector& AreaCenter)
{
	AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter();
	if (!ARPGCharacter || !GetWorld())
	{
		UE_LOG(LogMyGame, Log, TEXT("AreaSkill missed"));
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ARPGAreaSkillOverlap), false);
	QueryParams.AddIgnoredActor(ARPGCharacter);

	TArray<FOverlapResult> OverlapResults;
	const FCollisionShape AreaShape = FCollisionShape::MakeSphere(AreaSkillRadius);
	const bool bHit = GetWorld()->OverlapMultiByChannel(OverlapResults, AreaCenter, FQuat::Identity, ECC_Pawn, AreaShape, QueryParams);

	DrawDebugSphere(GetWorld(), AreaCenter, AreaSkillRadius, 32, FColor::Cyan, false, 0.8f, 0, 2.f);

	TSet<AARPGEnemyBase*> HitEnemies;
	if (bHit)
	{
		for (const FOverlapResult& OverlapResult : OverlapResults)
		{
			AARPGEnemyBase* Enemy = Cast<AARPGEnemyBase>(OverlapResult.GetActor());
			if (!Enemy || Enemy->IsDead() || HitEnemies.Contains(Enemy))
			{
				continue;
			}

			HitEnemies.Add(Enemy);
			DrawDebugSphere(GetWorld(), Enemy->GetActorLocation(), 28.f, 12, FColor::Yellow, false, 0.8f);
			UE_LOG(LogMyGame, Log, TEXT("AreaSkill hit enemy: %s"), *Enemy->GetName());
			const float FinalAreaDamage = bIsEmpowered ? AreaSkillDamage * EmpowerDamageMultiplier : AreaSkillDamage;
			Enemy->ReceiveAttackHit(FinalAreaDamage);
		}
	}

	if (HitEnemies.Num() == 0)
	{
		UE_LOG(LogMyGame, Log, TEXT("AreaSkill missed"));
	}
}

FVector AARPGPlayerController::GetPiercingSkillDirection(const AARPGPlayerCharacter* ARPGCharacter)
{
	if (!ARPGCharacter)
	{
		return FVector::ForwardVector;
	}

	FHitResult HitResult;
	if (GetCursorWorldHit(HitResult))
	{
		FVector Direction = HitResult.ImpactPoint - ARPGCharacter->GetActorLocation();
		Direction.Z = 0.f;
		if (!Direction.IsNearlyZero())
		{
			return Direction.GetSafeNormal();
		}
	}

	FVector ForwardDirection = ARPGCharacter->GetActorForwardVector();
	ForwardDirection.Z = 0.f;
	return ForwardDirection.IsNearlyZero() ? FVector::ForwardVector : ForwardDirection.GetSafeNormal();
}

void AARPGPlayerController::PerformPiercingSkill(const FVector& Direction)
{
	AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter();
	if (!ARPGCharacter || !GetWorld())
	{
		UE_LOG(LogMyGame, Log, TEXT("PiercingSkill missed"));
		return;
	}

	FVector SkillDirection = Direction;
	SkillDirection.Z = 0.f;
	SkillDirection = SkillDirection.IsNearlyZero() ? ARPGCharacter->GetActorForwardVector() : SkillDirection.GetSafeNormal();
	SkillDirection.Z = 0.f;
	SkillDirection = SkillDirection.IsNearlyZero() ? FVector::ForwardVector : SkillDirection.GetSafeNormal();

	const FVector PlayerLocation = ARPGCharacter->GetActorLocation();
	const FVector Start = PlayerLocation + (SkillDirection * 80.f);
	const FVector End = PlayerLocation + (SkillDirection * PiercingSkillRange);
	const FCollisionShape SkillShape = FCollisionShape::MakeSphere(PiercingSkillWidth);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ARPGPiercingSkillSweep), false);
	QueryParams.AddIgnoredActor(ARPGCharacter);

	TArray<FHitResult> HitResults;
	const bool bHit = GetWorld()->SweepMultiByChannel(HitResults, Start, End, FQuat::Identity, ECC_Pawn, SkillShape, QueryParams);

	DrawDebugLine(GetWorld(), Start, End, FColor::Purple, false, 0.8f, 0, 3.f);
	DrawDebugSphere(GetWorld(), Start, PiercingSkillWidth, 20, FColor::Purple, false, 0.8f, 0, 2.f);
	DrawDebugSphere(GetWorld(), End, PiercingSkillWidth, 20, FColor::Purple, false, 0.8f, 0, 2.f);
	for (int32 Index = 1; Index < 5; ++Index)
	{
		const FVector DebugPoint = FMath::Lerp(Start, End, Index / 5.f);
		DrawDebugSphere(GetWorld(), DebugPoint, PiercingSkillWidth, 16, FColor::Purple, false, 0.8f, 0, 1.f);
	}

	TSet<AARPGEnemyBase*> HitEnemies;
	if (bHit)
	{
		for (const FHitResult& HitResult : HitResults)
		{
			AActor* HitActor = HitResult.GetActor();
			AARPGEnemyBase* Enemy = Cast<AARPGEnemyBase>(HitActor);
			if (!Enemy && HitActor)
			{
				Enemy = Cast<AARPGEnemyBase>(HitActor->GetOwner());
			}

			if (!Enemy || Enemy->IsDead() || HitEnemies.Contains(Enemy))
			{
				continue;
			}

			HitEnemies.Add(Enemy);
			DrawDebugSphere(GetWorld(), Enemy->GetActorLocation(), 32.f, 12, FColor::Yellow, false, 0.8f);
			UE_LOG(LogMyGame, Log, TEXT("PiercingSkill hit enemy: %s"), *Enemy->GetName());

			const float FinalDamage = bIsEmpowered ? PiercingSkillDamage * EmpowerDamageMultiplier : PiercingSkillDamage;
			const float FinalBleedDamage = bIsEmpowered ? BleedDamagePerTick * EmpowerDamageMultiplier : BleedDamagePerTick;
			Enemy->ReceiveAttackHit(FinalDamage);
			if (!Enemy->IsDead())
			{
				Enemy->ApplyBleed(FinalBleedDamage, BleedDuration, BleedTickInterval);
			}
		}
	}

	if (HitEnemies.Num() == 0)
	{
		UE_LOG(LogMyGame, Log, TEXT("PiercingSkill missed"));
	}
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

UARPGManaComponent* AARPGPlayerController::GetManaComponent() const
{
	const AARPGPlayerCharacter* ARPGCharacter = GetARPGCharacter();
	return ARPGCharacter ? ARPGCharacter->GetManaComponent() : nullptr;
}

bool AARPGPlayerController::IsEmpowered() const
{
	return bIsEmpowered;
}

float AARPGPlayerController::GetEmpowerCooldownRemaining() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}

	const float CooldownEndTime = LastEmpowerTime + EmpowerCooldown;
	return FMath::Max(0.f, CooldownEndTime - World->GetTimeSeconds());
}

float AARPGPlayerController::GetCurrentMana() const
{
	const UARPGManaComponent* ManaComponent = GetManaComponent();
	return ManaComponent ? ManaComponent->GetCurrentMana() : 0.f;
}

float AARPGPlayerController::GetMaxMana() const
{
	const UARPGManaComponent* ManaComponent = GetManaComponent();
	return ManaComponent ? ManaComponent->GetMaxMana() : 0.f;
}

bool AARPGPlayerController::IsWhirlwinding() const
{
	return bIsWhirlwinding;
}

int32 AARPGPlayerController::GetHealthPotionCount() const
{
	return HealthPotionCount;
}

int32 AARPGPlayerController::GetManaPotionCount() const
{
	return ManaPotionCount;
}
