// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGBossSpawner.generated.h"

class AARPGEnemyBase;

UCLASS(Blueprintable)
class MYGAME_API AARPGBossSpawner : public AActor
{
	GENERATED_BODY()

public:
	AARPGBossSpawner();

	UFUNCTION(BlueprintCallable, Category="Boss")
	AARPGEnemyBase* SpawnBoss();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss")
	TSubclassOf<AARPGEnemyBase> BossClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss")
	AActor* BossSpawnPoint = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss")
	bool bHasSpawnedBoss = false;
};
