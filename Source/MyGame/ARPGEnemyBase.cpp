// Copyright Epic Games, Inc. All Rights Reserved.

#include "ARPGEnemyBase.h"
#include "ARPGHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "ARPGMissionManager.h"
#include "ARPGPlayerController.h"
#include "MyGame.h"

AARPGEnemyBase::AARPGEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	HealthComponent = CreateDefaultSubobject<UARPGHealthComponent>(TEXT("HealthComponent"));
}

void AARPGEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = EnemyMoveSpeed;
		MovementComponent->SetMovementMode(MOVE_Walking);
	}

	BossPhase = bIsBoss ? EARPGBossPhase::Phase1 : EARPGBossPhase::None;
	bHasTriggeredPhase2HalfHealthWave = false;
	bHasEnteredPhase3 = false;
	bHasStartedBossPhase3Ultimate = false;
	bHasBossPhase3SummonedMinions = false;
	bIsCastingBossPhase3FireRain = false;
	bIsBossPhase3SkillRecovering = false;
	NextBossPhase3SkillAllowedTime = 0.f;
	BossPhase3SkillRecoveryEndTime = 0.f;
	bHasLastBossPhase3SkillUsed = false;
	bHasLoggedBossPhase3BlinkSlashLocked = false;
	BossPhase3SummonedMinions.Empty();

	UARPGHealthComponent* InstanceHealth = ResolveHealthComponent();
	if (InstanceHealth)
	{
		InstanceHealth->InitializeHealth(true);

		UE_LOG(LogMyGame, Warning, TEXT("%s Enemy BeginPlay Health: %.1f / %.1f, HealthOwner=%s"),
			*GetName(),
			InstanceHealth->GetCurrentHealth(),
			InstanceHealth->GetMaxHealth(),
			InstanceHealth->GetOwner() ? *InstanceHealth->GetOwner()->GetName() : TEXT("None"));
	}

	UE_LOG(LogMyGame, Log, TEXT("Enemy BeginPlay: %s, bEnableSimpleAI: %s, Controller: %s"),
		*GetName(),
		bEnableSimpleAI ? TEXT("true") : TEXT("false"),
		GetController() ? *GetController()->GetName() : TEXT("None"));
}

void AARPGEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead)
	{
		return;
	}

	if (GetWorld())
	{
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		UpdateBleed(CurrentTime);
		if (bIsDead)
		{
			return;
		}

		if (bIsBoss)
		{
			UpdateBossPhase3FireRainZones(DeltaTime);
			UpdateBossPhase3IceSpears(DeltaTime);
			UpdateBossPhase3ThunderOrbs(DeltaTime);
		}

		if (bIsBossPhase3UltimateActive)
		{
			UpdateBossPhase3Ultimate(DeltaTime);
			return;
		}

		if (bIsBossPhase3SkillRecovering)
		{
			HandleBossPhase3SkillRecovery(DeltaTime);
			return;
		}

		if (bIsCastingBossPhase3FireRain)
		{
			HandleBossPhase3FireRainCast(DeltaTime);
			return;
		}

		if (bIsPreparingBossPhase3IceSpear)
		{
			HandleBossPhase3IceSpearWindup(DeltaTime);
			return;
		}

		if (bIsPreparingBossPhase3Thunder)
		{
			HandleBossPhase3ThunderWindup(DeltaTime);
			return;
		}

		if (bIsBossPhase2WaveAttackActive)
		{
			UpdateBossPhase2WaveAttack(DeltaTime);
			return;
		}

		UARPGHealthComponent* InstanceHealth = bIsBoss ? ResolveHealthComponent() : nullptr;
		if (bIsBoss
			&& BossPhase == EARPGBossPhase::Phase1
			&& InstanceHealth
			&& InstanceHealth->GetCurrentHealth() <= InstanceHealth->GetMaxHealth() * 0.6667f)
		{
			EnterBossPhase2();
		}

		if (bIsBoss && BossPhase == EARPGBossPhase::Phase2Transition)
		{
			HandleBossPhase2Transition(DeltaTime);
			return;
		}

		if (bIsBoss
			&& BossPhase == EARPGBossPhase::Phase2
			&& bEnableBossPhase2WaveAttack
			&& !bHasTriggeredPhase2HalfHealthWave
			&& InstanceHealth
			&& InstanceHealth->GetCurrentHealth() <= InstanceHealth->GetMaxHealth() * BossPhase2WaveHealthThreshold)
		{
			StartBossPhase2WaveAttack();
			return;
		}

		if (bIsBoss
			&& BossPhase == EARPGBossPhase::Phase2
			&& InstanceHealth
			&& InstanceHealth->GetCurrentHealth() <= InstanceHealth->GetMaxHealth() * (1.f / 3.f))
		{
			EnterBossPhase3();
			return;
		}
	}

	UpdateSimpleAI(DeltaTime);
}

void AARPGEnemyBase::ApplyDamageToEnemy(float DamageAmount)
{
	ReceiveAttackHit(DamageAmount);
}

UARPGHealthComponent* AARPGEnemyBase::GetHealthComponent()
{
	return ResolveHealthComponent();
}

UARPGHealthComponent* AARPGEnemyBase::ResolveHealthComponent()
{
	if (HealthComponent && HealthComponent->GetOwner() == this && !HealthComponent->IsTemplate())
	{
		return HealthComponent;
	}

	UE_LOG(LogMyGame, Error, TEXT("%s HealthComponent invalid or wrong owner. StoredOwner=%s. Trying FindComponentByClass."),
		*GetName(),
		HealthComponent && HealthComponent->GetOwner() ? *HealthComponent->GetOwner()->GetName() : TEXT("None"));

	UARPGHealthComponent* InstanceHealth = FindComponentByClass<UARPGHealthComponent>();
	if (!InstanceHealth)
	{
		HealthComponent = nullptr;
		UE_LOG(LogMyGame, Error, TEXT("%s has no valid instance HealthComponent"), *GetName());
		return nullptr;
	}

	if (InstanceHealth->GetOwner() != this || InstanceHealth->IsTemplate())
	{
		UE_LOG(LogMyGame, Error, TEXT("%s still has wrong HealthComponent owner: %s. Abort health access."),
			*GetName(),
			InstanceHealth->GetOwner() ? *InstanceHealth->GetOwner()->GetName() : TEXT("None"));
		return nullptr;
	}

	HealthComponent = InstanceHealth;
	return HealthComponent;
}

void AARPGEnemyBase::ReceiveAttackHit(float DamageAmount)
{
	if (bIsDead)
	{
		UE_LOG(LogMyGame, Warning, TEXT("%s ignored ReceiveAttackHit because already dead"), *GetName());
		return;
	}

	UE_LOG(LogMyGame, Warning, TEXT("%s ReceiveAttackHit %.1f"), *GetName(), DamageAmount);

	PlayHitFeedback();
	ApplyDamageWithBossModifiers(DamageAmount);
}

void AARPGEnemyBase::ApplyBleed(float DamagePerTick, float Duration, float TickInterval)
{
	if (bIsDead || DamagePerTick <= 0.f || Duration <= 0.f || TickInterval <= 0.f || !GetWorld())
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	bIsBleeding = true;
	BleedDamagePerTickRuntime = DamagePerTick;
	BleedTickIntervalRuntime = TickInterval;
	BleedEndTime = CurrentTime + Duration;
	NextBleedTickTime = CurrentTime + TickInterval;

	UE_LOG(LogMyGame, Log, TEXT("%s Enemy bleeding started"), *GetName());
}

void AARPGEnemyBase::UpdateSimpleAI(float DeltaTime)
{
	if (bIsDead || !bEnableSimpleAI || !GetWorld())
	{
		return;
	}

	if (!bIsBoss)
	{
		switch (EnemyCombatType)
		{
		case EARPGEnemyCombatType::Mage:
			UpdateMageAI(DeltaTime);
			return;
		case EARPGEnemyCombatType::Thrower:
			UpdateThrowerAI(DeltaTime);
			return;
		case EARPGEnemyCombatType::Melee:
		default:
			UpdateMeleeAI(DeltaTime);
			return;
		}
	}

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!PlayerPawn)
	{
		return;
	}

	if (bIsPreparingBossLongSlash)
	{
		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}

		FaceDirection(BossLongSlashDirection, DeltaTime);
		DrawBossLongSlashDebug(0.05f);

		if (GetWorld()->GetTimeSeconds() >= BossLongSlashResolveTime)
		{
			ResolveBossLongSlash();
		}

		return;
	}

	if (bIsRecoveringBossLongSlash)
	{
		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}

		if (GetWorld()->GetTimeSeconds() >= BossLongSlashRecoveryEndTime)
		{
			bIsRecoveringBossLongSlash = false;
			UE_LOG(LogMyGame, Log, TEXT("Boss Long Slash recovery ended"));
		}

		return;
	}

	if (bIsPreparingBossRandomSlash)
	{
		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}

		FaceDirection(BossRandomSlashBaseDirection, DeltaTime);
		DrawBossRandomSlashFanDebug(0.05f);

		if (GetWorld()->GetTimeSeconds() >= BossRandomSlashStartTime)
		{
			bIsPreparingBossRandomSlash = false;
			bIsExecutingBossRandomSlash = true;
			BossRandomSlashNextTime = GetWorld()->GetTimeSeconds();
			UE_LOG(LogMyGame, Log, TEXT("Boss Random Slash execution started"));
		}

		return;
	}

	if (bIsExecutingBossRandomSlash)
	{
		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}

		FaceDirection(BossRandomSlashBaseDirection, DeltaTime);
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime >= BossRandomSlashNextTime && BossRandomSlashExecutedCount < BossRandomSlashCount)
		{
			ExecuteOneBossRandomSlash();
			++BossRandomSlashExecutedCount;
			BossRandomSlashNextTime = CurrentTime + BossRandomSlashInterval;
		}

		if (BossRandomSlashExecutedCount >= BossRandomSlashCount)
		{
			bIsExecutingBossRandomSlash = false;
			UE_LOG(LogMyGame, Log, TEXT("Boss Random Slash ended"));
			if (bIsBoss && BossPhase == EARPGBossPhase::Phase3)
			{
				StartBossPhase3SkillRecovery();
			}
		}

		return;
	}

	if (bIsPreparingBossCone)
	{
		AActor* FacingTarget = PendingBossConeTarget.IsValid() ? PendingBossConeTarget.Get() : PlayerPawn;
		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}

		if (FacingTarget)
		{
			FVector FacingDirection = FacingTarget->GetActorLocation() - GetActorLocation();
			FacingDirection.Z = 0.f;
			FaceDirection(FacingDirection, DeltaTime);
		}

		DrawBossConeStrikeDebug(0.05f);

		if (GetWorld()->GetTimeSeconds() >= BossConeResolveTime)
		{
			ResolveBossConeStrike();
		}

		return;
	}

	if (bIsPreparingBossBarrage)
	{
		AActor* FacingTarget = PendingBossBarrageTarget.IsValid() ? PendingBossBarrageTarget.Get() : PlayerPawn;
		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}

		if (FacingTarget)
		{
			FVector FacingDirection = FacingTarget->GetActorLocation() - GetActorLocation();
			FacingDirection.Z = 0.f;
			FaceDirection(FacingDirection, DeltaTime);
		}

		DrawBossBarrageDebug(0.05f);

		if (GetWorld()->GetTimeSeconds() >= BossBarrageResolveTime)
		{
			LaunchBossBarrage();
		}

		return;
	}

	if (bIsBossBarrageActive)
	{
		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}

		UpdateBossBarrageOrbs(DeltaTime);
		return;
	}

	if (bIsPreparingBossSlam)
	{
		AActor* FacingTarget = PendingBossSlamTarget.IsValid() ? PendingBossSlamTarget.Get() : PlayerPawn;
		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}

		if (FacingTarget)
		{
			FVector FacingDirection = FacingTarget->GetActorLocation() - GetActorLocation();
			FacingDirection.Z = 0.f;
			FaceDirection(FacingDirection, DeltaTime);
		}

		DrawBossSlamRangeDebug(0.05f);

		if (GetWorld()->GetTimeSeconds() >= BossSlamResolveTime)
		{
			ResolveBossSlam();
		}

		return;
	}

	if (bIsPreparingAttack)
	{
		AActor* FacingTarget = PendingAttackTarget.IsValid() ? PendingAttackTarget.Get() : PlayerPawn;
		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}

		if (FacingTarget)
		{
			FVector FacingDirection = FacingTarget->GetActorLocation() - GetActorLocation();
			FacingDirection.Z = 0.f;
			FaceDirection(FacingDirection, DeltaTime);
		}

		DrawEnemyAttackRangeDebug(0.05f);

		if (GetWorld()->GetTimeSeconds() >= EnemyAttackResolveTime)
		{
			ResolveEnemyAttack();
		}

		return;
	}

	FVector EnemyLocation = GetActorLocation();
	FVector PlayerLocation = PlayerPawn->GetActorLocation();
	EnemyLocation.Z = 0.f;
	PlayerLocation.Z = 0.f;

	const float DistanceToPlayer = FVector::Dist2D(EnemyLocation, PlayerLocation);
	if (DistanceToPlayer > AggroRange)
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (bIsBoss && BossPhase == EARPGBossPhase::Phase1 && TryStartBossPhase1Skill(PlayerPawn, DistanceToPlayer))
	{
		return;
	}

	if (bIsBoss && BossPhase == EARPGBossPhase::Phase2 && TryStartBossPhase2Skill(PlayerPawn, DistanceToPlayer))
	{
		return;
	}

	if (bIsBoss && BossPhase == EARPGBossPhase::Phase3 && TryUseRandomBossPhase3Skill(PlayerPawn))
	{
		return;
	}

	FVector Direction = PlayerLocation - EnemyLocation;
	Direction.Z = 0.f;
	if (Direction.IsNearlyZero())
	{
		return;
	}

	const FVector MoveDirection = Direction.GetSafeNormal();
	if (DistanceToPlayer > AttackRange)
	{
		AddMovementInput(MoveDirection, 1.f);
		FaceDirection(MoveDirection, DeltaTime);
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	FaceDirection(MoveDirection, DeltaTime);
	if (CurrentTime - LastEnemyAttackTime >= EnemyAttackCooldown)
	{
		StartEnemyAttack(PlayerPawn, DeltaTime);
	}
}

void AARPGEnemyBase::UpdateMeleeAI(float DeltaTime)
{
	if (bIsDead || !GetWorld())
	{
		return;
	}

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!PlayerPawn)
	{
		return;
	}

	if (bIsPreparingAttack)
	{
		AActor* FacingTarget = PendingAttackTarget.IsValid() ? PendingAttackTarget.Get() : PlayerPawn;
		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}

		if (FacingTarget)
		{
			FVector FacingDirection = FacingTarget->GetActorLocation() - GetActorLocation();
			FacingDirection.Z = 0.f;
			FaceDirection(FacingDirection, DeltaTime);
		}

		DrawEnemyAttackRangeDebug(0.05f);

		if (GetWorld()->GetTimeSeconds() >= EnemyAttackResolveTime)
		{
			ResolveEnemyAttack();
		}

		return;
	}

	FVector EnemyLocation = GetActorLocation();
	FVector PlayerLocation = PlayerPawn->GetActorLocation();
	EnemyLocation.Z = 0.f;
	PlayerLocation.Z = 0.f;

	const float DistanceToPlayer = FVector::Dist2D(EnemyLocation, PlayerLocation);
	if (DistanceToPlayer > AggroRange)
	{
		return;
	}

	FVector Direction = PlayerLocation - EnemyLocation;
	Direction.Z = 0.f;
	if (Direction.IsNearlyZero())
	{
		return;
	}

	const FVector MoveDirection = Direction.GetSafeNormal();
	if (DistanceToPlayer > AttackRange)
	{
		AddMovementInput(MoveDirection, 1.f);
		FaceDirection(MoveDirection, DeltaTime);
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	FaceDirection(MoveDirection, DeltaTime);
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastEnemyAttackTime >= EnemyAttackCooldown)
	{
		StartEnemyAttack(PlayerPawn, DeltaTime);
	}
}

