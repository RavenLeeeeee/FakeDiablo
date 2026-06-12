// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ARPGHUD.generated.h"

UCLASS()
class MYGAME_API AARPGHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawPlayerHealth(float X, float Y);
	void DrawMissionObjective(float X, float Y);
	void DrawBossHealth();
	void DrawCenterMessage();
};
