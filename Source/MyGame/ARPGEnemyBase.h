// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ARPGEnemyBase.generated.h"

class UARPGHealthComponent;

UENUM(BlueprintType)
enum class EARPGBossPhase : uint8
{
	None,
	Phase1,
	Phase2Transition,
	Phase2,
	Phase3,
	Dead
};

UENUM(BlueprintType)
enum class EARPGEnemyCombatType : uint8
{
	Melee,
	Mage,
	Thrower
};

UENUM()
enum class EBossPhase3SkillType : uint8
{
	FireRain,
	IceSpear,
	Thunder,
	BlinkSlash
};

USTRUCT()
struct FARPGBossBarrageOrb
{
	GENERATED_BODY()

	FVector Location = FVector::ZeroVector;
	FVector Direction = FVector::ZeroVector;
	float TraveledDistance = 0.f;
	bool bActive = false;
};

USTRUCT()
struct FARPGPhase2WaveOrb
{
	GENERATED_BODY()

	FVector Location = FVector::ZeroVector;
	FVector Direction = FVector::ZeroVector;
	float TraveledDistance = 0.f;
	bool bReturning = false;
	bool bActive = false;
	bool bHitPlayerThisTrip = false;
};

USTRUCT()
struct FARPGPhase3FireRainZone
{
	GENERATED_BODY()

	FVector Center = FVector::ZeroVector;
	float ImpactTime = 0.f;
	float GroundEndTime = 0.f;
	float NextGroundDamageTime = 0.f;
	bool bHasImpacted = false;
	bool bActive = false;
	bool bImpactHitPlayer = false;
};

USTRUCT()
struct FARPGPhase3IceSpear
{
	GENERATED_BODY()

	FVector Location = FVector::ZeroVector;
	FVector Direction = FVector::ZeroVector;
	float TraveledDistance = 0.f;
	bool bActive = false;
	bool bHitPlayer = false;
};

USTRUCT()
struct FARPGPhase3ThunderOrb
{
	GENERATED_BODY()

	FVector Location = FVector::ZeroVector;
	FVector Direction = FVector::ZeroVector;
	float TraveledDistance = 0.f;
	bool bActive = false;
	bool bHitPlayer = false;
};

USTRUCT()
struct FARPGEnemyMageProjectile
{
	GENERATED_BODY()

	FVector Location = FVector::ZeroVector;
	FVector Direction = FVector::ZeroVector;
	float TraveledDistance = 0.f;
	bool bActive = false;
	bool bHitPlayer = false;
};

USTRUCT()
struct FARPGEnemyThrowerFireZone
{
	GENERATED_BODY()

	FVector Center = FVector::ZeroVector;
	float ImpactTime = 0.f;
	float GroundEndTime = 0.f;
	float NextGroundDamageTime = 0.f;
	bool bHasImpacted = false;
	bool bActive = false;
};

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
	UARPGHealthComponent* GetHealthComponent();

	UFUNCTION(BlueprintCallable, Category="Combat")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintCallable, Category="Status")
	void ApplyBleed(float DamagePerTick, float Duration, float TickInterval);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy")
	bool bIsBoss = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss")
	EARPGBossPhase BossPhase = EARPGBossPhase::None;