void AARPGEnemyBase::UpdateMageAI(float DeltaTime)
{
	if (bIsDead || !GetWorld())
	{
		return;
	}

	UpdateMageProjectiles(DeltaTime);

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!PlayerPawn)
	{
		return;
	}

	if (bIsPreparingMageAttack)
	{
		HandleMageAttackWindup(DeltaTime);
		return;
	}

	FVector EnemyLocation = GetActorLocation();
	FVector PlayerLocation = PlayerPawn->GetActorLocation();
	EnemyLocation.Z = 0.f;
	PlayerLocation.Z = 0.f;

	const float DistanceToPlayer = FVector::Dist2D(EnemyLocation, PlayerLocation);
	if (DistanceToPlayer > AggroRange)
	{
		return;
	}

	FVector Direction = PlayerLocation - EnemyLocation;
	Direction.Z = 0.f;
	if (Direction.IsNearlyZero())
	{
		return;
	}

	const FVector MoveDirection = Direction.GetSafeNormal();
	if (DistanceToPlayer > MagePreferredDistance)
	{
		AddMovementInput(MoveDirection, 1.f);
	}
	else if (DistanceToPlayer < MageTooCloseDistance)
	{
		AddMovementInput(-MoveDirection, 0.75f);
	}
	else if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	FaceDirection(MoveDirection, DeltaTime);

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (DistanceToPlayer <= MageAttackRange && CurrentTime - LastMageAttackTime >= MageAttackCooldown)
	{
		StartMageAttack(PlayerPawn);
	}
}

void AARPGEnemyBase::StartMageAttack(AActor* TargetActor)
{
	if (bIsDead || bIsPreparingMageAttack || !TargetActor || !GetWorld())
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastMageAttackTime < MageAttackCooldown)
	{
		return;
	}

	FVector Direction = TargetActor->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.f;
	MageAttackDirection = Direction.IsNearlyZero() ? GetActorForwardVector() : Direction.GetSafeNormal();
	MageAttackDirection.Z = 0.f;
	MageAttackDirection = MageAttackDirection.IsNearlyZero() ? FVector::ForwardVector : MageAttackDirection.GetSafeNormal();

	bIsPreparingMageAttack = true;
	LastMageAttackTime = CurrentTime;
	MageAttackResolveTime = CurrentTime + FMath::Max(0.f, MageAttackWindup);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	FaceDirection(MageAttackDirection, 0.1f);
	UE_LOG(LogMyGame, Log, TEXT("Mage attack windup started"));
}

void AARPGEnemyBase::HandleMageAttackWindup(float DeltaTime)
{
	if (bIsDead || !GetWorld())
	{
		bIsPreparingMageAttack = false;
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	FaceDirection(MageAttackDirection, DeltaTime);
	DrawMageAttackDebug(0.05f);

	if (GetWorld()->GetTimeSeconds() >= MageAttackResolveTime)
	{
		bIsPreparingMageAttack = false;
		LaunchMageProjectile();
	}
}

void AARPGEnemyBase::LaunchMageProjectile()
{
	if (bIsDead || !GetWorld())
	{
		return;
	}

	FVector Direction = MageAttackDirection;
	Direction.Z = 0.f;
	Direction = Direction.IsNearlyZero() ? GetActorForwardVector() : Direction.GetSafeNormal();
	Direction.Z = 0.f;
	Direction = Direction.IsNearlyZero() ? FVector::ForwardVector : Direction.GetSafeNormal();

	FARPGEnemyMageProjectile Projectile;
	Projectile.Direction = Direction;
	Projectile.Location = GetActorLocation() + Direction * 80.f + FVector(0.f, 0.f, 55.f);
	Projectile.TraveledDistance = 0.f;
	Projectile.bActive = true;
	Projectile.bHitPlayer = false;
	ActiveMageProjectiles.Add(Projectile);

	UE_LOG(LogMyGame, Log, TEXT("Mage projectile launched"));
}

void AARPGEnemyBase::UpdateMageProjectiles(float DeltaTime)
{
	if (!GetWorld() || ActiveMageProjectiles.Num() == 0)
	{
		return;
	}

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UARPGHealthComponent* PlayerHealthComponent = PlayerPawn ? PlayerPawn->FindComponentByClass<UARPGHealthComponent>() : nullptr;

	const float DeltaDistance = FMath::Max(0.f, MageProjectileSpeed) * DeltaTime;
	const float MaxDistance = FMath::Max(0.f, MageProjectileMaxDistance);
	const float HitRadius = FMath::Max(1.f, MageProjectileRadius);

	for (FARPGEnemyMageProjectile& Projectile : ActiveMageProjectiles)
	{
		if (!Projectile.bActive)
		{
			continue;
		}

		Projectile.Location += Projectile.Direction * DeltaDistance;
		Projectile.TraveledDistance += DeltaDistance;
		DrawDebugSphere(GetWorld(), Projectile.Location, HitRadius, 12, FColor::Cyan, false, 0.05f, 0, 2.f);

		if (Projectile.TraveledDistance >= MaxDistance)
		{
			Projectile.bActive = false;
			continue;
		}

		if (PlayerPawn
			&& PlayerHealthComponent
			&& !PlayerHealthComponent->IsDead()
			&& !Projectile.bHitPlayer
			&& FVector::Dist2D(PlayerPawn->GetActorLocation(), Projectile.Location) <= HitRadius + 40.f)
		{
			Projectile.bHitPlayer = true;
			Projectile.bActive = false;
			PlayerHealthComponent->ApplyDamage(MageProjectileDamage);
			UE_LOG(LogMyGame, Log, TEXT("Mage projectile hit player"));
			DrawDebugSphere(GetWorld(), PlayerPawn->GetActorLocation(), 50.f, 12, FColor::Red, false, 0.3f);
		}
	}

	ActiveMageProjectiles.RemoveAll([](const FARPGEnemyMageProjectile& Projectile)
	{
		return !Projectile.bActive;
	});
}

void AARPGEnemyBase::DrawMageAttackDebug(float Duration) const
{
	if (!GetWorld())
	{
		return;
	}

	FVector Direction = MageAttackDirection;
	Direction.Z = 0.f;
	Direction = Direction.IsNearlyZero() ? GetActorForwardVector() : Direction.GetSafeNormal();
	Direction.Z = 0.f;
	Direction = Direction.IsNearlyZero() ? FVector::ForwardVector : Direction.GetSafeNormal();

	const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 55.f);
	const FVector End = Start + Direction * MageProjectileMaxDistance;
	DrawDebugLine(GetWorld(), Start, End, FColor::Cyan, false, Duration, 0, 2.f);
}

void AARPGEnemyBase::UpdateThrowerAI(float DeltaTime)
{
	if (bIsDead || !GetWorld())
	{
		return;
	}

	UpdateThrowerFireZones(DeltaTime);

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!PlayerPawn)
	{
		return;
	}

	if (bIsPreparingThrowerAttack)
	{
		HandleThrowerAttackWindup(DeltaTime);
		return;
	}

	FVector EnemyLocation = GetActorLocation();
	FVector PlayerLocation = PlayerPawn->GetActorLocation();
	EnemyLocation.Z = 0.f;
	PlayerLocation.Z = 0.f;

	const float DistanceToPlayer = FVector::Dist2D(EnemyLocation, PlayerLocation);
	if (DistanceToPlayer > AggroRange)
	{
		return;
	}

	FVector Direction = PlayerLocation - EnemyLocation;
	Direction.Z = 0.f;
	if (Direction.IsNearlyZero())
	{
		return;
	}

	const FVector MoveDirection = Direction.GetSafeNormal();
	if (DistanceToPlayer > ThrowerPreferredDistance)
	{
		AddMovementInput(MoveDirection, 1.f);
	}
	else if (DistanceToPlayer < ThrowerPreferredDistance * 0.65f)
	{
		AddMovementInput(-MoveDirection, 0.65f);
	}
	else if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	FaceDirection(MoveDirection, DeltaTime);

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (DistanceToPlayer <= ThrowerAttackRange && CurrentTime - LastThrowerAttackTime >= ThrowerAttackCooldown)
	{
		StartThrowerAttack(PlayerPawn);
	}
}

void AARPGEnemyBase::StartThrowerAttack(AActor* TargetActor)
{
	if (bIsDead || bIsPreparingThrowerAttack || !TargetActor || !GetWorld())
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastThrowerAttackTime < ThrowerAttackCooldown)
	{
		return;
	}

	PendingThrowerTargetLocation = TargetActor->GetActorLocation();
	PendingThrowerTargetLocation.Z = GetActorLocation().Z + 10.f;
	bIsPreparingThrowerAttack = true;
	LastThrowerAttackTime = CurrentTime;
	ThrowerAttackResolveTime = CurrentTime + FMath::Max(0.f, ThrowerAttackWindup);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	FVector FacingDirection = TargetActor->GetActorLocation() - GetActorLocation();
	FacingDirection.Z = 0.f;
	FaceDirection(FacingDirection, 0.1f);
	UE_LOG(LogMyGame, Log, TEXT("Thrower attack windup started"));
}

void AARPGEnemyBase::HandleThrowerAttackWindup(float DeltaTime)
{
	if (bIsDead || !GetWorld())
	{
		bIsPreparingThrowerAttack = false;
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (PlayerPawn)
	{
		FVector FacingDirection = PlayerPawn->GetActorLocation() - GetActorLocation();
		FacingDirection.Z = 0.f;
		FaceDirection(FacingDirection, DeltaTime);
	}

	DrawEnemyFireCircle(PendingThrowerTargetLocation, ThrowerFireRadius, FColor::Red, 0.05f, 3.f);

	if (GetWorld()->GetTimeSeconds() >= ThrowerAttackResolveTime)
	{
		bIsPreparingThrowerAttack = false;
		LaunchThrowerFireZone();
	}
}

void AARPGEnemyBase::LaunchThrowerFireZone()
{
	if (bIsDead || !GetWorld())
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const float ImpactDelay = FMath::Max(0.f, ThrowerImpactDelay);

	FARPGEnemyThrowerFireZone Zone;
	Zone.Center = PendingThrowerTargetLocation;
	Zone.ImpactTime = CurrentTime + ImpactDelay;
	Zone.GroundEndTime = Zone.ImpactTime + FMath::Max(0.f, ThrowerFireGroundDuration);
	Zone.NextGroundDamageTime = Zone.ImpactTime;
	Zone.bHasImpacted = false;
	Zone.bActive = true;
	ActiveThrowerFireZones.Add(Zone);

	UE_LOG(LogMyGame, Log, TEXT("Thrower fire zone launched"));
}

void AARPGEnemyBase::UpdateThrowerFireZones(float DeltaTime)
{
	if (!GetWorld() || ActiveThrowerFireZones.Num() == 0)
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const float Radius = FMath::Max(0.f, ThrowerFireRadius);
	const float GroundDamageInterval = FMath::Max(0.05f, ThrowerFireGroundDamageInterval);
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UARPGHealthComponent* PlayerHealthComponent = PlayerPawn ? PlayerPawn->FindComponentByClass<UARPGHealthComponent>() : nullptr;

	for (FARPGEnemyThrowerFireZone& Zone : ActiveThrowerFireZones)
	{
		if (!Zone.bActive)
		{
			continue;
		}

		if (!Zone.bHasImpacted)
		{
			DrawEnemyFireCircle(Zone.Center, Radius, FColor::Red, 0.05f, 3.f);
			if (CurrentTime >= Zone.ImpactTime)
			{
				Zone.bHasImpacted = true;
				ResolveThrowerFireImpact(Zone);
			}
		}

		if (Zone.bHasImpacted)
		{
			DrawEnemyFireCircle(Zone.Center, Radius, FColor::Orange, 0.05f, 4.f);
			DrawDebugSphere(GetWorld(), Zone.Center, Radius, 16, FColor::Orange, false, 0.05f, 0, 1.f);

			if (CurrentTime >= Zone.GroundEndTime)
			{
				Zone.bActive = false;
				continue;
			}

			if (PlayerPawn
				&& PlayerHealthComponent
				&& !PlayerHealthComponent->IsDead()
				&& FVector::Dist2D(PlayerPawn->GetActorLocation(), Zone.Center) <= Radius
				&& CurrentTime >= Zone.NextGroundDamageTime)
			{
				PlayerHealthComponent->ApplyDamage(ThrowerFireGroundDamage);
				Zone.NextGroundDamageTime = CurrentTime + GroundDamageInterval;
				UE_LOG(LogMyGame, Log, TEXT("Thrower fire ground damaged player"));
			}
		}
	}

	ActiveThrowerFireZones.RemoveAll([](const FARPGEnemyThrowerFireZone& Zone)
	{
		return !Zone.bActive;
	});
}

void AARPGEnemyBase::ResolveThrowerFireImpact(FARPGEnemyThrowerFireZone& Zone)
{
	if (!GetWorld())
	{
		return;
	}

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UARPGHealthComponent* PlayerHealthComponent = PlayerPawn ? PlayerPawn->FindComponentByClass<UARPGHealthComponent>() : nullptr;
	if (PlayerPawn
		&& PlayerHealthComponent
		&& !PlayerHealthComponent->IsDead()
		&& FVector::Dist2D(PlayerPawn->GetActorLocation(), Zone.Center) <= FMath::Max(0.f, ThrowerFireRadius))
	{
		PlayerHealthComponent->ApplyDamage(ThrowerImpactDamage);
		UE_LOG(LogMyGame, Log, TEXT("Thrower fire impact hit player"));
		DrawDebugSphere(GetWorld(), PlayerPawn->GetActorLocation(), 55.f, 12, FColor::Red, false, 0.3f);
	}
	else
	{
		UE_LOG(LogMyGame, Log, TEXT("Thrower fire impact missed"));
	}
}

void AARPGEnemyBase::DrawEnemyFireCircle(const FVector& Center, float Radius, const FColor& Color, float Duration, float Thickness) const
{
	if (!GetWorld() || Radius <= 0.f)
	{
		return;
	}

	const int32 SegmentCount = 24;
	FVector PreviousPoint = Center + FVector(Radius, 0.f, 0.f);
	for (int32 Index = 1; Index <= SegmentCount; ++Index)
	{
		const float AngleRadians = FMath::DegreesToRadians(360.f * static_cast<float>(Index) / static_cast<float>(SegmentCount));
		const FVector CurrentPoint = Center + FVector(FMath::Cos(AngleRadians) * Radius, FMath::Sin(AngleRadians) * Radius, 0.f);
		DrawDebugLine(GetWorld(), PreviousPoint, CurrentPoint, Color, false, Duration, 0, Thickness);
		PreviousPoint = CurrentPoint;
	}
}

void AARPGEnemyBase::EnterBossPhase2()
{
	if (!bIsBoss || BossPhase != EARPGBossPhase::Phase1)
	{
		return;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	BossPhase = EARPGBossPhase::Phase2Transition;
	BossPhase2TransitionEndTime = CurrentTime + BossPhase2TransitionDuration;
	bBossPhase2TeleportCompleted = false;
	bIsPreparingAttack = false;
	PendingAttackTarget = nullptr;
	bIsPreparingBossSlam = false;
	PendingBossSlamTarget = nullptr;
	bIsPreparingBossCone = false;
	bIsPreparingBossBarrage = false;
	bIsPreparingBossLongSlash = false;
	bIsRecoveringBossLongSlash = false;
	bIsPreparingBossRandomSlash = false;
	bIsExecutingBossRandomSlash = false;
	bIsPreparingMageAttack = false;
	bIsPreparingThrowerAttack = false;
	MageAttackResolveTime = 0.f;
	MageAttackDirection = FVector::ZeroVector;
	ThrowerAttackResolveTime = 0.f;
	PendingThrowerTargetLocation = FVector::ZeroVector;
	ActiveMageProjectiles.Empty();
	ActiveThrowerFireZones.Empty();
	bIsBossPhase2WaveAttackActive = false;
	ActiveBossPhase2WaveOrbs.Empty();
	BossPhase2WaveCurrentLoop = 0;
	bBossPhase2WaveHitPlayerThisFrameOrWave = false;
	bIsBossBarrageActive = false;
	bBossBarrageHitPlayerThisWave = false;
	PendingBossConeTarget = nullptr;
	PendingBossBarrageTarget = nullptr;
	PendingBossLongSlashTarget = nullptr;
	BossLongSlashDirection = FVector::ZeroVector;
	BossRandomSlashExecutedCount = 0;
	BossRandomSlashBaseDirection = FVector::ZeroVector;
	ActiveBossBarrageOrbs.Empty();

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	UE_LOG(LogMyGame, Log, TEXT("Boss entered Phase 2 transition"));
	UE_LOG(LogMyGame, Log, TEXT("Boss gains 90%% damage reduction"));
}

void AARPGEnemyBase::HandleBossPhase2Transition(float DeltaTime)
{
	if (bIsDead || !GetWorld())
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (PlayerPawn)
	{
		FVector FacingDirection = PlayerPawn->GetActorLocation() - GetActorLocation();
		FacingDirection.Z = 0.f;
		FaceDirection(FacingDirection, DeltaTime);
	}

	DrawDebugSphere(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 80.f), 90.f, 24, FColor::Purple, false, 0.05f, 0, 3.f);

	if (GetWorld()->GetTimeSeconds() >= BossPhase2TransitionEndTime)
	{
		FinishBossPhase2Transition();
	}
}

void AARPGEnemyBase::FinishBossPhase2Transition()
{
	if (bIsDead || !GetWorld())
	{
		return;
	}

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (PlayerPawn)
	{
		FVector PlayerForward = PlayerPawn->GetActorForwardVector();
		PlayerForward.Z = 0.f;
		PlayerForward = PlayerForward.IsNearlyZero() ? FVector::ForwardVector : PlayerForward.GetSafeNormal();

		FVector TargetLocation = PlayerPawn->GetActorLocation() - PlayerForward * BossPhase2TeleportBehindDistance;
		TargetLocation.Z = GetActorLocation().Z;
		SetActorLocation(TargetLocation, false);

		FVector FacingDirection = PlayerPawn->GetActorLocation() - GetActorLocation();
		FacingDirection.Z = 0.f;
		if (!FacingDirection.IsNearlyZero())
		{
			SetActorRotation(FRotator(0.f, FacingDirection.Rotation().Yaw, 0.f));
		}

		UE_LOG(LogMyGame, Log, TEXT("Boss teleported behind player"));
	}
	else
	{
		UE_LOG(LogMyGame, Warning, TEXT("Boss Phase 2 teleport skipped: player pawn not found"));
	}

	BossPhase = EARPGBossPhase::Phase2;
	bBossPhase2TeleportCompleted = true;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
		MovementComponent->StopMovementImmediately();
	}

	UE_LOG(LogMyGame, Log, TEXT("Boss Phase 2 started"));
}

