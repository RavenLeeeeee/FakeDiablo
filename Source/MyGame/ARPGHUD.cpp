// Copyright Epic Games, Inc. All Rights Reserved.

#include "ARPGHUD.h"
#include "ARPGEnemyBase.h"
#include "ARPGHealthComponent.h"
#include "ARPGMissionManager.h"
#include "ARPGPlayerController.h"
#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"

void AARPGHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	const float LeftX = 30.f;
	const float TopY = 30.f;

	DrawPlayerHealth(LeftX, TopY);
	DrawManaStatus(LeftX, TopY + 28.f);
	DrawPotionStatus(LeftX, TopY + 56.f);
	DrawMissionObjective(LeftX, TopY + 112.f);
	DrawEmpowerStatus(LeftX, TopY + 140.f);
	DrawWhirlwindStatus(LeftX, TopY + 168.f);
	DrawBossHealth();
	DrawCenterMessage();
}

void AARPGHUD::DrawPlayerHealth(float X, float Y)
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	UARPGHealthComponent* PlayerHealth = PlayerPawn ? PlayerPawn->FindComponentByClass<UARPGHealthComponent>() : nullptr;
	if (!PlayerHealth)
	{
		DrawText(TEXT("Player HP: -- / --"), FColor::White, X, Y);
		return;
	}

	const FString HealthText = FString::Printf(TEXT("Player HP: %.0f / %.0f"),
		PlayerHealth->GetCurrentHealth(),
		PlayerHealth->GetMaxHealth());
	DrawText(HealthText, FColor::White, X, Y);
}

void AARPGHUD::DrawMissionObjective(float X, float Y)
{
	TArray<AActor*> MissionManagers;
	UGameplayStatics::GetAllActorsOfClass(this, AARPGMissionManager::StaticClass(), MissionManagers);

	AARPGMissionManager* MissionManager = MissionManagers.Num() > 0 ? Cast<AARPGMissionManager>(MissionManagers[0]) : nullptr;
	if (!MissionManager)
	{
		DrawText(TEXT("Objective: None"), FColor::White, X, Y);
		return;
	}

	FString ObjectiveText;
	switch (MissionManager->GetMissionState())
	{
	case EARPGMissionState::KillEnemies:
		ObjectiveText = FString::Printf(TEXT("Objective: Kill enemies %d / %d"),
			MissionManager->GetCurrentEnemyKills(),
			MissionManager->GetRequiredEnemyKills());
		break;
	case EARPGMissionState::ReadyForBoss:
		ObjectiveText = TEXT("Objective: Defeat the Boss");
		break;
	case EARPGMissionState::Completed:
		ObjectiveText = TEXT("Victory");
		break;
	case EARPGMissionState::None:
	default:
		ObjectiveText = TEXT("Objective: None");
		break;
	}

	DrawText(ObjectiveText, FColor::White, X, Y);
}

void AARPGHUD::DrawEmpowerStatus(float X, float Y)
{
	const AARPGPlayerController* ARPGController = Cast<AARPGPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (!ARPGController)
	{
		DrawText(TEXT("Empower: --"), FColor::White, X, Y);
		return;
	}

	if (ARPGController->IsEmpowered())
	{
		DrawText(TEXT("Empower: Active"), FColor::Green, X, Y);
		return;
	}

	const float CooldownRemaining = ARPGController->GetEmpowerCooldownRemaining();
	if (CooldownRemaining > 0.f)
	{
		DrawText(FString::Printf(TEXT("Empower: Cooldown %.1fs"), CooldownRemaining), FColor::Yellow, X, Y);
		return;
	}

	DrawText(TEXT("Empower: Ready"), FColor::White, X, Y);
}

void AARPGHUD::DrawManaStatus(float X, float Y)
{
	const AARPGPlayerController* ARPGController = Cast<AARPGPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (!ARPGController)
	{
		DrawText(TEXT("Mana: -- / --"), FColor::White, X, Y);
		return;
	}

	DrawText(FString::Printf(TEXT("Mana: %.0f / %.0f"), ARPGController->GetCurrentMana(), ARPGController->GetMaxMana()), FColor::White, X, Y);
}

