// Copyright Epic Games, Inc. All Rights Reserved.

#include "ARPGGameMode.h"
#include "ARPGPlayerCharacter.h"
#include "ARPGPlayerController.h"

AARPGGameMode::AARPGGameMode()
{
	DefaultPawnClass = AARPGPlayerCharacter::StaticClass();
	PlayerControllerClass = AARPGPlayerController::StaticClass();
}