void AARPGEnemyBase::ApplyDamageWithBossModifiers(float DamageAmount)
{
	UARPGHealthComponent* InstanceHealth = ResolveHealthComponent();
	if (!InstanceHealth)
	{
		UE_LOG(LogMyGame, Error, TEXT("%s has no valid instance HealthComponent"), *GetName());
		return;
	}

	float FinalDamage = DamageAmount;
	if (bIsBoss && bIsBossPhase3UltimateActive)
	{
		UE_LOG(LogMyGame, Warning, TEXT("Boss is invincible during Phase3 ultimate. Damage ignored."));
		return;
	}

	if (bIsBoss && bIsBossPhase2WaveAttackActive)
	{
		UE_LOG(LogMyGame, Warning, TEXT("Boss is invincible during Phase2 wave attack. Damage ignored."));
		return;
	}

	if (bIsBoss && BossPhase == EARPGBossPhase::Phase2Transition)
	{
		const float DamageMultiplier = 1.f - FMath::Clamp(BossPhase2TransitionDamageReduction, 0.f, 1.f);
		FinalDamage = DamageAmount * DamageMultiplier;
		UE_LOG(LogMyGame, Warning, TEXT("Boss phase transition damage reduced: %.1f -> %.1f"), DamageAmount, FinalDamage);
	}

	InstanceHealth->ApplyDamage(FinalDamage);

	if (InstanceHealth->IsDead())
	{
		Die();
	}
}

bool AARPGEnemyBase::TryStartBossPhase1Skill(AActor* TargetActor, float DistanceToTarget)
{
	if (!bIsBoss
		|| BossPhase != EARPGBossPhase::Phase1
		|| bIsDead
		|| !TargetActor
		|| !GetWorld()
		|| bIsPreparingAttack
		|| bIsPreparingBossCone
		|| bIsPreparingBossBarrage
		|| bIsBossBarrageActive)
	{
		return false;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const bool bCanUseCone = bEnableBossConeStrike
		&& DistanceToTarget <= BossConeRange
		&& CurrentTime - LastBossConeTime >= BossConeCooldown;
	const bool bCanUseBarrage = bEnableBossBarrage
		&& DistanceToTarget <= BossBarrageMaxDistance
		&& CurrentTime - LastBossBarrageTime >= BossBarrageCooldown;

	if (bNextBossSkillUseCone)
	{
		if (bCanUseCone)
		{
			StartBossConeStrike(TargetActor);
			bNextBossSkillUseCone = false;
			return true;
		}

		if (bCanUseBarrage)
		{
			StartBossBarrage(TargetActor);
			bNextBossSkillUseCone = true;
			return true;
		}
	}
	else
	{
		if (bCanUseBarrage)
		{
			StartBossBarrage(TargetActor);
			bNextBossSkillUseCone = true;
			return true;
		}

		if (bCanUseCone)
		{
			StartBossConeStrike(TargetActor);
			bNextBossSkillUseCone = false;
			return true;
		}
	}

	return false;
}

bool AARPGEnemyBase::TryStartBossPhase2Skill(AActor* TargetActor, float DistanceToTarget)
{
	if (!bIsBoss
		|| BossPhase != EARPGBossPhase::Phase2
		|| bIsDead
		|| !TargetActor
		|| !GetWorld()
		|| bIsPreparingAttack
		|| bIsPreparingBossCone
		|| bIsPreparingBossBarrage
		|| bIsBossBarrageActive
		|| bIsBossPhase2WaveAttackActive
		|| bIsBossPhase3UltimateActive
		|| bIsCastingBossPhase3FireRain
		|| bIsPreparingBossLongSlash
		|| bIsRecoveringBossLongSlash
		|| bIsPreparingBossRandomSlash
		|| bIsExecutingBossRandomSlash)
	{
		return false;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const bool bCanUseLongSlash = bEnableBossLongSlash
		&& DistanceToTarget <= BossLongSlashRange
		&& CurrentTime - LastBossLongSlashTime >= BossLongSlashCooldown;
	const bool bCanUseRandomSlash = bEnableBossRandomSlash
		&& DistanceToTarget <= BossRandomSlashRange
		&& CurrentTime - LastBossRandomSlashTime >= BossRandomSlashCooldown;

	if (bNextPhase2SkillUseLongSlash)
	{
		if (bCanUseLongSlash)
		{
			StartBossLongSlash(TargetActor);
			bNextPhase2SkillUseLongSlash = false;
			return true;
		}

		if (bCanUseRandomSlash)
		{
			StartBossRandomSlash(TargetActor);
			bNextPhase2SkillUseLongSlash = true;
			return true;
		}
	}
	else
	{
		if (bCanUseRandomSlash)
		{
			StartBossRandomSlash(TargetActor);
			bNextPhase2SkillUseLongSlash = true;
			return true;
		}

		if (bCanUseLongSlash)
		{
			StartBossLongSlash(TargetActor);
			bNextPhase2SkillUseLongSlash = false;
			return true;
		}
	}

	return false;
}

bool AARPGEnemyBase::TryStartBossPhase3Skill(AActor* TargetActor)
{
	return TryUseRandomBossPhase3Skill(TargetActor);
}

bool AARPGEnemyBase::TryUseRandomBossPhase3Skill(AActor* TargetActor)
{
	if (!bIsBoss
		|| BossPhase != EARPGBossPhase::Phase3
		|| bIsDead
		|| !TargetActor
		|| !GetWorld()
		|| bIsBossPhase3UltimateActive
		|| bIsBossPhase2WaveAttackActive
		|| bIsCastingBossPhase3FireRain
		|| bIsPreparingBossPhase3IceSpear
		|| bIsPreparingBossPhase3Thunder
		|| bIsBossPhase3SkillRecovering
		|| bIsPreparingAttack
		|| bIsPreparingBossCone
		|| bIsPreparingBossBarrage
		|| bIsBossBarrageActive
		|| bIsPreparingBossLongSlash
		|| bIsRecoveringBossLongSlash
		|| bIsPreparingBossRandomSlash
		|| bIsExecutingBossRandomSlash)
	{
		return false;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime < NextBossPhase3SkillAllowedTime)
	{
		return false;
	}

	TArray<EBossPhase3SkillType> AvailableSkills;
	if (bEnableBossPhase3FireRain && CurrentTime - LastBossPhase3FireRainTime >= BossPhase3FireRainCooldown)
	{
		AvailableSkills.Add(EBossPhase3SkillType::FireRain);
	}

	if (bEnableBossPhase3IceSpear && CurrentTime - LastBossPhase3IceSpearTime >= BossPhase3IceSpearCooldown)
	{
		AvailableSkills.Add(EBossPhase3SkillType::IceSpear);
	}

	if (bEnableBossPhase3Thunder && CurrentTime - LastBossPhase3ThunderTime >= BossPhase3ThunderCooldown)
	{
		AvailableSkills.Add(EBossPhase3SkillType::Thunder);
	}

	if (bEnableBossPhase3BlinkSlash && CurrentTime - LastBossRandomSlashTime >= BossRandomSlashCooldown)
	{
		if (AreBossPhase3SummonedMinionsAllDead())
		{
			AvailableSkills.Add(EBossPhase3SkillType::BlinkSlash);
			bHasLoggedBossPhase3BlinkSlashLocked = false;
		}
		else if (!bHasLoggedBossPhase3BlinkSlashLocked)
		{
			bHasLoggedBossPhase3BlinkSlashLocked = true;
			UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 BlinkSlash locked: summoned minions still alive."));
		}
	}

	if (AvailableSkills.Num() == 0)
	{
		return false;
	}

	if (AvailableSkills.Num() > 1 && bHasLastBossPhase3SkillUsed)
	{
		AvailableSkills.Remove(LastBossPhase3SkillUsed);
	}

	const EBossPhase3SkillType SelectedSkill = AvailableSkills[FMath::RandRange(0, AvailableSkills.Num() - 1)];
	switch (SelectedSkill)
	{
	case EBossPhase3SkillType::FireRain:
		StartBossPhase3FireRain(TargetActor);
		LastBossPhase3SkillUsed = SelectedSkill;
		bHasLastBossPhase3SkillUsed = true;
		UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 random skill selected: FireRain"));
		return true;
	case EBossPhase3SkillType::IceSpear:
		StartBossPhase3IceSpear(TargetActor);
		LastBossPhase3SkillUsed = SelectedSkill;
		bHasLastBossPhase3SkillUsed = true;
		UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 random skill selected: IceSpear"));
		return true;
	case EBossPhase3SkillType::Thunder:
		StartBossPhase3Thunder(TargetActor);
		LastBossPhase3SkillUsed = SelectedSkill;
		bHasLastBossPhase3SkillUsed = true;
		UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 random skill selected: Thunder"));
		return true;
	case EBossPhase3SkillType::BlinkSlash:
		StartBossRandomSlash(TargetActor);
		LastBossPhase3SkillUsed = SelectedSkill;
		bHasLastBossPhase3SkillUsed = true;
		UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 random skill selected: BlinkSlash"));
		return true;
	default:
		return false;
	}
}

bool AARPGEnemyBase::AreBossPhase3SummonedMinionsAllDead() const
{
	if (!bHasBossPhase3SummonedMinions)
	{
		return false;
	}

	if (BossPhase3SummonedMinions.Num() < BossPhase3SummonedMinionTargetCount)
	{
		return false;
	}

	for (const TWeakObjectPtr<AARPGEnemyBase>& MinionPtr : BossPhase3SummonedMinions)
	{
		if (const AARPGEnemyBase* Minion = MinionPtr.Get())
		{
			if (!Minion->IsDead())
			{
				return false;
			}
		}
	}

	return true;
}

void AARPGEnemyBase::StartBossPhase3SkillRecovery()
{
	if (!GetWorld() || bIsDead)
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	bIsBossPhase3SkillRecovering = true;
	BossPhase3SkillRecoveryEndTime = CurrentTime + FMath::Max(0.f, BossPhase3SkillRecovery);
	NextBossPhase3SkillAllowedTime = CurrentTime + FMath::Max(0.f, BossPhase3GlobalSkillGap);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 skill recovery started"));
}

void AARPGEnemyBase::HandleBossPhase3SkillRecovery(float DeltaTime)
{
	if (bIsDead || !GetWorld())
	{
		bIsBossPhase3SkillRecovering = false;
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	if (GetWorld()->GetTimeSeconds() >= BossPhase3SkillRecoveryEndTime)
	{
		bIsBossPhase3SkillRecovering = false;
		UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 skill recovery ended"));
	}
}

float AARPGEnemyBase::DistancePointToSegment2D(const FVector& Point, const FVector& SegmentStart, const FVector& SegmentEnd) const
{
	FVector Point2D(Point.X, Point.Y, 0.f);
	FVector Start2D(SegmentStart.X, SegmentStart.Y, 0.f);
	FVector End2D(SegmentEnd.X, SegmentEnd.Y, 0.f);
	const FVector Segment = End2D - Start2D;
	const float SegmentLengthSq = Segment.SizeSquared();
	if (SegmentLengthSq <= KINDA_SMALL_NUMBER)
	{
		return FVector::Dist2D(Point2D, Start2D);
	}

	const float T = FMath::Clamp(FVector::DotProduct(Point2D - Start2D, Segment) / SegmentLengthSq, 0.f, 1.f);
	const FVector ClosestPoint = Start2D + Segment * T;
	return FVector::Dist2D(Point2D, ClosestPoint);
}

void AARPGEnemyBase::EnterBossPhase3()
{
	if (!bIsBoss || bIsDead || bHasEnteredPhase3)
	{
		return;
	}

	bHasEnteredPhase3 = true;
	BossPhase = EARPGBossPhase::Phase3;
	NextBossPhase3SkillAllowedTime = 0.f;
	bIsBossPhase3SkillRecovering = false;
	BossPhase3SkillRecoveryEndTime = 0.f;
	bHasLastBossPhase3SkillUsed = false;
	bHasLoggedBossPhase3BlinkSlashLocked = false;
	bIsPreparingAttack = false;
	PendingAttackTarget = nullptr;
	bIsPreparingBossSlam = false;
	PendingBossSlamTarget = nullptr;
	bIsPreparingBossCone = false;
	PendingBossConeTarget = nullptr;
	bIsPreparingBossBarrage = false;
	PendingBossBarrageTarget = nullptr;
	bIsBossBarrageActive = false;
	bBossBarrageHitPlayerThisWave = false;
	ActiveBossBarrageOrbs.Empty();
	bIsPreparingBossLongSlash = false;
	bIsRecoveringBossLongSlash = false;
	bIsPreparingBossRandomSlash = false;
	bIsExecutingBossRandomSlash = false;
	bIsPreparingMageAttack = false;
	bIsPreparingThrowerAttack = false;
	MageAttackResolveTime = 0.f;
	MageAttackDirection = FVector::ZeroVector;
	ThrowerAttackResolveTime = 0.f;
	PendingThrowerTargetLocation = FVector::ZeroVector;
	ActiveMageProjectiles.Empty();
	ActiveThrowerFireZones.Empty();
	bIsBossPhase2WaveAttackActive = false;
	ActiveBossPhase2WaveOrbs.Empty();
	BossPhase2WaveCurrentLoop = 0;
	bBossPhase2WaveHitPlayerThisFrameOrWave = false;
	bIsCastingBossPhase3FireRain = false;
	bIsPreparingBossPhase3IceSpear = false;
	bIsPreparingBossPhase3Thunder = false;
	ActiveBossPhase3IceSpears.Empty();
	ActiveBossPhase3ThunderOrbs.Empty();
	BossPhase3IceSpearBaseDirection = FVector::ZeroVector;
	PendingBossLongSlashTarget = nullptr;
	BossLongSlashDirection = FVector::ZeroVector;
	BossRandomSlashExecutedCount = 0;
	BossRandomSlashBaseDirection = FVector::ZeroVector;
	BossPhase3SummonedMinions.Empty();

	UE_LOG(LogMyGame, Log, TEXT("Boss entered Phase 3"));
	StartBossPhase3Ultimate();
}

void AARPGEnemyBase::StartEnemyAttack(AActor* TargetActor, float DeltaTime)
{
	if (bIsDead || bIsPreparingAttack || !TargetActor || !GetWorld())
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastEnemyAttackTime < EnemyAttackCooldown)
	{
		return;
	}

	LastEnemyAttackTime = CurrentTime;
	bIsPreparingAttack = true;
	PendingAttackTarget = TargetActor;
	EnemyAttackResolveTime = CurrentTime + EnemyAttackWindup;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	FVector AttackDirection = TargetActor->GetActorLocation() - GetActorLocation();
	AttackDirection.Z = 0.f;
	FaceDirection(AttackDirection, DeltaTime);

	UE_LOG(LogMyGame, Log, TEXT("Enemy attack windup started: %s"), *GetName());
	DrawEnemyAttackRangeDebug(EnemyAttackDebugDuration);
}

void AARPGEnemyBase::ResolveEnemyAttack()
{
	if (bIsDead)
	{
		bIsPreparingAttack = false;
		PendingAttackTarget = nullptr;
		return;
	}

	bIsPreparingAttack = false;

	AActor* TargetActor = PendingAttackTarget.Get();
	PendingAttackTarget = nullptr;
	if (!TargetActor)
	{
		return;
	}

	FVector EnemyLocation = GetActorLocation();
	FVector TargetLocation = TargetActor->GetActorLocation();
	EnemyLocation.Z = 0.f;
	TargetLocation.Z = 0.f;

	const float DistanceToTarget = FVector::Dist2D(EnemyLocation, TargetLocation);
	FVector AttackDirection = TargetLocation - EnemyLocation;
	AttackDirection.Z = 0.f;
	FaceDirection(AttackDirection, 0.1f);

	if (DistanceToTarget > AttackRange + 30.f)
	{
		UE_LOG(LogMyGame, Log, TEXT("Enemy attack missed: %s"), *GetName());
		return;
	}

	UARPGHealthComponent* PlayerHealthComponent = TargetActor->FindComponentByClass<UARPGHealthComponent>();
	if (!PlayerHealthComponent || PlayerHealthComponent->IsDead())
	{
		UE_LOG(LogMyGame, Log, TEXT("Enemy attack missed: %s"), *GetName());
		return;
	}

	PlayerHealthComponent->ApplyDamage(EnemyAttackDamage);
	UE_LOG(LogMyGame, Log, TEXT("Enemy attack hit player: %s"), *GetName());
	UE_LOG(LogMyGame, Log, TEXT("Player took damage: %.1f"), EnemyAttackDamage);
	DrawDebugSphere(GetWorld(), TargetActor->GetActorLocation(), 55.f, 16, FColor::Red, false, EnemyAttackDebugDuration);

	if (PlayerHealthComponent->IsDead())
	{
		UE_LOG(LogMyGame, Log, TEXT("Player died"));
	}
}

void AARPGEnemyBase::StartBossSlam(AActor* TargetActor)
{
	if (!bIsBoss || !bEnableBossSlam || bIsDead || bIsPreparingBossSlam || bIsPreparingAttack || !TargetActor || !GetWorld())
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastBossSlamTime < BossSlamCooldown)
	{
		return;
	}

	bIsPreparingBossSlam = true;
	PendingBossSlamTarget = TargetActor;
	BossSlamResolveTime = CurrentTime + BossSlamWindup;
	LastBossSlamTime = CurrentTime;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	FVector SlamDirection = TargetActor->GetActorLocation() - GetActorLocation();
	SlamDirection.Z = 0.f;
	if (!SlamDirection.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.f, SlamDirection.Rotation().Yaw, 0.f));
	}

	UE_LOG(LogMyGame, Log, TEXT("Boss Slam windup started"));
	DrawBossSlamRangeDebug(0.05f);
}