void AARPGHUD::DrawPotionStatus(float X, float Y)
{
	const AARPGPlayerController* ARPGController = Cast<AARPGPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (!ARPGController)
	{
		DrawText(TEXT("Health Potion [1]: --"), FColor::White, X, Y);
		DrawText(TEXT("Mana Potion [2]: --"), FColor::White, X, Y + 28.f);
		return;
	}

	DrawText(FString::Printf(TEXT("Health Potion [1]: %d"), ARPGController->GetHealthPotionCount()), FColor::White, X, Y);
	DrawText(FString::Printf(TEXT("Mana Potion [2]: %d"), ARPGController->GetManaPotionCount()), FColor::White, X, Y + 28.f);
}

void AARPGHUD::DrawWhirlwindStatus(float X, float Y)
{
	const AARPGPlayerController* ARPGController = Cast<AARPGPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (!ARPGController)
	{
		DrawText(TEXT("Whirlwind: --"), FColor::White, X, Y);
		return;
	}

	DrawText(ARPGController->IsWhirlwinding() ? TEXT("Whirlwind: Active") : TEXT("Whirlwind: Ready"),
		ARPGController->IsWhirlwinding() ? FColor::Orange : FColor::White,
		X,
		Y);
}

void AARPGHUD::DrawBossHealth()
{
	TArray<AActor*> EnemyActors;
	UGameplayStatics::GetAllActorsOfClass(this, AARPGEnemyBase::StaticClass(), EnemyActors);

	AARPGEnemyBase* BossEnemy = nullptr;
	for (AActor* EnemyActor : EnemyActors)
	{
		AARPGEnemyBase* Enemy = Cast<AARPGEnemyBase>(EnemyActor);
		if (!Enemy || Enemy->IsTemplate() || Enemy->GetName().StartsWith(TEXT("Default__")))
		{
			continue;
		}

		if (Enemy->bIsBoss && !Enemy->IsDead())
		{
			UARPGHealthComponent* CandidateHealth = Enemy->GetHealthComponent();
			if (CandidateHealth && CandidateHealth->GetOwner() == Enemy && !CandidateHealth->IsTemplate())
			{
				BossEnemy = Enemy;
				break;
			}
		}
	}

	if (!BossEnemy)
	{
		return;
	}

	UARPGHealthComponent* BossHealth = BossEnemy->GetHealthComponent();
	if (!BossHealth)
	{
		return;
	}

	if (!BossHealth->IsHealthInitialized())
	{
		BossHealth->InitializeHealth(false);
	}

	const float MaxHealth = FMath::Max(1.f, BossHealth->GetMaxHealth());
	const float CurrentHealth = BossHealth->GetCurrentHealth();

	const float HealthPercent = FMath::Clamp(CurrentHealth / MaxHealth, 0.f, 1.f);

	const float BarWidth = 420.f;
	const float BarHeight = 18.f;
	const float BarX = (Canvas->SizeX - BarWidth) * 0.5f;
	const float BarY = 42.f;

	DrawText(FString::Printf(TEXT("Boss HP: %.0f / %.0f"), CurrentHealth, MaxHealth),
		FColor::White,
		BarX,
		BarY - 24.f);
	DrawRect(FLinearColor(0.08f, 0.08f, 0.08f, 0.85f), BarX, BarY, BarWidth, BarHeight);
	DrawRect(FLinearColor(0.85f, 0.05f, 0.03f, 0.95f), BarX, BarY, BarWidth * HealthPercent, BarHeight);
}

void AARPGHUD::DrawCenterMessage()
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	UARPGHealthComponent* PlayerHealth = PlayerPawn ? PlayerPawn->FindComponentByClass<UARPGHealthComponent>() : nullptr;

	TArray<AActor*> MissionManagers;
	UGameplayStatics::GetAllActorsOfClass(this, AARPGMissionManager::StaticClass(), MissionManagers);
	AARPGMissionManager* MissionManager = MissionManagers.Num() > 0 ? Cast<AARPGMissionManager>(MissionManagers[0]) : nullptr;

	FString CenterMessage;
	FColor MessageColor = FColor::White;
	if (MissionManager && MissionManager->GetMissionState() == EARPGMissionState::Completed)
	{
		CenterMessage = TEXT("Victory");
		MessageColor = FColor::Yellow;
	}
	else if (PlayerHealth && PlayerHealth->IsDead())
	{
		CenterMessage = TEXT("Player died");
		MessageColor = FColor::Red;
	}

	if (!CenterMessage.IsEmpty())
	{
		DrawText(CenterMessage, MessageColor, Canvas->SizeX * 0.5f - 80.f, Canvas->SizeY * 0.5f - 20.f, nullptr, 1.8f);
	}
}
