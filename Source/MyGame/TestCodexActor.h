// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestCodexActor.generated.h"

UCLASS()
class ATestCodexActor : public AActor
{
	GENERATED_BODY()

public:
	ATestCodexActor();

protected:
	virtual void BeginPlay() override;
};