void AARPGEnemyBase::ResolveBossSlam()
{
	bIsPreparingBossSlam = false;

	AActor* TargetActor = PendingBossSlamTarget.Get();
	PendingBossSlamTarget = nullptr;

	if (bIsDead || !TargetActor || !GetWorld())
	{
		return;
	}

	FVector BossLocation = GetActorLocation();
	FVector TargetLocation = TargetActor->GetActorLocation();
	BossLocation.Z = 0.f;
	TargetLocation.Z = 0.f;

	const float DistanceToTarget = FVector::Dist2D(BossLocation, TargetLocation);
	if (DistanceToTarget > BossSlamRange)
	{
		UE_LOG(LogMyGame, Log, TEXT("Boss Slam missed"));
		return;
	}

	UARPGHealthComponent* PlayerHealthComponent = TargetActor->FindComponentByClass<UARPGHealthComponent>();
	if (!PlayerHealthComponent || PlayerHealthComponent->IsDead())
	{
		UE_LOG(LogMyGame, Log, TEXT("Boss Slam missed"));
		return;
	}

	PlayerHealthComponent->ApplyDamage(BossSlamDamage);
	UE_LOG(LogMyGame, Log, TEXT("Boss Slam hit player"));
	DrawDebugSphere(GetWorld(), TargetActor->GetActorLocation(), 55.f, 16, FColor::Red, false, 0.3f);
}

void AARPGEnemyBase::StartBossConeStrike(AActor* TargetActor)
{
	if (!bIsBoss
		|| BossPhase != EARPGBossPhase::Phase1
		|| !bEnableBossConeStrike
		|| bIsDead
		|| bIsPreparingBossCone
		|| bIsPreparingBossBarrage
		|| bIsBossBarrageActive
		|| bIsPreparingAttack
		|| !TargetActor
		|| !GetWorld())
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastBossConeTime < BossConeCooldown)
	{
		return;
	}

	bIsPreparingBossCone = true;
	PendingBossConeTarget = TargetActor;
	BossConeResolveTime = CurrentTime + BossConeWindup;
	LastBossConeTime = CurrentTime;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	FVector ConeDirection = TargetActor->GetActorLocation() - GetActorLocation();
	ConeDirection.Z = 0.f;
	if (!ConeDirection.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.f, ConeDirection.Rotation().Yaw, 0.f));
	}

	UE_LOG(LogMyGame, Log, TEXT("Boss Cone Strike windup started"));
	DrawBossConeStrikeDebug(0.05f);
}

void AARPGEnemyBase::ResolveBossConeStrike()
{
	bIsPreparingBossCone = false;

	AActor* TargetActor = PendingBossConeTarget.Get();
	PendingBossConeTarget = nullptr;

	if (bIsDead || !TargetActor || !GetWorld())
	{
		return;
	}

	FVector BossLocation = GetActorLocation();
	FVector TargetLocation = TargetActor->GetActorLocation();
	BossLocation.Z = 0.f;
	TargetLocation.Z = 0.f;

	const float DistanceToTarget = FVector::Dist2D(BossLocation, TargetLocation);
	if (DistanceToTarget > BossConeRange)
	{
		UE_LOG(LogMyGame, Log, TEXT("Boss Cone Strike missed"));
		return;
	}

	FVector ToTarget = TargetLocation - BossLocation;
	ToTarget.Z = 0.f;
	FVector Forward = GetActorForwardVector();
	Forward.Z = 0.f;

	if (ToTarget.IsNearlyZero() || Forward.IsNearlyZero())
	{
		UE_LOG(LogMyGame, Log, TEXT("Boss Cone Strike missed"));
		return;
	}

	ToTarget.Normalize();
	Forward.Normalize();

	const float Dot = FMath::Clamp(FVector::DotProduct(Forward, ToTarget), -1.f, 1.f);
	const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(Dot));
	if (AngleDegrees > BossConeAngleDegrees * 0.5f)
	{
		UE_LOG(LogMyGame, Log, TEXT("Boss Cone Strike missed"));
		return;
	}

	UARPGHealthComponent* PlayerHealthComponent = TargetActor->FindComponentByClass<UARPGHealthComponent>();
	if (!PlayerHealthComponent || PlayerHealthComponent->IsDead())
	{
		UE_LOG(LogMyGame, Log, TEXT("Boss Cone Strike missed"));
		return;
	}

	PlayerHealthComponent->ApplyDamage(BossConeDamage);
	APawn* TargetPawn = Cast<APawn>(TargetActor);
	if (AARPGPlayerController* ARPGPlayerController = Cast<AARPGPlayerController>(TargetPawn ? TargetPawn->GetController() : nullptr))
	{
		ARPGPlayerController->ApplyMoveSpeedSlow(BossConeSlowMultiplier, BossConeSlowDuration);
	}

	UE_LOG(LogMyGame, Log, TEXT("Boss Cone Strike hit player, slow applied"));
	DrawDebugSphere(GetWorld(), TargetActor->GetActorLocation(), 55.f, 16, FColor::Yellow, false, 0.3f);
}

void AARPGEnemyBase::DrawBossConeStrikeDebug(float Duration) const
{
	if (!GetWorld())
	{
		return;
	}

	FVector Origin = GetActorLocation();
	Origin.Z += 20.f;

	FVector Forward = GetActorForwardVector();
	Forward.Z = 0.f;
	Forward = Forward.IsNearlyZero() ? FVector::ForwardVector : Forward.GetSafeNormal();

	const float HalfAngle = BossConeAngleDegrees * 0.5f;
	const FVector CenterEnd = Origin + Forward * BossConeRange;
	const FVector LeftEnd = Origin + Forward.RotateAngleAxis(-HalfAngle, FVector::UpVector) * BossConeRange;
	const FVector RightEnd = Origin + Forward.RotateAngleAxis(HalfAngle, FVector::UpVector) * BossConeRange;

	DrawDebugLine(GetWorld(), Origin, CenterEnd, FColor::Yellow, false, Duration, 0, 2.f);
	DrawDebugLine(GetWorld(), Origin, LeftEnd, FColor::Yellow, false, Duration, 0, 2.f);
	DrawDebugLine(GetWorld(), Origin, RightEnd, FColor::Yellow, false, Duration, 0, 2.f);

	FVector PreviousPoint = LeftEnd;
	const int32 SegmentCount = 8;
	for (int32 Index = 1; Index <= SegmentCount; ++Index)
	{
		const float Alpha = static_cast<float>(Index) / static_cast<float>(SegmentCount);
		const float Angle = FMath::Lerp(-HalfAngle, HalfAngle, Alpha);
		const FVector ArcPoint = Origin + Forward.RotateAngleAxis(Angle, FVector::UpVector) * BossConeRange;
		DrawDebugLine(GetWorld(), PreviousPoint, ArcPoint, FColor::Yellow, false, Duration, 0, 2.f);
		DrawDebugSphere(GetWorld(), ArcPoint, 12.f, 8, FColor::Yellow, false, Duration);
		PreviousPoint = ArcPoint;
	}
}

void AARPGEnemyBase::StartBossBarrage(AActor* TargetActor)
{
	if (!bIsBoss
		|| BossPhase != EARPGBossPhase::Phase1
		|| !bEnableBossBarrage
		|| bIsDead
		|| bIsPreparingBossCone
		|| bIsPreparingBossBarrage
		|| bIsPreparingBossSlam
		|| bIsBossBarrageActive
		|| bIsPreparingAttack
		|| !TargetActor
		|| !GetWorld())
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastBossBarrageTime < BossBarrageCooldown)
	{
		return;
	}

	bIsPreparingBossBarrage = true;
	PendingBossBarrageTarget = TargetActor;
	BossBarrageResolveTime = CurrentTime + BossBarrageWindup;
	LastBossBarrageTime = CurrentTime;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	FVector BarrageDirection = TargetActor->GetActorLocation() - GetActorLocation();
	BarrageDirection.Z = 0.f;
	if (!BarrageDirection.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.f, BarrageDirection.Rotation().Yaw, 0.f));
	}

	UE_LOG(LogMyGame, Log, TEXT("Boss Barrage windup started"));
	DrawBossBarrageDebug(0.05f);
}

void AARPGEnemyBase::LaunchBossBarrage()
{
	bIsPreparingBossBarrage = false;
	PendingBossBarrageTarget = nullptr;

	if (bIsDead || !GetWorld())
	{
		return;
	}

	ActiveBossBarrageOrbs.Empty();
	bIsBossBarrageActive = true;
	bBossBarrageHitPlayerThisWave = false;

	const int32 OrbCount = FMath::Max(1, BossBarrageOrbCount);
	FVector BossLocation = GetActorLocation();
	BossLocation.Z += 35.f;

	ActiveBossBarrageOrbs.Reserve(OrbCount);
	for (int32 Index = 0; Index < OrbCount; ++Index)
	{
		const float AngleRadians = FMath::DegreesToRadians(360.f * static_cast<float>(Index) / static_cast<float>(OrbCount));
		FVector Direction(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians), 0.f);
		Direction = Direction.GetSafeNormal();

		FARPGBossBarrageOrb Orb;
		Orb.Location = BossLocation + Direction * 80.f;
		Orb.Direction = Direction;
		Orb.TraveledDistance = 0.f;
		Orb.bActive = true;
		ActiveBossBarrageOrbs.Add(Orb);
	}

	UE_LOG(LogMyGame, Log, TEXT("Boss Barrage launched"));
}

void AARPGEnemyBase::UpdateBossBarrageOrbs(float DeltaTime)
{
	if (bIsDead || !GetWorld())
	{
		ActiveBossBarrageOrbs.Empty();
		bIsBossBarrageActive = false;
		return;
	}

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UARPGHealthComponent* PlayerHealthComponent = PlayerPawn ? PlayerPawn->FindComponentByClass<UARPGHealthComponent>() : nullptr;
	const FVector PlayerLocation = PlayerPawn ? PlayerPawn->GetActorLocation() : FVector::ZeroVector;

	bool bAnyOrbActive = false;
	for (FARPGBossBarrageOrb& Orb : ActiveBossBarrageOrbs)
	{
		if (!Orb.bActive)
		{
			continue;
		}

		const FVector DeltaMove = Orb.Direction * BossBarrageOrbSpeed * DeltaTime;
		Orb.Location += DeltaMove;
		Orb.TraveledDistance += DeltaMove.Size();

		DrawDebugSphere(GetWorld(), Orb.Location, BossBarrageOrbRadius, 12, FColor::Cyan, false, 0.05f, 0, 2.f);

		if (Orb.TraveledDistance >= BossBarrageMaxDistance)
		{
			Orb.bActive = false;
			continue;
		}

		bAnyOrbActive = true;

		if (PlayerPawn && PlayerHealthComponent && !bBossBarrageHitPlayerThisWave)
		{
			FVector OrbLocation2D = Orb.Location;
			FVector PlayerLocation2D = PlayerLocation;
			OrbLocation2D.Z = 0.f;
			PlayerLocation2D.Z = 0.f;

			if (FVector::Dist2D(OrbLocation2D, PlayerLocation2D) <= BossBarrageOrbRadius + 40.f)
			{
				PlayerHealthComponent->ApplyDamage(BossBarrageDamage);
				bBossBarrageHitPlayerThisWave = true;
				UE_LOG(LogMyGame, Log, TEXT("Boss Barrage orb hit player"));
				DrawDebugSphere(GetWorld(), PlayerPawn->GetActorLocation(), 55.f, 16, FColor::Red, false, 0.3f);
			}
		}
	}

	if (!bAnyOrbActive)
	{
		ActiveBossBarrageOrbs.Empty();
		bIsBossBarrageActive = false;
		bBossBarrageHitPlayerThisWave = false;
		UE_LOG(LogMyGame, Log, TEXT("Boss Barrage ended"));
	}
}

void AARPGEnemyBase::DrawBossBarrageDebug(float Duration) const
{
	if (!GetWorld())
	{
		return;
	}

	FVector Origin = GetActorLocation();
	Origin.Z += 20.f;

	const int32 SegmentCount = 48;
	FVector PreviousPoint = Origin + FVector(BossBarrageMaxDistance, 0.f, 0.f);
	for (int32 Index = 1; Index <= SegmentCount; ++Index)
	{
		const float AngleRadians = FMath::DegreesToRadians(360.f * static_cast<float>(Index) / static_cast<float>(SegmentCount));
		const FVector CurrentPoint = Origin + FVector(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians), 0.f) * BossBarrageMaxDistance;
		DrawDebugLine(GetWorld(), PreviousPoint, CurrentPoint, FColor::Cyan, false, Duration, 0, 2.f);
		PreviousPoint = CurrentPoint;
	}
	DrawDebugSphere(GetWorld(), Origin, BossBarrageOrbRadius, 12, FColor::Cyan, false, Duration, 0, 1.f);
}

