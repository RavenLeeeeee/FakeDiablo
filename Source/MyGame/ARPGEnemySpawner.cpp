// Copyright Epic Games, Inc. All Rights Reserved.

#include "ARPGEnemySpawner.h"
#include "ARPGEnemyBase.h"
#include "Engine/TargetPoint.h"
#include "GameFramework/Controller.h"
#include "MyGame.h"

AARPGEnemySpawner::AARPGEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AARPGEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	if (bSpawnOnBeginPlay)
	{
		SpawnEnemies();
	}
}

void AARPGEnemySpawner::SpawnEnemies()
{
	UE_LOG(LogMyGame, Log, TEXT("EnemySpawner started"));

	if (!EnemyClass && EnemyClasses.Num() == 0)
	{
		UE_LOG(LogMyGame, Warning, TEXT("EnemyClass is null and EnemyClasses is empty"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const int32 TargetSpawnCount = FMath::Max(0, SpawnCount);
	int32 SpawnedCount = 0;

	for (int32 SpawnIndex = 0; SpawnIndex < TargetSpawnCount; ++SpawnIndex)
	{
		FVector SpawnLocation = GetActorLocation();
		FRotator SpawnRotation = GetActorRotation();

		if (SpawnPoints.Num() > 0)
		{
			AActor* SpawnPoint = SpawnPoints[SpawnIndex % SpawnPoints.Num()];
			if (!SpawnPoint)
			{
				continue;
			}

			SpawnLocation = SpawnPoint->GetActorLocation();
			SpawnRotation = SpawnPoint->GetActorRotation();
		}
		else
		{
			const float Angle = FMath::FRandRange(0.f, UE_TWO_PI);
			const float Radius = FMath::FRandRange(0.f, SpawnRadius);
			SpawnLocation.X += FMath::Cos(Angle) * Radius;
			SpawnLocation.Y += FMath::Sin(Angle) * Radius;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		TSubclassOf<AARPGEnemyBase> SelectedEnemyClass = EnemyClass;
		if (EnemyClasses.Num() > 0)
		{
			SelectedEnemyClass = EnemyClasses[FMath::RandRange(0, EnemyClasses.Num() - 1)];
			if (!SelectedEnemyClass)
			{
				SelectedEnemyClass = EnemyClass;
			}
		}

		if (!SelectedEnemyClass)
		{
			UE_LOG(LogMyGame, Warning, TEXT("EnemySpawner skipped null enemy class"));
			continue;
		}

		AARPGEnemyBase* SpawnedEnemy = World->SpawnActor<AARPGEnemyBase>(SelectedEnemyClass, SpawnLocation, SpawnRotation, SpawnParameters);
		if (SpawnedEnemy)
		{
			if (SpawnedEnemy->GetController() == nullptr)
			{
				SpawnedEnemy->SpawnDefaultController();
				UE_LOG(LogMyGame, Log, TEXT("Spawned enemy default controller created"));
			}

			++SpawnedCount;
			UE_LOG(LogMyGame, Log, TEXT("Spawned enemy: %s, Controller: %s"),
				*SpawnedEnemy->GetName(),
				SpawnedEnemy->GetController() ? *SpawnedEnemy->GetController()->GetName() : TEXT("None"));
		}
	}

	UE_LOG(LogMyGame, Log, TEXT("EnemySpawner finished: %d enemies spawned"), SpawnedCount);
}