protected:
	UARPGHealthComponent* ResolveHealthComponent();

	void UpdateSimpleAI(float DeltaTime);
	void UpdateMeleeAI(float DeltaTime);
	void UpdateMageAI(float DeltaTime);
	void StartMageAttack(AActor* TargetActor);
	void HandleMageAttackWindup(float DeltaTime);
	void LaunchMageProjectile();
	void UpdateMageProjectiles(float DeltaTime);
	void DrawMageAttackDebug(float Duration) const;
	void UpdateThrowerAI(float DeltaTime);
	void StartThrowerAttack(AActor* TargetActor);
	void HandleThrowerAttackWindup(float DeltaTime);
	void LaunchThrowerFireZone();
	void UpdateThrowerFireZones(float DeltaTime);
	void ResolveThrowerFireImpact(FARPGEnemyThrowerFireZone& Zone);
	void DrawEnemyFireCircle(const FVector& Center, float Radius, const FColor& Color, float Duration, float Thickness) const;
	void EnterBossPhase2();
	void HandleBossPhase2Transition(float DeltaTime);
	void FinishBossPhase2Transition();
	bool TryStartBossPhase1Skill(AActor* TargetActor, float DistanceToTarget);
	bool TryStartBossPhase2Skill(AActor* TargetActor, float DistanceToTarget);
	bool TryStartBossPhase3Skill(AActor* TargetActor);
	void EnterBossPhase3();
	void ApplyDamageWithBossModifiers(float DamageAmount);
	void StartEnemyAttack(AActor* TargetActor, float DeltaTime);
	void ResolveEnemyAttack();
	void DrawEnemyAttackRangeDebug(float Duration) const;
	void StartBossSlam(AActor* TargetActor);
	void ResolveBossSlam();
	void DrawBossSlamRangeDebug(float Duration) const;
	void StartBossConeStrike(AActor* TargetActor);
	void ResolveBossConeStrike();
	void DrawBossConeStrikeDebug(float Duration) const;
	void StartBossBarrage(AActor* TargetActor);
	void LaunchBossBarrage();
	void UpdateBossBarrageOrbs(float DeltaTime);
	void DrawBossBarrageDebug(float Duration) const;
	void StartBossLongSlash(AActor* TargetActor);
	void ResolveBossLongSlash();
	void DrawBossLongSlashDebug(float Duration) const;
	void StartBossRandomSlash(AActor* TargetActor);
	void ExecuteOneBossRandomSlash();
	void DrawBossRandomSlashFanDebug(float Duration) const;
	void DrawBossRandomSlashStrikeDebug(const FVector& SlashDirection, float Duration) const;
	void StartBossPhase2WaveAttack();
	void LaunchBossPhase2WaveLoop();
	void UpdateBossPhase2WaveAttack(float DeltaTime);
	void FinishBossPhase2WaveAttack();
	void StartBossPhase3Ultimate();
	void GenerateBossPhase3SafeZone();
	void UpdateBossPhase3Ultimate(float DeltaTime);
	void ResolveBossPhase3UltimateExplosion();
	void FinishBossPhase3Ultimate();
	void SummonBossPhase3Minions();
	void DrawBossPhase3UltimateDebug(float Duration) const;
	void StartBossPhase3FireRain(AActor* TargetActor);
	void HandleBossPhase3FireRainCast(float DeltaTime);
	void LaunchBossPhase3FireRain();
	void UpdateBossPhase3FireRainZones(float DeltaTime);
	void ResolveBossPhase3FireRainImpact(FARPGPhase3FireRainZone& Zone);
	void DrawBossPhase3FireRainCircle(const FVector& Center, float Radius, const FColor& Color, float Duration, float Thickness) const;
	void StartBossPhase3IceSpear(AActor* TargetActor);
	void HandleBossPhase3IceSpearWindup(float DeltaTime);
	void LaunchBossPhase3IceSpears();
	void UpdateBossPhase3IceSpears(float DeltaTime);
	void DrawBossPhase3IceSpearWarningDebug(float Duration) const;
	void StartBossPhase3Thunder(AActor* TargetActor);
	void HandleBossPhase3ThunderWindup(float DeltaTime);
	void LaunchBossPhase3Thunder();
	void UpdateBossPhase3ThunderOrbs(float DeltaTime);
	void StartBossPhase3SkillRecovery();
	void HandleBossPhase3SkillRecovery(float DeltaTime);
	bool TryUseRandomBossPhase3Skill(AActor* TargetActor);
	bool AreBossPhase3SummonedMinionsAllDead() const;
	float DistancePointToSegment2D(const FVector& Point, const FVector& SegmentStart, const FVector& SegmentEnd) const;
	void FaceDirection(const FVector& Direction, float DeltaTime);
	void PlayHitFeedback();
	void UpdateBleed(float CurrentTime);
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Type")
	EARPGEnemyCombatType EnemyCombatType = EARPGEnemyCombatType::Melee;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Mage")
	float MageAttackRange = 850.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Mage")
	float MagePreferredDistance = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Mage")
	float MageTooCloseDistance = 320.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Mage")
	float MageAttackCooldown = 2.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Mage")
	float MageAttackWindup = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Mage")
	float MageProjectileSpeed = 650.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Mage")
	float MageProjectileMaxDistance = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Mage")
	float MageProjectileRadius = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Mage")
	float MageProjectileDamage = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Thrower")
	float ThrowerAttackRange = 760.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Thrower")
	float ThrowerPreferredDistance = 520.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Thrower")
	float ThrowerAttackCooldown = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Thrower")
	float ThrowerAttackWindup = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Thrower")
	float ThrowerImpactDelay = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Thrower")
	float ThrowerFireRadius = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Thrower")
	float ThrowerImpactDamage = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Thrower")
	float ThrowerFireGroundDamage = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Thrower")
	float ThrowerFireGroundDuration = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Thrower")
	float ThrowerFireGroundDamageInterval = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss")
	bool bEnableBossSlam = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss")
	float BossSlamRange = 420.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss")
	float BossSlamDamage = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss")
	float BossSlamCooldown = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss")
	float BossSlamWindup = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Cone")
	bool bEnableBossConeStrike = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Cone")
	float BossConeRange = 520.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Cone")
	float BossConeAngleDegrees = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Cone")
	float BossConeDamage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Cone")
	float BossConeCooldown = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Cone")
	float BossConeWindup = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Cone")
	float BossConeSlowMultiplier = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Cone")
	float BossConeSlowDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Barrage")
	bool bEnableBossBarrage = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Barrage")
	int32 BossBarrageOrbCount = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Barrage")
	float BossBarrageOrbRadius = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Barrage")
	float BossBarrageOrbSpeed = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Barrage")
	float BossBarrageMaxDistance = 760.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Barrage")
	float BossBarrageDamage = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Barrage")
	float BossBarrageCooldown = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Barrage")
	float BossBarrageWindup = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase")
	float BossPhase2TransitionDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase")
	float BossPhase2TransitionDamageReduction = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase")
	float BossPhase2TeleportBehindDistance = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2")
	bool bEnableBossLongSlash = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2")
	float BossLongSlashRange = 720.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2")
	float BossLongSlashWidth = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2")
	float BossLongSlashDamage = 32.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2")
	float BossLongSlashCooldown = 5.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2")
	float BossLongSlashWindup = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2")
	float BossLongSlashRecovery = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2")
	bool bEnableBossRandomSlash = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2")
	float BossRandomSlashFanAngleDegrees = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2")
	float BossRandomSlashRange = 680.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2")
	float BossRandomSlashWidth = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2")
	float BossRandomSlashDamage = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2")
	float BossRandomSlashCooldown = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2")
	float BossRandomSlashWindup = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2")
	int32 BossRandomSlashCount = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2")
	float BossRandomSlashInterval = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2Wave")
	bool bEnableBossPhase2WaveAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2Wave")
	float BossPhase2WaveHealthThreshold = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2Wave")
	int32 BossPhase2WaveCount = 9;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2Wave")
	int32 BossPhase2WaveLoopCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2Wave")
	float BossPhase2WaveSpeed = 420.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2Wave")
	float BossPhase2WaveMaxDistance = 760.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2Wave")
	float BossPhase2WaveRadius = 55.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2Wave")
	float BossPhase2WaveDamage = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase2Wave")
	float BossPhase2WaveStartOffset = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3")
	bool bEnableBossPhase3Ultimate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3")
	int32 BossPhase3UltimateTotalRounds = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3")
	float BossPhase3UltimateFirstDelay = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3")
	float BossPhase3UltimateNextDelay = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3")
	float BossPhase3ArenaRadius = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3")
	float BossPhase3SafeZoneRadius = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3")
	float BossPhase3ExplosionDamage = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3")
	float BossPhase3SafeZoneMinDistanceFromBoss = 420.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3")
	float BossPhase3SafeZoneZOffset = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3")
	bool bEnableBossPhase3SummonMinionsAfterUltimate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3")
	TSubclassOf<AARPGEnemyBase> BossPhase3MinionClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3")
	TArray<TSubclassOf<AARPGEnemyBase>> BossPhase3MinionClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3")
	int32 BossPhase3MinionCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3")
	float BossPhase3MinionSpawnRadius = 480.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3")
	float BossPhase3GlobalSkillGap = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3")
	float BossPhase3SkillRecovery = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3")
	float BossPhase3MinionSpawnZOffset = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3BlinkSlash")
	bool bEnableBossPhase3BlinkSlash = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3FireRain")
	bool bEnableBossPhase3FireRain = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3FireRain")
	int32 BossPhase3FireRainCount = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3FireRain")
	float BossPhase3FireRainRadius = 130.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3FireRain")
	float BossPhase3FireRainWarningTime = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3FireRain")
	float BossPhase3FireRainSpawnInterval = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3FireRain")
	float BossPhase3FireRainImpactDamage = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3FireRain")
	float BossPhase3FireGroundDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3FireRain")
	float BossPhase3FireGroundDamage = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3FireRain")
	float BossPhase3FireGroundDamageInterval = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3FireRain")
	float BossPhase3FireRainArenaRadius = 850.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3FireRain")
	float BossPhase3FireRainCooldown = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3FireRain")
	float BossPhase3FireRainCastTime = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3Ice")
	bool bEnableBossPhase3IceSpear = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3Ice")
	float BossPhase3IceSpearWindup = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3Ice")
	int32 BossPhase3IceSpearCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3Ice")
	float BossPhase3IceSpearSpreadDegrees = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3Ice")
	float BossPhase3IceSpearSpeed = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3Ice")
	float BossPhase3IceSpearMaxDistance = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3Ice")
	float BossPhase3IceSpearRadius = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3Ice")
	float BossPhase3IceSpearDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3Ice")
	float BossPhase3IceBuildupPerHit = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3Ice")
	float BossPhase3IceSpearCooldown = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3Ice")
	float BossPhase3IceSpearLength = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3Ice")
	float BossPhase3IceSpearWidth = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3Thunder")
	bool bEnableBossPhase3Thunder = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3Thunder")
	float BossPhase3ThunderWindup = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3Thunder")
	int32 BossPhase3ThunderOrbCount = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3Thunder")
	float BossPhase3ThunderSpeed = 520.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3Thunder")
	float BossPhase3ThunderMaxDistance = 850.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3Thunder")
	float BossPhase3ThunderOrbRadius = 42.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3Thunder")
	float BossPhase3ThunderDamage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3Thunder")
	float BossPhase3ShockBuildupPerHit = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3Thunder")
	float BossPhase3ThunderCooldown = 6.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Phase3Thunder")
	float BossPhase3ThunderSpiralAngleOffsetDegrees = 20.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Status")
	bool bIsBleeding = false;

	bool bIsDead = false;
	bool bIsPreparingAttack = false;
	bool bIsPreparingBossSlam = false;
	bool bIsPreparingBossCone = false;
	bool bIsPreparingBossBarrage = false;
	bool bIsBossBarrageActive = false;
	bool bBossBarrageHitPlayerThisWave = false;
	bool bIsPreparingBossLongSlash = false;
	bool bIsRecoveringBossLongSlash = false;
	bool bIsPreparingBossRandomSlash = false;
	bool bIsExecutingBossRandomSlash = false;
	bool bHasTriggeredPhase2HalfHealthWave = false;
	bool bIsBossPhase2WaveAttackActive = false;
	bool bBossPhase2WaveHitPlayerThisFrameOrWave = false;
	bool bIsBossPhase3UltimateActive = false;
	bool bHasStartedBossPhase3Ultimate = false;
	bool bHasBossPhase3SummonedMinions = false;
	bool bIsCastingBossPhase3FireRain = false;
	bool bIsPreparingBossPhase3IceSpear = false;
	bool bIsPreparingBossPhase3Thunder = false;
	bool bIsBossPhase3SkillRecovering = false;
	bool bHasLastBossPhase3SkillUsed = false;
	bool bHasEnteredPhase3 = false;
	bool bHasLoggedBossPhase3BlinkSlashLocked = false;
	bool bNextBossSkillUseCone = true;
	bool bNextPhase2SkillUseLongSlash = true;
	bool bIsPreparingMageAttack = false;
	bool bIsPreparingThrowerAttack = false;
	float EnemyAttackResolveTime = 0.f;
	float LastEnemyAttackTime = -999.f;
	float MageAttackResolveTime = 0.f;
	float LastMageAttackTime = -999.f;
	float ThrowerAttackResolveTime = 0.f;
	float LastThrowerAttackTime = -999.f;
	float BossSlamResolveTime = 0.f;
	float LastBossSlamTime = -999.f;
	float BossConeResolveTime = 0.f;
	float LastBossConeTime = -999.f;
	float BossBarrageResolveTime = 0.f;
	float LastBossBarrageTime = -999.f;
	float BossPhase2TransitionEndTime = 0.f;
	bool bBossPhase2TeleportCompleted = false;
	float BossLongSlashResolveTime = 0.f;
	float BossLongSlashRecoveryEndTime = 0.f;
	float LastBossLongSlashTime = -999.f;
	float BossRandomSlashStartTime = 0.f;
	float BossRandomSlashNextTime = 0.f;
	float LastBossRandomSlashTime = -999.f;
	int32 BossRandomSlashExecutedCount = 0;
	int32 BossPhase2WaveCurrentLoop = 0;
	int32 BossPhase3UltimateCurrentRound = 0;
	float BossPhase3UltimateExplosionTime = 0.f;
	float BossPhase3FireRainCastEndTime = 0.f;
	float LastBossPhase3FireRainTime = -999.f;
	float BossPhase3IceSpearLaunchTime = 0.f;
	float LastBossPhase3IceSpearTime = -999.f;
	float BossPhase3ThunderLaunchTime = 0.f;
	float LastBossPhase3ThunderTime = -999.f;
	float BossPhase3ThunderPatternRotationDegrees = 0.f;
	float NextBossPhase3SkillAllowedTime = 0.f;
	float BossPhase3SkillRecoveryEndTime = 0.f;
	float BleedDamagePerTickRuntime = 0.f;
	float BleedEndTime = 0.f;
	float NextBleedTickTime = 0.f;
	float BleedTickIntervalRuntime = 1.f;
	TWeakObjectPtr<AActor> PendingAttackTarget;
	TWeakObjectPtr<AActor> PendingBossSlamTarget;
	TWeakObjectPtr<AActor> PendingBossConeTarget;
	TWeakObjectPtr<AActor> PendingBossBarrageTarget;
	TWeakObjectPtr<AActor> PendingBossLongSlashTarget;
	TArray<TWeakObjectPtr<AARPGEnemyBase>> BossPhase3SummonedMinions;
	int32 BossPhase3SummonedMinionTargetCount = 3;
	FVector MageAttackDirection = FVector::ZeroVector;
	FVector PendingThrowerTargetLocation = FVector::ZeroVector;
	FVector BossLongSlashDirection = FVector::ZeroVector;
	FVector BossRandomSlashBaseDirection = FVector::ZeroVector;
	FVector BossPhase3CurrentSafeZoneCenter = FVector::ZeroVector;
	FVector BossPhase3ArenaCenter = FVector::ZeroVector;
	FVector BossPhase3IceSpearBaseDirection = FVector::ZeroVector;
	EBossPhase3SkillType LastBossPhase3SkillUsed = EBossPhase3SkillType::FireRain;
	TArray<FARPGBossBarrageOrb> ActiveBossBarrageOrbs;
	TArray<FARPGPhase2WaveOrb> ActiveBossPhase2WaveOrbs;
	TArray<FARPGPhase3FireRainZone> ActiveBossPhase3FireRainZones;
	TArray<FARPGPhase3IceSpear> ActiveBossPhase3IceSpears;
	TArray<FARPGPhase3ThunderOrb> ActiveBossPhase3ThunderOrbs;
	TArray<FARPGEnemyMageProjectile> ActiveMageProjectiles;
	TArray<FARPGEnemyThrowerFireZone> ActiveThrowerFireZones;
};