void AARPGEnemyBase::StartBossLongSlash(AActor* TargetActor)
{
	if (!bIsBoss
		|| BossPhase != EARPGBossPhase::Phase2
		|| !bEnableBossLongSlash
		|| bIsDead
		|| bIsPreparingAttack
		|| bIsPreparingBossCone
		|| bIsPreparingBossBarrage
		|| bIsPreparingBossSlam
		|| bIsBossBarrageActive
		|| bIsBossPhase2WaveAttackActive
		|| bIsPreparingBossLongSlash
		|| bIsRecoveringBossLongSlash
		|| bIsPreparingBossRandomSlash
		|| bIsExecutingBossRandomSlash
		|| !TargetActor
		|| !GetWorld())
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastBossLongSlashTime < BossLongSlashCooldown)
	{
		return;
	}

	FVector SlashDirection = TargetActor->GetActorLocation() - GetActorLocation();
	SlashDirection.Z = 0.f;
	if (SlashDirection.IsNearlyZero())
	{
		SlashDirection = GetActorForwardVector();
		SlashDirection.Z = 0.f;
	}
	SlashDirection = SlashDirection.IsNearlyZero() ? FVector::ForwardVector : SlashDirection.GetSafeNormal();

	bIsPreparingBossLongSlash = true;
	PendingBossLongSlashTarget = TargetActor;
	BossLongSlashDirection = SlashDirection;
	BossLongSlashResolveTime = CurrentTime + BossLongSlashWindup;
	LastBossLongSlashTime = CurrentTime;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	SetActorRotation(FRotator(0.f, BossLongSlashDirection.Rotation().Yaw, 0.f));
	UE_LOG(LogMyGame, Log, TEXT("Boss Long Slash windup started"));
	DrawBossLongSlashDebug(0.05f);
}

void AARPGEnemyBase::ResolveBossLongSlash()
{
	bIsPreparingBossLongSlash = false;

	AActor* TargetActor = PendingBossLongSlashTarget.Get();
	PendingBossLongSlashTarget = nullptr;

	if (bIsDead || !GetWorld())
	{
		return;
	}

	bool bHitPlayer = false;
	if (TargetActor)
	{
		FVector BossLocation = GetActorLocation();
		FVector TargetLocation = TargetActor->GetActorLocation();
		BossLocation.Z = 0.f;
		TargetLocation.Z = 0.f;

		FVector Direction = BossLongSlashDirection;
		Direction.Z = 0.f;
		Direction = Direction.IsNearlyZero() ? GetActorForwardVector() : Direction.GetSafeNormal();
		Direction.Z = 0.f;
		Direction = Direction.IsNearlyZero() ? FVector::ForwardVector : Direction.GetSafeNormal();

		const FVector RightVector = FVector::CrossProduct(FVector::UpVector, Direction).GetSafeNormal();
		const FVector PlayerVector = TargetLocation - BossLocation;
		const float ForwardDistance = FVector::DotProduct(PlayerVector, Direction);
		const float RightDistance = FMath::Abs(FVector::DotProduct(PlayerVector, RightVector));

		bHitPlayer = ForwardDistance >= 0.f
			&& ForwardDistance <= BossLongSlashRange
			&& RightDistance <= BossLongSlashWidth * 0.5f;

		if (bHitPlayer)
		{
			UARPGHealthComponent* PlayerHealthComponent = TargetActor->FindComponentByClass<UARPGHealthComponent>();
			if (PlayerHealthComponent && !PlayerHealthComponent->IsDead())
			{
				PlayerHealthComponent->ApplyDamage(BossLongSlashDamage);
				UE_LOG(LogMyGame, Log, TEXT("Boss Long Slash hit player"));
				DrawDebugSphere(GetWorld(), TargetActor->GetActorLocation(), 55.f, 16, FColor::Red, false, 0.3f);
			}
			else
			{
				bHitPlayer = false;
			}
		}
	}

	if (!bHitPlayer)
	{
		UE_LOG(LogMyGame, Log, TEXT("Boss Long Slash missed"));
	}

	bIsRecoveringBossLongSlash = true;
	BossLongSlashRecoveryEndTime = GetWorld()->GetTimeSeconds() + BossLongSlashRecovery;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	UE_LOG(LogMyGame, Log, TEXT("Boss Long Slash recovery started"));
}

void AARPGEnemyBase::DrawBossLongSlashDebug(float Duration) const
{
	if (!GetWorld())
	{
		return;
	}

	FVector Origin = GetActorLocation();
	Origin.Z += 20.f;

	FVector Direction = BossLongSlashDirection;
	Direction.Z = 0.f;
	Direction = Direction.IsNearlyZero() ? GetActorForwardVector() : Direction.GetSafeNormal();
	Direction.Z = 0.f;
	Direction = Direction.IsNearlyZero() ? FVector::ForwardVector : Direction.GetSafeNormal();

	const FVector RightVector = FVector::CrossProduct(FVector::UpVector, Direction).GetSafeNormal();
	const FVector Start = Origin + Direction * 80.f;
	const FVector End = Origin + Direction * BossLongSlashRange;
	const FVector WidthOffset = RightVector * (BossLongSlashWidth * 0.5f);

	DrawDebugLine(GetWorld(), Start, End, FColor::Magenta, false, Duration, 0, 3.f);
	DrawDebugLine(GetWorld(), Start + WidthOffset, End + WidthOffset, FColor::Magenta, false, Duration, 0, 2.f);
	DrawDebugLine(GetWorld(), Start - WidthOffset, End - WidthOffset, FColor::Magenta, false, Duration, 0, 2.f);
	DrawDebugLine(GetWorld(), Start + WidthOffset, Start - WidthOffset, FColor::Magenta, false, Duration, 0, 2.f);
	DrawDebugLine(GetWorld(), End + WidthOffset, End - WidthOffset, FColor::Magenta, false, Duration, 0, 2.f);
}

void AARPGEnemyBase::StartBossRandomSlash(AActor* TargetActor)
{
	const bool bCanUsePhase2RandomSlash = BossPhase == EARPGBossPhase::Phase2 && bEnableBossRandomSlash;
	const bool bCanUsePhase3BlinkSlash = BossPhase == EARPGBossPhase::Phase3
		&& bEnableBossPhase3BlinkSlash
		&& AreBossPhase3SummonedMinionsAllDead();

	if (!bIsBoss
		|| (!bCanUsePhase2RandomSlash && !bCanUsePhase3BlinkSlash)
		|| bIsDead
		|| bIsPreparingAttack
		|| bIsPreparingBossCone
		|| bIsPreparingBossBarrage
		|| bIsPreparingBossSlam
		|| bIsBossBarrageActive
		|| bIsBossPhase2WaveAttackActive
		|| bIsBossPhase3UltimateActive
		|| bIsCastingBossPhase3FireRain
		|| bIsPreparingBossPhase3IceSpear
		|| bIsPreparingBossPhase3Thunder
		|| bIsBossPhase3SkillRecovering
		|| bIsPreparingBossLongSlash
		|| bIsRecoveringBossLongSlash
		|| bIsPreparingBossRandomSlash
		|| bIsExecutingBossRandomSlash
		|| !TargetActor
		|| !GetWorld())
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastBossRandomSlashTime < BossRandomSlashCooldown)
	{
		return;
	}

	if (BossPhase == EARPGBossPhase::Phase3)
	{
		FVector TargetForward = TargetActor->GetActorForwardVector();
		TargetForward.Z = 0.f;
		if (TargetForward.IsNearlyZero())
		{
			TargetForward = TargetActor->GetActorLocation() - GetActorLocation();
			TargetForward.Z = 0.f;
		}
		TargetForward = TargetForward.IsNearlyZero() ? FVector::ForwardVector : TargetForward.GetSafeNormal();

		FVector BlinkLocation = TargetActor->GetActorLocation() - TargetForward * FMath::Max(120.f, AttackRange);
		BlinkLocation.Z = TargetActor->GetActorLocation().Z;
		SetActorLocation(BlinkLocation, false, nullptr, ETeleportType::TeleportPhysics);
		UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 BlinkSlash teleported behind player"));
	}

	FVector BaseDirection = TargetActor->GetActorLocation() - GetActorLocation();
	BaseDirection.Z = 0.f;
	if (BaseDirection.IsNearlyZero())
	{
		BaseDirection = GetActorForwardVector();
		BaseDirection.Z = 0.f;
	}
	BaseDirection = BaseDirection.IsNearlyZero() ? FVector::ForwardVector : BaseDirection.GetSafeNormal();

	bIsPreparingBossRandomSlash = true;
	LastBossRandomSlashTime = CurrentTime;
	BossRandomSlashStartTime = CurrentTime + BossRandomSlashWindup;
	BossRandomSlashNextTime = BossRandomSlashStartTime;
	BossRandomSlashExecutedCount = 0;
	BossRandomSlashBaseDirection = BaseDirection;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	SetActorRotation(FRotator(0.f, BossRandomSlashBaseDirection.Rotation().Yaw, 0.f));
	UE_LOG(LogMyGame, Log, TEXT("Boss Random Slash windup started"));
	DrawBossRandomSlashFanDebug(0.05f);
}

void AARPGEnemyBase::ExecuteOneBossRandomSlash()
{
	if (bIsDead || !GetWorld())
	{
		return;
	}

	FVector BaseDirection = BossRandomSlashBaseDirection;
	BaseDirection.Z = 0.f;
	BaseDirection = BaseDirection.IsNearlyZero() ? GetActorForwardVector() : BaseDirection.GetSafeNormal();
	BaseDirection.Z = 0.f;
	BaseDirection = BaseDirection.IsNearlyZero() ? FVector::ForwardVector : BaseDirection.GetSafeNormal();

	const float HalfFanAngle = BossRandomSlashFanAngleDegrees * 0.5f;
	const float RandomAngle = FMath::FRandRange(-HalfFanAngle, HalfFanAngle);
	FVector SlashDirection = BaseDirection.RotateAngleAxis(RandomAngle, FVector::UpVector);
	SlashDirection.Z = 0.f;
	SlashDirection = SlashDirection.IsNearlyZero() ? BaseDirection : SlashDirection.GetSafeNormal();

	DrawBossRandomSlashStrikeDebug(SlashDirection, 0.45f);

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!PlayerPawn)
	{
		return;
	}

	FVector BossLocation = GetActorLocation();
	FVector PlayerLocation = PlayerPawn->GetActorLocation();
	BossLocation.Z = 0.f;
	PlayerLocation.Z = 0.f;

	const FVector RightVector = FVector::CrossProduct(FVector::UpVector, SlashDirection).GetSafeNormal();
	const FVector PlayerVector = PlayerLocation - BossLocation;
	const float ForwardDistance = FVector::DotProduct(PlayerVector, SlashDirection);
	const float RightDistance = FMath::Abs(FVector::DotProduct(PlayerVector, RightVector));

	const bool bHitPlayer = ForwardDistance >= 0.f
		&& ForwardDistance <= BossRandomSlashRange
		&& RightDistance <= BossRandomSlashWidth * 0.5f;

	if (!bHitPlayer)
	{
		return;
	}

	UARPGHealthComponent* PlayerHealthComponent = PlayerPawn->FindComponentByClass<UARPGHealthComponent>();
	if (!PlayerHealthComponent || PlayerHealthComponent->IsDead())
	{
		return;
	}

	PlayerHealthComponent->ApplyDamage(BossRandomSlashDamage);
	UE_LOG(LogMyGame, Log, TEXT("Boss Random Slash hit player"));
	DrawDebugSphere(GetWorld(), PlayerPawn->GetActorLocation(), 45.f, 12, FColor::Red, false, 0.3f);
}

void AARPGEnemyBase::DrawBossRandomSlashFanDebug(float Duration) const
{
	if (!GetWorld())
	{
		return;
	}

	FVector Origin = GetActorLocation();
	Origin.Z += 20.f;

	FVector BaseDirection = BossRandomSlashBaseDirection;
	BaseDirection.Z = 0.f;
	BaseDirection = BaseDirection.IsNearlyZero() ? GetActorForwardVector() : BaseDirection.GetSafeNormal();
	BaseDirection.Z = 0.f;
	BaseDirection = BaseDirection.IsNearlyZero() ? FVector::ForwardVector : BaseDirection.GetSafeNormal();

	const float HalfFanAngle = BossRandomSlashFanAngleDegrees * 0.5f;
	const FVector CenterEnd = Origin + BaseDirection * BossRandomSlashRange;
	const FVector LeftEnd = Origin + BaseDirection.RotateAngleAxis(-HalfFanAngle, FVector::UpVector) * BossRandomSlashRange;
	const FVector RightEnd = Origin + BaseDirection.RotateAngleAxis(HalfFanAngle, FVector::UpVector) * BossRandomSlashRange;

	DrawDebugLine(GetWorld(), Origin, CenterEnd, FColor::Blue, false, Duration, 0, 2.f);
	DrawDebugLine(GetWorld(), Origin, LeftEnd, FColor::Blue, false, Duration, 0, 2.f);
	DrawDebugLine(GetWorld(), Origin, RightEnd, FColor::Blue, false, Duration, 0, 2.f);

	FVector PreviousPoint = LeftEnd;
	const int32 SegmentCount = 16;
	for (int32 Index = 1; Index <= SegmentCount; ++Index)
	{
		const float Alpha = static_cast<float>(Index) / static_cast<float>(SegmentCount);
		const float Angle = FMath::Lerp(-HalfFanAngle, HalfFanAngle, Alpha);
		const FVector ArcPoint = Origin + BaseDirection.RotateAngleAxis(Angle, FVector::UpVector) * BossRandomSlashRange;
		DrawDebugLine(GetWorld(), PreviousPoint, ArcPoint, FColor::Blue, false, Duration, 0, 2.f);
		PreviousPoint = ArcPoint;
	}
}

void AARPGEnemyBase::DrawBossRandomSlashStrikeDebug(const FVector& SlashDirection, float Duration) const
{
	if (!GetWorld())
	{
		return;
	}

	FVector Origin = GetActorLocation();
	Origin.Z += 20.f;

	FVector Direction = SlashDirection;
	Direction.Z = 0.f;
	Direction = Direction.IsNearlyZero() ? FVector::ForwardVector : Direction.GetSafeNormal();
	const FVector RightVector = FVector::CrossProduct(FVector::UpVector, Direction).GetSafeNormal();
	const FVector Start = Origin + Direction * 80.f;
	const FVector End = Origin + Direction * BossRandomSlashRange;
	const FVector WidthOffset = RightVector * (BossRandomSlashWidth * 0.5f);

	DrawDebugLine(GetWorld(), Start, End, FColor::Blue, false, Duration, 0, 3.f);
	DrawDebugLine(GetWorld(), Start + WidthOffset, End + WidthOffset, FColor::Blue, false, Duration, 0, 2.f);
	DrawDebugLine(GetWorld(), Start - WidthOffset, End - WidthOffset, FColor::Blue, false, Duration, 0, 2.f);
	DrawDebugLine(GetWorld(), Start + WidthOffset, Start - WidthOffset, FColor::Blue, false, Duration, 0, 2.f);
	DrawDebugLine(GetWorld(), End + WidthOffset, End - WidthOffset, FColor::Blue, false, Duration, 0, 2.f);
}

