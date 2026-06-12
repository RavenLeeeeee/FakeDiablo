// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGMissionManager.generated.h"

class AARPGEnemyBase;

UENUM(BlueprintType)
enum class EARPGMissionState : uint8
{
	None,
	KillEnemies,
	ReadyForBoss,
	Completed
};

UCLASS(Blueprintable)
class MYGAME_API AARPGMissionManager : public AActor
{
	GENERATED_BODY()

public:
	AARPGMissionManager();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category="Mission")
	void NotifyEnemyKilled(AARPGEnemyBase* DeadEnemy);

	UFUNCTION(BlueprintCallable, Category="Mission")
	void CompleteKillObjective();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mission")
	int32 RequiredEnemyKills = 3;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mission")
	int32 CurrentEnemyKills = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mission")
	EARPGMissionState MissionState = EARPGMissionState::KillEnemies;
};
