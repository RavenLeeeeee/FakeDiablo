// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGEnemySpawner.generated.h"

class AARPGEnemyBase;

UCLASS(Blueprintable)
class MYGAME_API AARPGEnemySpawner : public AActor
{
	GENERATED_BODY()

public:
	AARPGEnemySpawner();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category="Spawner")
	void SpawnEnemies();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner")
	TSubclassOf<AARPGEnemyBase> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner")
	int32 SpawnCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner")
	float SpawnRadius = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner")
	bool bSpawnOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner")
	TArray<AActor*> SpawnPoints;
};