void AARPGEnemyBase::StartBossPhase2WaveAttack()
{
	if (!bIsBoss || BossPhase != EARPGBossPhase::Phase2 || bIsDead || !GetWorld())
	{
		return;
	}

	bHasTriggeredPhase2HalfHealthWave = true;
	bIsBossPhase2WaveAttackActive = true;
	BossPhase2WaveCurrentLoop = 1;
	bBossPhase2WaveHitPlayerThisFrameOrWave = false;

	bIsPreparingAttack = false;
	PendingAttackTarget = nullptr;
	bIsPreparingBossSlam = false;
	PendingBossSlamTarget = nullptr;
	bIsPreparingBossCone = false;
	PendingBossConeTarget = nullptr;
	bIsPreparingBossBarrage = false;
	PendingBossBarrageTarget = nullptr;
	bIsBossBarrageActive = false;
	bBossBarrageHitPlayerThisWave = false;
	ActiveBossBarrageOrbs.Empty();
	bIsPreparingBossLongSlash = false;
	bIsRecoveringBossLongSlash = false;
	PendingBossLongSlashTarget = nullptr;
	BossLongSlashDirection = FVector::ZeroVector;
	bIsPreparingBossRandomSlash = false;
	bIsExecutingBossRandomSlash = false;
	BossRandomSlashExecutedCount = 0;
	BossRandomSlashBaseDirection = FVector::ZeroVector;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	UE_LOG(LogMyGame, Log, TEXT("Boss Phase2 half health wave attack started"));
	UE_LOG(LogMyGame, Log, TEXT("Boss became invincible"));
	LaunchBossPhase2WaveLoop();
}

void AARPGEnemyBase::LaunchBossPhase2WaveLoop()
{
	if (!GetWorld())
	{
		return;
	}

	ActiveBossPhase2WaveOrbs.Empty();

	const int32 WaveCount = FMath::Max(1, BossPhase2WaveCount);
	FVector BossLocation = GetActorLocation();
	BossLocation.Z += 35.f;

	ActiveBossPhase2WaveOrbs.Reserve(WaveCount);
	for (int32 Index = 0; Index < WaveCount; ++Index)
	{
		const float AngleRadians = FMath::DegreesToRadians(360.f * static_cast<float>(Index) / static_cast<float>(WaveCount));
		FVector Direction(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians), 0.f);
		Direction = Direction.GetSafeNormal();

		FARPGPhase2WaveOrb Orb;
		Orb.Location = BossLocation + Direction * BossPhase2WaveStartOffset;
		Orb.Direction = Direction;
		Orb.TraveledDistance = 0.f;
		Orb.bReturning = false;
		Orb.bActive = true;
		Orb.bHitPlayerThisTrip = false;
		ActiveBossPhase2WaveOrbs.Add(Orb);
	}

	UE_LOG(LogMyGame, Log, TEXT("Boss Phase2 wave loop launched"));
}

void AARPGEnemyBase::UpdateBossPhase2WaveAttack(float DeltaTime)
{
	if (bIsDead || !GetWorld())
	{
		ActiveBossPhase2WaveOrbs.Empty();
		bIsBossPhase2WaveAttackActive = false;
		BossPhase2WaveCurrentLoop = 0;
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	DrawDebugSphere(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 95.f), 105.f, 24, FColor::Purple, false, 0.05f, 0, 3.f);

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UARPGHealthComponent* PlayerHealthComponent = PlayerPawn ? PlayerPawn->FindComponentByClass<UARPGHealthComponent>() : nullptr;
	const FVector PlayerLocation = PlayerPawn ? PlayerPawn->GetActorLocation() : FVector::ZeroVector;
	bool bPlayerDamagedThisUpdate = false;
	bBossPhase2WaveHitPlayerThisFrameOrWave = false;

	bool bAnyOrbActive = false;
	const FVector BossReturnLocation = GetActorLocation() + FVector(0.f, 0.f, 35.f);
	for (FARPGPhase2WaveOrb& Orb : ActiveBossPhase2WaveOrbs)
	{
		if (!Orb.bActive)
		{
			continue;
		}

		if (!Orb.bReturning)
		{
			const float DeltaDistance = BossPhase2WaveSpeed * DeltaTime;
			Orb.Location += Orb.Direction * DeltaDistance;
			Orb.TraveledDistance += DeltaDistance;

			if (Orb.TraveledDistance >= BossPhase2WaveMaxDistance)
			{
				Orb.bReturning = true;
				Orb.bHitPlayerThisTrip = false;
			}
		}
		else
		{
			FVector ReturnDirection = BossReturnLocation - Orb.Location;
			const float DistanceToBoss = ReturnDirection.Size();
			if (DistanceToBoss <= BossPhase2WaveStartOffset)
			{
				Orb.bActive = false;
				continue;
			}

			ReturnDirection = ReturnDirection.GetSafeNormal();
			Orb.Location += ReturnDirection * BossPhase2WaveSpeed * DeltaTime;
		}

		DrawDebugSphere(GetWorld(), Orb.Location, BossPhase2WaveRadius, 12, FColor::Purple, false, 0.05f, 0, 2.f);
		bAnyOrbActive = true;

		if (PlayerPawn && PlayerHealthComponent && !Orb.bHitPlayerThisTrip && !bPlayerDamagedThisUpdate)
		{
			FVector OrbLocation2D = Orb.Location;
			FVector PlayerLocation2D = PlayerLocation;
			OrbLocation2D.Z = 0.f;
			PlayerLocation2D.Z = 0.f;

			if (FVector::Dist2D(OrbLocation2D, PlayerLocation2D) <= BossPhase2WaveRadius + 40.f)
			{
				PlayerHealthComponent->ApplyDamage(BossPhase2WaveDamage);
				Orb.bHitPlayerThisTrip = true;
				bPlayerDamagedThisUpdate = true;
				bBossPhase2WaveHitPlayerThisFrameOrWave = true;
				UE_LOG(LogMyGame, Log, TEXT("Boss Phase2 wave hit player"));
				DrawDebugSphere(GetWorld(), PlayerPawn->GetActorLocation(), 55.f, 16, FColor::Red, false, 0.3f);
			}
		}
	}

	if (!bAnyOrbActive)
	{
		if (BossPhase2WaveCurrentLoop < BossPhase2WaveLoopCount)
		{
			++BossPhase2WaveCurrentLoop;
			LaunchBossPhase2WaveLoop();
		}
		else
		{
			FinishBossPhase2WaveAttack();
		}
	}
}

void AARPGEnemyBase::FinishBossPhase2WaveAttack()
{
	bIsBossPhase2WaveAttackActive = false;
	ActiveBossPhase2WaveOrbs.Empty();
	BossPhase2WaveCurrentLoop = 0;
	bBossPhase2WaveHitPlayerThisFrameOrWave = false;

	UE_LOG(LogMyGame, Log, TEXT("Boss Phase2 wave attack finished"));
	UE_LOG(LogMyGame, Log, TEXT("Boss invincibility ended"));
}

void AARPGEnemyBase::StartBossPhase3Ultimate()
{
	if (!bIsBoss
		|| BossPhase != EARPGBossPhase::Phase3
		|| bIsDead
		|| !GetWorld()
		|| !bEnableBossPhase3Ultimate
		|| bHasStartedBossPhase3Ultimate)
	{
		return;
	}

	bHasStartedBossPhase3Ultimate = true;
	bIsBossPhase3UltimateActive = true;
	BossPhase3UltimateCurrentRound = 1;
	BossPhase3ArenaCenter = GetActorLocation();
	BossPhase3CurrentSafeZoneCenter = FVector::ZeroVector;

	bIsPreparingAttack = false;
	PendingAttackTarget = nullptr;
	bIsPreparingBossSlam = false;
	PendingBossSlamTarget = nullptr;
	bIsPreparingBossCone = false;
	PendingBossConeTarget = nullptr;
	bIsPreparingBossBarrage = false;
	PendingBossBarrageTarget = nullptr;
	bIsBossBarrageActive = false;
	bBossBarrageHitPlayerThisWave = false;
	ActiveBossBarrageOrbs.Empty();
	bIsPreparingBossLongSlash = false;
	bIsRecoveringBossLongSlash = false;
	PendingBossLongSlashTarget = nullptr;
	BossLongSlashDirection = FVector::ZeroVector;
	bIsPreparingBossRandomSlash = false;
	bIsExecutingBossRandomSlash = false;
	BossRandomSlashExecutedCount = 0;
	BossRandomSlashBaseDirection = FVector::ZeroVector;
	bIsBossPhase2WaveAttackActive = false;
	ActiveBossPhase2WaveOrbs.Empty();
	BossPhase2WaveCurrentLoop = 0;
	bBossPhase2WaveHitPlayerThisFrameOrWave = false;
	bIsCastingBossPhase3FireRain = false;
	ActiveBossPhase3FireRainZones.Empty();
	bIsPreparingBossPhase3IceSpear = false;
	ActiveBossPhase3IceSpears.Empty();
	BossPhase3IceSpearBaseDirection = FVector::ZeroVector;
	bIsPreparingBossPhase3Thunder = false;
	ActiveBossPhase3ThunderOrbs.Empty();
	bIsBossPhase3SkillRecovering = false;
	NextBossPhase3SkillAllowedTime = 0.f;
	BossPhase3SkillRecoveryEndTime = 0.f;
	bHasLastBossPhase3SkillUsed = false;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	GenerateBossPhase3SafeZone();
	BossPhase3UltimateExplosionTime = GetWorld()->GetTimeSeconds() + BossPhase3UltimateFirstDelay;

	UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 ultimate started"));
	UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 safe zone round 1 generated"));
}

void AARPGEnemyBase::GenerateBossPhase3SafeZone()
{
	if (!GetWorld())
	{
		return;
	}

	const FVector BossLocation = GetActorLocation();
	const float SafeZoneRadius = FMath::Max(1.f, BossPhase3SafeZoneRadius);
	const float MaxSafeRadius = FMath::Max(0.f, BossPhase3ArenaRadius - SafeZoneRadius);
	const float DesiredMinDistance = FMath::Max(0.f, BossPhase3SafeZoneMinDistanceFromBoss);
	const float MinSafeRadius = FMath::Clamp(DesiredMinDistance, 0.f, FMath::Max(0.f, MaxSafeRadius - 50.f));
	FVector SafeZoneCenter = FVector::ZeroVector;
	bool bFoundSafeZone = false;

	if (MaxSafeRadius > KINDA_SMALL_NUMBER && MinSafeRadius <= MaxSafeRadius)
	{
		for (int32 Attempt = 0; Attempt < 20; ++Attempt)
		{
			const float RandomRadius = FMath::FRandRange(MinSafeRadius, MaxSafeRadius);
			const float RandomAngleRadians = FMath::DegreesToRadians(FMath::FRandRange(0.f, 360.f));
			FVector Candidate = BossPhase3ArenaCenter;
			Candidate.X += FMath::Cos(RandomAngleRadians) * RandomRadius;
			Candidate.Y += FMath::Sin(RandomAngleRadians) * RandomRadius;

			if (FVector::Dist2D(Candidate, BossLocation) >= MinSafeRadius
				&& FVector::Dist2D(Candidate, BossPhase3ArenaCenter) <= MaxSafeRadius)
			{
				SafeZoneCenter = Candidate;
				bFoundSafeZone = true;
				break;
			}
		}
	}

	if (!bFoundSafeZone)
	{
		FVector FallbackDirection = GetActorForwardVector() + GetActorRightVector() * 0.35f;
		FallbackDirection.Z = 0.f;
		FallbackDirection = FallbackDirection.IsNearlyZero() ? FVector::ForwardVector : FallbackDirection.GetSafeNormal();
		const float FallbackDistance = FMath::Clamp(DesiredMinDistance, 0.f, MaxSafeRadius);
		SafeZoneCenter = BossLocation + FallbackDirection * FallbackDistance;
	}

	if (!SafeZoneCenter.ContainsNaN())
	{
		SafeZoneCenter.Z = BossLocation.Z + BossPhase3SafeZoneZOffset;
	}
	else
	{
		SafeZoneCenter = BossLocation + FVector(FMath::Min(DesiredMinDistance, MaxSafeRadius), 0.f, BossPhase3SafeZoneZOffset);
	}

	BossPhase3CurrentSafeZoneCenter = SafeZoneCenter;
	UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 safe zone generated. DistanceToBoss = %.1f, Radius = %.1f"),
		FVector::Dist2D(BossPhase3CurrentSafeZoneCenter, BossLocation),
		SafeZoneRadius);
}

void AARPGEnemyBase::UpdateBossPhase3Ultimate(float DeltaTime)
{
	if (bIsDead || !GetWorld())
	{
		bIsBossPhase3UltimateActive = false;
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (PlayerPawn)
	{
		FVector FacingDirection = PlayerPawn->GetActorLocation() - GetActorLocation();
		FacingDirection.Z = 0.f;
		FaceDirection(FacingDirection, DeltaTime);
	}

	DrawBossPhase3UltimateDebug(0.05f);

	if (GetWorld()->GetTimeSeconds() >= BossPhase3UltimateExplosionTime)
	{
		ResolveBossPhase3UltimateExplosion();
	}
}

void AARPGEnemyBase::ResolveBossPhase3UltimateExplosion()
{
	if (bIsDead || !GetWorld())
	{
		return;
	}

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (PlayerPawn)
	{
		FVector PlayerLocation = PlayerPawn->GetActorLocation();
		FVector SafeZoneLocation = BossPhase3CurrentSafeZoneCenter;
		PlayerLocation.Z = 0.f;
		SafeZoneLocation.Z = 0.f;

		if (FVector::Dist2D(PlayerLocation, SafeZoneLocation) <= BossPhase3SafeZoneRadius)
		{
			UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 explosion: player safe"));
		}
		else
		{
			UARPGHealthComponent* PlayerHealthComponent = PlayerPawn->FindComponentByClass<UARPGHealthComponent>();
			if (PlayerHealthComponent && !PlayerHealthComponent->IsDead())
			{
				PlayerHealthComponent->ApplyLethalDamageIgnoringInvincibility();
				UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 lava executed player"));
				DrawDebugSphere(GetWorld(), PlayerPawn->GetActorLocation(), 85.f, 16, FColor::Red, false, 0.5f);
			}
		}
	}

	const int32 TotalRounds = FMath::Max(1, BossPhase3UltimateTotalRounds);
	if (BossPhase3UltimateCurrentRound < TotalRounds)
	{
		++BossPhase3UltimateCurrentRound;
		GenerateBossPhase3SafeZone();
		BossPhase3UltimateExplosionTime = GetWorld()->GetTimeSeconds() + BossPhase3UltimateNextDelay;
		UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 next safe zone generated"));
		UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 safe zone round %d generated"), BossPhase3UltimateCurrentRound);
	}
	else
	{
		FinishBossPhase3Ultimate();
	}
}

void AARPGEnemyBase::FinishBossPhase3Ultimate()
{
	BossPhase3UltimateCurrentRound = 0;
	BossPhase3CurrentSafeZoneCenter = FVector::ZeroVector;

	UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 ultimate finished"));
	bIsBossPhase3UltimateActive = false;
	SummonBossPhase3Minions();
	UE_LOG(LogMyGame, Log, TEXT("Boss invincibility ended"));
}

void AARPGEnemyBase::SummonBossPhase3Minions()
{
	if (!bIsBoss
		|| bIsDead
		|| !GetWorld()
		|| !bEnableBossPhase3SummonMinionsAfterUltimate
		|| bHasBossPhase3SummonedMinions)
	{
		return;
	}

	BossPhase3SummonedMinions.Empty();
	bHasLoggedBossPhase3BlinkSlashLocked = false;

	TSubclassOf<AARPGEnemyBase> FallbackMinionClass = BossPhase3MinionClass;
	if (!FallbackMinionClass)
	{
		UClass* LoadedMinionClass = StaticLoadClass(
			AARPGEnemyBase::StaticClass(),
			nullptr,
			TEXT("/Game/Blueprint/BP_ARPGEnemyBase.BP_ARPGEnemyBase_C"));
		FallbackMinionClass = LoadedMinionClass ? LoadedMinionClass : AARPGEnemyBase::StaticClass();
		UE_LOG(LogMyGame, Warning, TEXT("BossPhase3MinionClass is not set. Prefer setting BossPhase3MinionClass = BP_ARPGEnemyBase in the Boss Blueprint."));
	}

	BossPhase3SummonedMinionTargetCount = 3;
	BossPhase3MinionCount = BossPhase3SummonedMinionTargetCount;
	const int32 SpawnCount = FMath::Max(0, BossPhase3SummonedMinionTargetCount);
	if (SpawnCount <= 0)
	{
		bHasBossPhase3SummonedMinions = true;
		UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 minion summon result: Spawned 0 / 0"));
		UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 tracked summoned minions: %d / %d"),
			BossPhase3SummonedMinions.Num(),
			BossPhase3SummonedMinionTargetCount);
		return;
	}

	bHasBossPhase3SummonedMinions = true;
	const FVector BossLocation = GetActorLocation();
	const float SpawnRadius = FMath::Max(0.f, BossPhase3MinionSpawnRadius);
	int32 SpawnedCount = 0;

	for (int32 Index = 0; Index < SpawnCount; ++Index)
	{
		const float AngleRadians = FMath::DegreesToRadians(360.f * static_cast<float>(Index) / static_cast<float>(SpawnCount));
		const FVector Direction(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians), 0.f);
		FVector SpawnLocation = BossLocation + Direction * SpawnRadius;
		SpawnLocation.Z += BossPhase3MinionSpawnZOffset;

		const FRotator SpawnRotation(0.f, (-Direction).Rotation().Yaw, 0.f);
		const FTransform SpawnTransform(SpawnRotation, SpawnLocation);
		TSubclassOf<AARPGEnemyBase> MinionClass = FallbackMinionClass;
		if (BossPhase3MinionClasses.Num() > 0)
		{
			MinionClass = BossPhase3MinionClasses[Index % BossPhase3MinionClasses.Num()];
			if (!MinionClass)
			{
				MinionClass = FallbackMinionClass;
			}
		}

		AARPGEnemyBase* Minion = GetWorld()->SpawnActorDeferred<AARPGEnemyBase>(
			MinionClass,
			SpawnTransform,
			this,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		if (!Minion)
		{
			UE_LOG(LogMyGame, Warning, TEXT("Boss Phase3 failed to summon minion %d / %d"), Index + 1, SpawnCount);
			continue;
		}

		Minion->bIsBoss = false;
		Minion->BossPhase = EARPGBossPhase::None;
		UGameplayStatics::FinishSpawningActor(Minion, SpawnTransform);
		Minion->bIsBoss = false;
		Minion->BossPhase = EARPGBossPhase::None;
		BossPhase3SummonedMinions.Add(Minion);
		Minion->SpawnDefaultController();
		++SpawnedCount;

		UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 summoned minion %d / %d"), Index + 1, SpawnCount);
	}

	UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 minion summon result: Spawned %d / %d"), SpawnedCount, SpawnCount);
	UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 tracked summoned minions: %d / %d"),
		BossPhase3SummonedMinions.Num(),
		BossPhase3SummonedMinionTargetCount);
}

