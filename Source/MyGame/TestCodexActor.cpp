// Copyright Epic Games, Inc. All Rights Reserved.

#include "TestCodexActor.h"
#include "MyGame.h"

ATestCodexActor::ATestCodexActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATestCodexActor::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogMyGame, Log, TEXT("ATestCodexActor BeginPlay"));
}
