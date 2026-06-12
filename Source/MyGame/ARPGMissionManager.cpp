// Copyright Epic Games, Inc. All Rights Reserved.

#include "ARPGMissionManager.h"
#include "ARPGBossSpawner.h"
#include "ARPGEnemyBase.h"
#include "MyGame.h"

AARPGMissionManager::AARPGMissionManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AARPGMissionManager::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogMyGame, Log, TEXT("Mission started"));
	UE_LOG(LogMyGame, Log, TEXT("Kill enemies: %d / %d"), CurrentEnemyKills, RequiredEnemyKills);
}

void AARPGMissionManager::NotifyEnemyKilled(AARPGEnemyBase* DeadEnemy)
{
	if (!DeadEnemy)
	{
		return;
	}

	if (MissionState != EARPGMissionState::KillEnemies)
	{
		return;
	}

	++CurrentEnemyKills;
	UE_LOG(LogMyGame, Log, TEXT("Enemy killed: %d / %d"), CurrentEnemyKills, RequiredEnemyKills);

	if (CurrentEnemyKills >= RequiredEnemyKills)
	{
		CompleteKillObjective();
	}
}

void AARPGMissionManager::NotifyBossKilled(AARPGEnemyBase* DeadBoss)
{
	if (!DeadBoss)
	{
		return;
	}

	if (MissionState != EARPGMissionState::ReadyForBoss)
	{
		return;
	}

	MissionState = EARPGMissionState::Completed;

	UE_LOG(LogMyGame, Log, TEXT("Boss defeated"));
	UE_LOG(LogMyGame, Log, TEXT("Mission completed"));
	UE_LOG(LogMyGame, Log, TEXT("Victory"));
}

void AARPGMissionManager::CompleteKillObjective()
{
	MissionState = EARPGMissionState::ReadyForBoss;

	UE_LOG(LogMyGame, Log, TEXT("Kill objective completed"));
	UE_LOG(LogMyGame, Log, TEXT("Boss stage unlocked"));

	if (BossSpawner)
	{
		BossSpawner->SpawnBoss();
	}
	else
	{
		UE_LOG(LogMyGame, Warning, TEXT("BossSpawner not assigned"));
	}
}