void AARPGEnemyBase::StartBossPhase3FireRain(AActor* TargetActor)
{
	if (!bIsBoss
		|| BossPhase != EARPGBossPhase::Phase3
		|| bIsDead
		|| !GetWorld()
		|| !TargetActor
		|| bIsBossPhase3UltimateActive
		|| bIsBossPhase2WaveAttackActive
		|| bIsBossPhase3SkillRecovering
		|| bIsCastingBossPhase3FireRain)
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	bIsCastingBossPhase3FireRain = true;
	BossPhase3FireRainCastEndTime = CurrentTime + FMath::Max(0.f, BossPhase3FireRainCastTime);
	LastBossPhase3FireRainTime = CurrentTime;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	FVector FacingDirection = TargetActor->GetActorLocation() - GetActorLocation();
	FacingDirection.Z = 0.f;
	FaceDirection(FacingDirection, 0.1f);

	UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 FireRain cast started"));
}

void AARPGEnemyBase::HandleBossPhase3FireRainCast(float DeltaTime)
{
	if (bIsDead || !GetWorld())
	{
		bIsCastingBossPhase3FireRain = false;
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (PlayerPawn)
	{
		FVector FacingDirection = PlayerPawn->GetActorLocation() - GetActorLocation();
		FacingDirection.Z = 0.f;
		FaceDirection(FacingDirection, DeltaTime);
	}

	DrawDebugSphere(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 110.f), 85.f, 24, FColor::Yellow, false, 0.05f, 0, 3.f);

	if (GetWorld()->GetTimeSeconds() >= BossPhase3FireRainCastEndTime)
	{
		bIsCastingBossPhase3FireRain = false;
		LaunchBossPhase3FireRain();
	}
}

void AARPGEnemyBase::LaunchBossPhase3FireRain()
{
	if (bIsDead || !GetWorld())
	{
		return;
	}

	ActiveBossPhase3FireRainZones.RemoveAll([](const FARPGPhase3FireRainZone& Zone)
	{
		return !Zone.bActive;
	});

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const int32 ZoneCount = FMath::Max(0, BossPhase3FireRainCount);
	const float WarningTime = FMath::Max(0.f, BossPhase3FireRainWarningTime);
	const float SpawnInterval = FMath::Max(0.f, BossPhase3FireRainSpawnInterval);
	const float GroundDuration = FMath::Max(0.f, BossPhase3FireGroundDuration);
	const float MaxRadius = FMath::Max(0.f, BossPhase3FireRainArenaRadius);
	const float MinRadius = MaxRadius > 100.f ? 100.f : 0.f;
	const FVector BossLocation = GetActorLocation();

	for (int32 Index = 0; Index < ZoneCount; ++Index)
	{
		const float RandomAngleRadians = FMath::DegreesToRadians(FMath::FRandRange(0.f, 360.f));
		const float RandomRadius = FMath::FRandRange(MinRadius, MaxRadius);
		const FVector Direction(FMath::Cos(RandomAngleRadians), FMath::Sin(RandomAngleRadians), 0.f);

		FARPGPhase3FireRainZone Zone;
		Zone.Center = BossLocation + Direction * RandomRadius;
		Zone.Center.Z = BossLocation.Z + 10.f;
		Zone.ImpactTime = CurrentTime + WarningTime + static_cast<float>(Index) * SpawnInterval;
		Zone.GroundEndTime = Zone.ImpactTime + GroundDuration;
		Zone.NextGroundDamageTime = Zone.ImpactTime;
		Zone.bHasImpacted = false;
		Zone.bActive = true;
		Zone.bImpactHitPlayer = false;
		ActiveBossPhase3FireRainZones.Add(Zone);
	}

	UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 FireRain launched"));
	StartBossPhase3SkillRecovery();
}

void AARPGEnemyBase::UpdateBossPhase3FireRainZones(float DeltaTime)
{
	if (!GetWorld() || ActiveBossPhase3FireRainZones.Num() == 0)
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const float Radius = FMath::Max(0.f, BossPhase3FireRainRadius);
	const float GroundDamageInterval = FMath::Max(0.05f, BossPhase3FireGroundDamageInterval);
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UARPGHealthComponent* PlayerHealthComponent = PlayerPawn ? PlayerPawn->FindComponentByClass<UARPGHealthComponent>() : nullptr;

	for (FARPGPhase3FireRainZone& Zone : ActiveBossPhase3FireRainZones)
	{
		if (!Zone.bActive)
		{
			continue;
		}

		if (!Zone.bHasImpacted)
		{
			DrawBossPhase3FireRainCircle(Zone.Center, Radius, FColor::Red, 0.05f, 3.f);
			if (CurrentTime >= Zone.ImpactTime)
			{
				Zone.bHasImpacted = true;
				ResolveBossPhase3FireRainImpact(Zone);
			}
		}

		if (Zone.bHasImpacted)
		{
			DrawBossPhase3FireRainCircle(Zone.Center, Radius, FColor::Orange, 0.05f, 4.f);
			DrawDebugSphere(GetWorld(), Zone.Center, Radius, 24, FColor::Orange, false, 0.05f, 0, 1.f);

			if (CurrentTime >= Zone.GroundEndTime)
			{
				Zone.bActive = false;
				continue;
			}

			if (PlayerPawn
				&& PlayerHealthComponent
				&& !PlayerHealthComponent->IsDead()
				&& FVector::Dist2D(PlayerPawn->GetActorLocation(), Zone.Center) <= Radius
				&& CurrentTime >= Zone.NextGroundDamageTime)
			{
				PlayerHealthComponent->ApplyDamage(BossPhase3FireGroundDamage);
				Zone.NextGroundDamageTime = CurrentTime + GroundDamageInterval;
			}
		}
	}

	ActiveBossPhase3FireRainZones.RemoveAll([](const FARPGPhase3FireRainZone& Zone)
	{
		return !Zone.bActive;
	});
}

void AARPGEnemyBase::ResolveBossPhase3FireRainImpact(FARPGPhase3FireRainZone& Zone)
{
	if (!GetWorld())
	{
		return;
	}

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UARPGHealthComponent* PlayerHealthComponent = PlayerPawn ? PlayerPawn->FindComponentByClass<UARPGHealthComponent>() : nullptr;
	if (!PlayerPawn || !PlayerHealthComponent || PlayerHealthComponent->IsDead())
	{
		return;
	}

	if (FVector::Dist2D(PlayerPawn->GetActorLocation(), Zone.Center) <= FMath::Max(0.f, BossPhase3FireRainRadius))
	{
		PlayerHealthComponent->ApplyDamage(BossPhase3FireRainImpactDamage);
		Zone.bImpactHitPlayer = true;
		DrawDebugSphere(GetWorld(), PlayerPawn->GetActorLocation(), 65.f, 16, FColor::Red, false, 0.5f);
		UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 FireRain impact hit player"));
	}
}

void AARPGEnemyBase::DrawBossPhase3FireRainCircle(const FVector& Center, float Radius, const FColor& Color, float Duration, float Thickness) const
{
	if (!GetWorld() || Radius <= 0.f)
	{
		return;
	}

	const int32 SegmentCount = 32;
	FVector PreviousPoint = Center + FVector(Radius, 0.f, 0.f);
	for (int32 Index = 1; Index <= SegmentCount; ++Index)
	{
		const float AngleRadians = FMath::DegreesToRadians(360.f * static_cast<float>(Index) / static_cast<float>(SegmentCount));
		const FVector CurrentPoint = Center + FVector(FMath::Cos(AngleRadians) * Radius, FMath::Sin(AngleRadians) * Radius, 0.f);
		DrawDebugLine(GetWorld(), PreviousPoint, CurrentPoint, Color, false, Duration, 0, Thickness);
		PreviousPoint = CurrentPoint;
	}
}

void AARPGEnemyBase::StartBossPhase3IceSpear(AActor* TargetActor)
{
	if (!bIsBoss
		|| BossPhase != EARPGBossPhase::Phase3
		|| bIsDead
		|| !GetWorld()
		|| !TargetActor
		|| bIsBossPhase3UltimateActive
		|| bIsBossPhase2WaveAttackActive
		|| bIsCastingBossPhase3FireRain
		|| bIsBossPhase3SkillRecovering
		|| bIsPreparingBossPhase3IceSpear)
	{
		return;
	}

	FVector Direction = TargetActor->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.f;
	BossPhase3IceSpearBaseDirection = Direction.IsNearlyZero() ? GetActorForwardVector() : Direction.GetSafeNormal();
	BossPhase3IceSpearBaseDirection.Z = 0.f;
	BossPhase3IceSpearBaseDirection = BossPhase3IceSpearBaseDirection.IsNearlyZero()
		? FVector::ForwardVector
		: BossPhase3IceSpearBaseDirection.GetSafeNormal();

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	bIsPreparingBossPhase3IceSpear = true;
	LastBossPhase3IceSpearTime = CurrentTime;
	BossPhase3IceSpearLaunchTime = CurrentTime + FMath::Max(0.f, BossPhase3IceSpearWindup);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	FaceDirection(BossPhase3IceSpearBaseDirection, 0.1f);
	UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 IceSpear windup started"));
}

void AARPGEnemyBase::HandleBossPhase3IceSpearWindup(float DeltaTime)
{
	if (bIsDead || !GetWorld())
	{
		bIsPreparingBossPhase3IceSpear = false;
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	FaceDirection(BossPhase3IceSpearBaseDirection, DeltaTime);
	DrawBossPhase3IceSpearWarningDebug(0.05f);

	if (GetWorld()->GetTimeSeconds() >= BossPhase3IceSpearLaunchTime)
	{
		bIsPreparingBossPhase3IceSpear = false;
		LaunchBossPhase3IceSpears();
	}
}

void AARPGEnemyBase::LaunchBossPhase3IceSpears()
{
	if (bIsDead || !GetWorld())
	{
		return;
	}

	const int32 SpearCount = FMath::Max(1, BossPhase3IceSpearCount);
	const float Spread = FMath::Max(0.f, BossPhase3IceSpearSpreadDegrees);
	const float StartOffset = 90.f;
	const FVector BossLocation = GetActorLocation();

	for (int32 Index = 0; Index < SpearCount; ++Index)
	{
		const float Alpha = SpearCount > 1 ? static_cast<float>(Index) / static_cast<float>(SpearCount - 1) : 0.5f;
		const float AngleOffset = FMath::Lerp(-Spread * 0.5f, Spread * 0.5f, Alpha);
		FVector SpearDirection = BossPhase3IceSpearBaseDirection.RotateAngleAxis(AngleOffset, FVector::UpVector);
		SpearDirection.Z = 0.f;
		SpearDirection = SpearDirection.IsNearlyZero() ? FVector::ForwardVector : SpearDirection.GetSafeNormal();

		FARPGPhase3IceSpear Spear;
		Spear.Direction = SpearDirection;
		Spear.Location = BossLocation + SpearDirection * StartOffset;
		Spear.Location.Z += 60.f;
		Spear.TraveledDistance = 0.f;
		Spear.bActive = true;
		Spear.bHitPlayer = false;
		ActiveBossPhase3IceSpears.Add(Spear);
	}

	UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 IceSpears launched"));
	StartBossPhase3SkillRecovery();
}

void AARPGEnemyBase::UpdateBossPhase3IceSpears(float DeltaTime)
{
	if (!GetWorld() || ActiveBossPhase3IceSpears.Num() == 0)
	{
		return;
	}

	const float DeltaDistance = FMath::Max(0.f, BossPhase3IceSpearSpeed) * DeltaTime;
	const float MaxDistance = FMath::Max(0.f, BossPhase3IceSpearMaxDistance);
	const float SpearLength = FMath::Max(10.f, BossPhase3IceSpearLength);
	const float SpearWidth = FMath::Max(1.f, BossPhase3IceSpearWidth);
	const float HitRadius = SpearWidth + 40.f;
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UARPGHealthComponent* PlayerHealthComponent = PlayerPawn ? PlayerPawn->FindComponentByClass<UARPGHealthComponent>() : nullptr;
	AARPGPlayerController* ARPGPlayerController = Cast<AARPGPlayerController>(PlayerController);

	for (FARPGPhase3IceSpear& Spear : ActiveBossPhase3IceSpears)
	{
		if (!Spear.bActive)
		{
			continue;
		}

		Spear.Location += Spear.Direction * DeltaDistance;
		Spear.TraveledDistance += DeltaDistance;
		const FVector SpearStart = Spear.Location - Spear.Direction * SpearLength * 0.5f;
		const FVector SpearEnd = Spear.Location + Spear.Direction * SpearLength * 0.5f;
		const FVector RightVector = FVector::CrossProduct(FVector::UpVector, Spear.Direction).GetSafeNormal();
		const FVector WidthOffset = RightVector * SpearWidth * 0.5f;

		DrawDebugLine(GetWorld(), SpearStart, SpearEnd, FColor::Cyan, false, 0.05f, 0, 5.f);
		DrawDebugLine(GetWorld(), SpearStart + WidthOffset, SpearEnd + WidthOffset, FColor::Cyan, false, 0.05f, 0, 1.5f);
		DrawDebugLine(GetWorld(), SpearStart - WidthOffset, SpearEnd - WidthOffset, FColor::Cyan, false, 0.05f, 0, 1.5f);
		DrawDebugSphere(GetWorld(), SpearEnd, 18.f, 8, FColor::Cyan, false, 0.05f, 0, 2.f);

		if (Spear.TraveledDistance >= MaxDistance)
		{
			Spear.bActive = false;
			continue;
		}

		if (PlayerPawn
			&& !Spear.bHitPlayer
			&& DistancePointToSegment2D(PlayerPawn->GetActorLocation(), SpearStart, SpearEnd) <= HitRadius)
		{
			Spear.bHitPlayer = true;
			Spear.bActive = false;

			if (PlayerHealthComponent && !PlayerHealthComponent->IsDead() && BossPhase3IceSpearDamage > 0.f)
			{
				PlayerHealthComponent->ApplyDamage(BossPhase3IceSpearDamage);
			}

			if (ARPGPlayerController)
			{
				ARPGPlayerController->ApplyFreezeBuildup(BossPhase3IceBuildupPerHit);
			}

			DrawDebugSphere(GetWorld(), PlayerPawn->GetActorLocation(), 65.f, 16, FColor::Cyan, false, 0.5f);
			UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 IceSpear hit player, freeze buildup applied"));
		}
	}

	ActiveBossPhase3IceSpears.RemoveAll([](const FARPGPhase3IceSpear& Spear)
	{
		return !Spear.bActive;
	});
}

void AARPGEnemyBase::DrawBossPhase3IceSpearWarningDebug(float Duration) const
{
	if (!GetWorld())
	{
		return;
	}

	const int32 SpearCount = FMath::Max(1, BossPhase3IceSpearCount);
	const float Spread = FMath::Max(0.f, BossPhase3IceSpearSpreadDegrees);
	const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 60.f);

	for (int32 Index = 0; Index < SpearCount; ++Index)
	{
		const float Alpha = SpearCount > 1 ? static_cast<float>(Index) / static_cast<float>(SpearCount - 1) : 0.5f;
		const float AngleOffset = FMath::Lerp(-Spread * 0.5f, Spread * 0.5f, Alpha);
		FVector SpearDirection = BossPhase3IceSpearBaseDirection.RotateAngleAxis(AngleOffset, FVector::UpVector);
		SpearDirection.Z = 0.f;
		SpearDirection = SpearDirection.IsNearlyZero() ? FVector::ForwardVector : SpearDirection.GetSafeNormal();
		DrawDebugLine(GetWorld(), Start, Start + SpearDirection * BossPhase3IceSpearMaxDistance, FColor::Cyan, false, Duration, 0, 2.f);
	}
}

