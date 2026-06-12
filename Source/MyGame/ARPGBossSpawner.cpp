// Copyright Epic Games, Inc. All Rights Reserved.

#include "ARPGBossSpawner.h"
#include "ARPGEnemyBase.h"
#include "GameFramework/Controller.h"
#include "MyGame.h"

AARPGBossSpawner::AARPGBossSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

AARPGEnemyBase* AARPGBossSpawner::SpawnBoss()
{
	if (bHasSpawnedBoss)
	{
		UE_LOG(LogMyGame, Log, TEXT("Boss already spawned"));
		return nullptr;
	}

	if (!BossClass)
	{
		UE_LOG(LogMyGame, Warning, TEXT("BossClass is null"));
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogMyGame, Warning, TEXT("Boss spawn failed"));
		return nullptr;
	}

	FVector SpawnLocation = GetActorLocation();
	FRotator SpawnRotation = GetActorRotation();

	if (BossSpawnPoint)
	{
		SpawnLocation = BossSpawnPoint->GetActorLocation();
		SpawnRotation = BossSpawnPoint->GetActorRotation();
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AARPGEnemyBase* SpawnedBoss = World->SpawnActor<AARPGEnemyBase>(BossClass, SpawnLocation, SpawnRotation, SpawnParameters);
	if (!SpawnedBoss)
	{
		UE_LOG(LogMyGame, Warning, TEXT("Boss spawn failed"));
		return nullptr;
	}

	bHasSpawnedBoss = true;
	SpawnedBoss->bIsBoss = true;
	UE_LOG(LogMyGame, Log, TEXT("Boss marked as boss"));

	if (SpawnedBoss->GetController() == nullptr)
	{
		SpawnedBoss->SpawnDefaultController();
	}

	UE_LOG(LogMyGame, Log, TEXT("Boss spawned: %s"), *SpawnedBoss->GetName());
	return SpawnedBoss;
}