void AARPGEnemyBase::StartBossPhase3Thunder(AActor* TargetActor)
{
	if (!bIsBoss
		|| BossPhase != EARPGBossPhase::Phase3
		|| bIsDead
		|| !GetWorld()
		|| !TargetActor
		|| bIsBossPhase3UltimateActive
		|| bIsBossPhase2WaveAttackActive
		|| bIsCastingBossPhase3FireRain
		|| bIsPreparingBossPhase3IceSpear
		|| bIsBossPhase3SkillRecovering
		|| bIsPreparingBossPhase3Thunder)
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	bIsPreparingBossPhase3Thunder = true;
	LastBossPhase3ThunderTime = CurrentTime;
	BossPhase3ThunderLaunchTime = CurrentTime + FMath::Max(0.f, BossPhase3ThunderWindup);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	FVector FacingDirection = TargetActor->GetActorLocation() - GetActorLocation();
	FacingDirection.Z = 0.f;
	FaceDirection(FacingDirection, 0.1f);
	UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 Thunder windup started"));
}

void AARPGEnemyBase::HandleBossPhase3ThunderWindup(float DeltaTime)
{
	if (bIsDead || !GetWorld())
	{
		bIsPreparingBossPhase3Thunder = false;
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (PlayerPawn)
	{
		FVector FacingDirection = PlayerPawn->GetActorLocation() - GetActorLocation();
		FacingDirection.Z = 0.f;
		FaceDirection(FacingDirection, DeltaTime);
	}

	DrawDebugSphere(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 100.f), 110.f, 24, FColor::Purple, false, 0.05f, 0, 3.f);

	if (GetWorld()->GetTimeSeconds() >= BossPhase3ThunderLaunchTime)
	{
		bIsPreparingBossPhase3Thunder = false;
		LaunchBossPhase3Thunder();
	}
}

void AARPGEnemyBase::LaunchBossPhase3Thunder()
{
	if (bIsDead || !GetWorld())
	{
		return;
	}

	const int32 OrbCount = FMath::Max(1, BossPhase3ThunderOrbCount);
	const FVector BossLocation = GetActorLocation();
	const float StartOffset = 90.f;

	for (int32 Index = 0; Index < OrbCount; ++Index)
	{
		const float AngleDegrees = 360.f * static_cast<float>(Index) / static_cast<float>(OrbCount) + BossPhase3ThunderPatternRotationDegrees;
		const float AngleRadians = FMath::DegreesToRadians(AngleDegrees);
		FVector Direction(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians), 0.f);
		Direction = Direction.GetSafeNormal();

		FARPGPhase3ThunderOrb Orb;
		Orb.Direction = Direction;
		Orb.Location = BossLocation + Direction * StartOffset;
		Orb.Location.Z += 65.f;
		Orb.TraveledDistance = 0.f;
		Orb.bActive = true;
		Orb.bHitPlayer = false;
		ActiveBossPhase3ThunderOrbs.Add(Orb);
	}

	BossPhase3ThunderPatternRotationDegrees = FMath::Fmod(
		BossPhase3ThunderPatternRotationDegrees + BossPhase3ThunderSpiralAngleOffsetDegrees,
		360.f);
	if (BossPhase3ThunderPatternRotationDegrees < 0.f)
	{
		BossPhase3ThunderPatternRotationDegrees += 360.f;
	}

	UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 Thunder launched"));
	StartBossPhase3SkillRecovery();
}

void AARPGEnemyBase::UpdateBossPhase3ThunderOrbs(float DeltaTime)
{
	if (!GetWorld() || ActiveBossPhase3ThunderOrbs.Num() == 0)
	{
		return;
	}

	const float DeltaDistance = FMath::Max(0.f, BossPhase3ThunderSpeed) * DeltaTime;
	const float MaxDistance = FMath::Max(0.f, BossPhase3ThunderMaxDistance);
	const float HitRadius = FMath::Max(0.f, BossPhase3ThunderOrbRadius) + 40.f;
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UARPGHealthComponent* PlayerHealthComponent = PlayerPawn ? PlayerPawn->FindComponentByClass<UARPGHealthComponent>() : nullptr;
	AARPGPlayerController* ARPGPlayerController = Cast<AARPGPlayerController>(PlayerController);

	for (FARPGPhase3ThunderOrb& Orb : ActiveBossPhase3ThunderOrbs)
	{
		if (!Orb.bActive)
		{
			continue;
		}

		Orb.Location += Orb.Direction * DeltaDistance;
		Orb.TraveledDistance += DeltaDistance;
		DrawDebugSphere(GetWorld(), Orb.Location, BossPhase3ThunderOrbRadius, 12, FColor::Purple, false, 0.05f, 0, 2.f);

		if (Orb.TraveledDistance >= MaxDistance)
		{
			Orb.bActive = false;
			continue;
		}

		if (PlayerPawn
			&& !Orb.bHitPlayer
			&& FVector::Dist2D(PlayerPawn->GetActorLocation(), Orb.Location) <= HitRadius)
		{
			Orb.bHitPlayer = true;
			Orb.bActive = false;

			if (PlayerHealthComponent && !PlayerHealthComponent->IsDead())
			{
				PlayerHealthComponent->ApplyDamage(BossPhase3ThunderDamage);
			}

			if (ARPGPlayerController)
			{
				ARPGPlayerController->ApplyShockBuildup(BossPhase3ShockBuildupPerHit);
			}

			DrawDebugSphere(GetWorld(), PlayerPawn->GetActorLocation(), 65.f, 16, FColor::Purple, false, 0.5f);
			UE_LOG(LogMyGame, Log, TEXT("Boss Phase3 Thunder hit player, shock buildup applied"));
		}
	}

	ActiveBossPhase3ThunderOrbs.RemoveAll([](const FARPGPhase3ThunderOrb& Orb)
	{
		return !Orb.bActive;
	});
}

void AARPGEnemyBase::DrawBossPhase3UltimateDebug(float Duration) const
{
	if (!GetWorld())
	{
		return;
	}

	const int32 SegmentCount = 64;
	FVector PreviousArenaPoint = BossPhase3ArenaCenter + FVector(BossPhase3ArenaRadius, 0.f, BossPhase3SafeZoneZOffset);
	for (int32 Index = 1; Index <= SegmentCount; ++Index)
	{
		const float AngleRadians = FMath::DegreesToRadians(360.f * static_cast<float>(Index) / static_cast<float>(SegmentCount));
		const FVector CurrentPoint = BossPhase3ArenaCenter
			+ FVector(FMath::Cos(AngleRadians) * BossPhase3ArenaRadius, FMath::Sin(AngleRadians) * BossPhase3ArenaRadius, BossPhase3SafeZoneZOffset);
		DrawDebugLine(GetWorld(), PreviousArenaPoint, CurrentPoint, FColor::Red, false, Duration, 0, 2.f);
		PreviousArenaPoint = CurrentPoint;
	}

	FVector PreviousSafePoint = BossPhase3CurrentSafeZoneCenter + FVector(BossPhase3SafeZoneRadius, 0.f, 0.f);
	for (int32 Index = 1; Index <= SegmentCount; ++Index)
	{
		const float AngleRadians = FMath::DegreesToRadians(360.f * static_cast<float>(Index) / static_cast<float>(SegmentCount));
		const FVector CurrentPoint = BossPhase3CurrentSafeZoneCenter
			+ FVector(FMath::Cos(AngleRadians) * BossPhase3SafeZoneRadius, FMath::Sin(AngleRadians) * BossPhase3SafeZoneRadius, 0.f);
		DrawDebugLine(GetWorld(), PreviousSafePoint, CurrentPoint, FColor::Green, false, Duration, 0, 3.f);
		PreviousSafePoint = CurrentPoint;
	}

	DrawDebugSphere(GetWorld(), BossPhase3CurrentSafeZoneCenter, BossPhase3SafeZoneRadius, 24, FColor::Green, false, Duration, 0, 1.f);
	DrawDebugSphere(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 100.f), 95.f, 24, FColor::Yellow, false, Duration, 0, 3.f);
}

void AARPGEnemyBase::DrawEnemyAttackRangeDebug(float Duration) const
{
	if (GetWorld())
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), AttackRange, 32, FColor::Orange, false, Duration, 0, 2.f);
	}
}

void AARPGEnemyBase::DrawBossSlamRangeDebug(float Duration) const
{
	if (GetWorld())
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), BossSlamRange, 48, FColor::Red, false, Duration, 0, 4.f);
	}
}

void AARPGEnemyBase::FaceDirection(const FVector& Direction, float DeltaTime)
{
	FVector FlatDirection(Direction.X, Direction.Y, 0.f);
	if (FlatDirection.IsNearlyZero())
	{
		return;
	}

	const FRotator TargetRotation(0.f, FlatDirection.Rotation().Yaw, 0.f);
	const FRotator SmoothedRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, EnemyFacingInterpSpeed);
	SetActorRotation(FRotator(0.f, SmoothedRotation.Yaw, 0.f));
}

void AARPGEnemyBase::PlayHitFeedback()
{
	if (GetWorld())
	{
		DrawDebugSphere(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 80.f), 40.f, 16, FColor::Orange, false, 1.f);
	}
}

void AARPGEnemyBase::UpdateBleed(float CurrentTime)
{
	if (!bIsBleeding)
	{
		return;
	}

	if (bIsDead)
	{
		bIsBleeding = false;
		return;
	}

	if (CurrentTime >= BleedEndTime)
	{
		bIsBleeding = false;
		UE_LOG(LogMyGame, Log, TEXT("%s Bleed ended"), *GetName());
		return;
	}

	if (CurrentTime < NextBleedTickTime)
	{
		return;
	}

	NextBleedTickTime += BleedTickIntervalRuntime;

	UARPGHealthComponent* InstanceHealth = ResolveHealthComponent();
	if (!InstanceHealth)
	{
		bIsBleeding = false;
		return;
	}

	ApplyDamageWithBossModifiers(BleedDamagePerTickRuntime);
	UE_LOG(LogMyGame, Log, TEXT("%s Bleed tick damage: %.1f"), *GetName(), BleedDamagePerTickRuntime);
}

void AARPGEnemyBase::Die()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	bIsBleeding = false;
	BleedDamagePerTickRuntime = 0.f;
	BleedEndTime = 0.f;
	NextBleedTickTime = 0.f;
	bIsPreparingAttack = false;
	PendingAttackTarget = nullptr;
	bIsPreparingBossSlam = false;
	PendingBossSlamTarget = nullptr;
	bIsPreparingBossCone = false;
	PendingBossConeTarget = nullptr;
	bIsPreparingBossBarrage = false;
	bIsBossBarrageActive = false;
	bBossBarrageHitPlayerThisWave = false;
	bIsPreparingBossLongSlash = false;
	bIsRecoveringBossLongSlash = false;
	bIsPreparingBossRandomSlash = false;
	bIsExecutingBossRandomSlash = false;
	bIsPreparingMageAttack = false;
	bIsPreparingThrowerAttack = false;
	MageAttackResolveTime = 0.f;
	MageAttackDirection = FVector::ZeroVector;
	ThrowerAttackResolveTime = 0.f;
	PendingThrowerTargetLocation = FVector::ZeroVector;
	ActiveMageProjectiles.Empty();
	ActiveThrowerFireZones.Empty();
	bIsBossPhase2WaveAttackActive = false;
	ActiveBossPhase2WaveOrbs.Empty();
	BossPhase2WaveCurrentLoop = 0;
	bBossPhase2WaveHitPlayerThisFrameOrWave = false;
	bIsBossPhase3UltimateActive = false;
	BossPhase3UltimateCurrentRound = 0;
	BossPhase3CurrentSafeZoneCenter = FVector::ZeroVector;
	BossPhase3ArenaCenter = FVector::ZeroVector;
	bIsCastingBossPhase3FireRain = false;
	BossPhase3FireRainCastEndTime = 0.f;
	ActiveBossPhase3FireRainZones.Empty();
	bIsPreparingBossPhase3IceSpear = false;
	BossPhase3IceSpearLaunchTime = 0.f;
	BossPhase3IceSpearBaseDirection = FVector::ZeroVector;
	ActiveBossPhase3IceSpears.Empty();
	bIsPreparingBossPhase3Thunder = false;
	BossPhase3ThunderLaunchTime = 0.f;
	ActiveBossPhase3ThunderOrbs.Empty();
	bIsBossPhase3SkillRecovering = false;
	NextBossPhase3SkillAllowedTime = 0.f;
	BossPhase3SkillRecoveryEndTime = 0.f;
	bHasLastBossPhase3SkillUsed = false;
	bHasLoggedBossPhase3BlinkSlashLocked = false;
	BossPhase3SummonedMinions.Empty();
	BossPhase2TransitionEndTime = 0.f;
	bBossPhase2TeleportCompleted = false;
	PendingBossBarrageTarget = nullptr;
	PendingBossLongSlashTarget = nullptr;
	BossLongSlashDirection = FVector::ZeroVector;
	BossRandomSlashExecutedCount = 0;
	BossRandomSlashBaseDirection = FVector::ZeroVector;
	ActiveBossBarrageOrbs.Empty();
	if (bIsBoss)
	{
		BossPhase = EARPGBossPhase::Dead;
	}
	UE_LOG(LogMyGame, Log, TEXT("Enemy died: %s"), *GetName());

	TArray<AActor*> MissionManagers;
	UGameplayStatics::GetAllActorsOfClass(this, AARPGMissionManager::StaticClass(), MissionManagers);
	AARPGMissionManager* MissionManager = nullptr;
	for (AActor* MissionManagerActor : MissionManagers)
	{
		MissionManager = Cast<AARPGMissionManager>(MissionManagerActor);
		if (MissionManager)
		{
			break;
		}
	}

	if (MissionManager)
	{
		if (bIsBoss)
		{
			MissionManager->NotifyBossKilled(this);
		}
		else
		{
			MissionManager->NotifyEnemyKilled(this);
		}
	}
	else
	{
		UE_LOG(LogMyGame, Warning, TEXT("MissionManager not found"));
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (USkeletalMeshComponent* MeshComponent = GetMesh())
	{
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->SetRelativeRotation(MeshComponent->GetRelativeRotation() + DeathMeshRotationOffset);
		MeshComponent->AddLocalOffset(DeathMeshLocationOffset);
	}

	if (!bIsBoss)
	{
		SetLifeSpan(CorpseLifeSpan);
	}
	else
	{
		UE_LOG(LogMyGame, Log, TEXT("Boss corpse will remain: %s"), *GetName());
	}
}
