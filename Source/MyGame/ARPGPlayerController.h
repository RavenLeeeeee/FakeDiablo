// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ARPGBasicEquipmentTypes.h"
#include "GameFramework/PlayerController.h"
#include "ARPGPlayerController.generated.h"

class AARPGPlayerCharacter;
class UARPGManaComponent;
class UARPGEquipmentPanelWidget;

/**
 * Minimal top-down ARPG player controller prototype.
 */
UCLASS()
class AARPGPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AARPGPlayerController();

	UFUNCTION(BlueprintCallable, Category="Empower")
	bool IsEmpowered() const;

	UFUNCTION(BlueprintCallable, Category="Empower")
	float GetEmpowerCooldownRemaining() const;

	UFUNCTION(BlueprintCallable, Category="Mana")
	float GetCurrentMana() const;

	UFUNCTION(BlueprintCallable, Category="Mana")
	float GetMaxMana() const;

	UFUNCTION(BlueprintCallable, Category="Whirlwind")
	bool IsWhirlwinding() const;

	UFUNCTION(BlueprintCallable, Category="Potion")
	int32 GetHealthPotionCount() const;

	UFUNCTION(BlueprintCallable, Category="Potion")
	int32 GetManaPotionCount() const;

	UFUNCTION(BlueprintCallable, Category="Movement")
	void ApplyMoveSpeedSlow(float Multiplier, float Duration);

	UFUNCTION(BlueprintCallable, Category="Status|Freeze")
	void ApplyFreezeBuildup(float Amount);

	UFUNCTION(BlueprintCallable, Category="Status|Freeze")
	bool IsFrozen() const { return bIsFrozen; }

	UFUNCTION(BlueprintCallable, Category="Status|Shock")
	void ApplyShockBuildup(float Amount);

	UFUNCTION(BlueprintCallable, Category="Status|Shock")
	bool IsShocked() const { return bIsShocked; }

	UFUNCTION(BlueprintCallable, Category="Status|Shock")
	float GetDamageTakenMultiplier() const;

	UFUNCTION(BlueprintCallable, Category="Equipment")
	void EquipDemoInventoryItemByIndex(int32 ItemIndex);

	UFUNCTION(BlueprintCallable, Category="Equipment")
	void ToggleEquipDemoInventoryItemByIndex(int32 ItemIndex);

	UFUNCTION(BlueprintCallable, Category="Equipment")
	void UnequipSlot(EARPGEquipmentSlot Slot);

	UFUNCTION(BlueprintCallable, Category="Equipment")
	bool IsItemEquipped(const FARPGSimpleEquipmentItem& Item) const;

	UFUNCTION(BlueprintCallable, Category="Equipment")
	float GetEquipmentDamageMultiplier() const;

	UFUNCTION(BlueprintCallable, Category="Equipment")
	bool IsEquipmentPanelOpen() const { return bIsEquipmentPanelOpen; }

	const TArray<FARPGSimpleEquipmentItem>& GetDemoInventoryItems() const { return DemoInventoryItems; }
	bool HasEquippedWeapon() const { return bHasEquippedWeapon; }
	bool HasEquippedHelmet() const { return bHasEquippedHelmet; }
	bool HasEquippedArmor() const { return bHasEquippedArmor; }
	bool HasEquippedLegs() const { return bHasEquippedLegs; }
	bool HasEquippedBoots() const { return bHasEquippedBoots; }
	bool HasEquippedAmulet() const { return bHasEquippedAmulet; }
	bool HasEquippedRing1() const { return bHasEquippedRing1; }
	bool HasEquippedRing2() const { return bHasEquippedRing2; }
	const FARPGSimpleEquipmentItem& GetEquippedWeapon() const { return EquippedWeapon; }
	const FARPGSimpleEquipmentItem& GetEquippedHelmet() const { return EquippedHelmet; }
	const FARPGSimpleEquipmentItem& GetEquippedArmor() const { return EquippedArmor; }
	const FARPGSimpleEquipmentItem& GetEquippedLegs() const { return EquippedLegs; }
	const FARPGSimpleEquipmentItem& GetEquippedBoots() const { return EquippedBoots; }
	const FARPGSimpleEquipmentItem& GetEquippedAmulet() const { return EquippedAmulet; }
	const FARPGSimpleEquipmentItem& GetEquippedRing1() const { return EquippedRing1; }
	const FARPGSimpleEquipmentItem& GetEquippedRing2() const { return EquippedRing2; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dodge")
	float DodgeDistance = 520.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dodge")
	float DodgeDuration = 0.34f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dodge")
	float DodgeCooldown = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill")
	float AreaSkillRadius = 260.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill")
	float AreaSkillDamage = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill")
	float AreaSkillCooldown = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill")
	float AreaSkillCastLockDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill")
	float AreaSkillManaCost = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Empower")
	float EmpowerDuration = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Empower")
	float EmpowerCooldown = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Empower")
	float EmpowerDamageMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Empower")
	float EmpowerMoveSpeedBonus = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Empower")
	float EmpowerManaCost = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Whirlwind")
	float WhirlwindStartManaCost = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Whirlwind")
	float WhirlwindManaCostPerSecond = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Whirlwind")
	float WhirlwindRadius = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Whirlwind")
	float WhirlwindDamage = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Whirlwind")
	float WhirlwindHitInterval = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status|Freeze")
	float FreezeThreshold = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status|Freeze")
	float FreezeDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status|Shock")
	float ShockThreshold = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status|Shock")
	float ShockDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status|Shock")
	float ShockDamageTakenMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Testing")
	float TemporaryPlayerMaxHealthForBossTesting = 10000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PiercingSkill")
	float PiercingSkillRange = 425.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PiercingSkill")
	float PiercingSkillWidth = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PiercingSkill")
	float PiercingSkillDamage = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PiercingSkill")
	float PiercingSkillManaCost = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PiercingSkill")
	float PiercingSkillCooldown = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PiercingSkill")
	float PiercingSkillCastLockDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PiercingSkill")
	float BleedDamagePerTick = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PiercingSkill")
	float BleedDuration = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PiercingSkill")
	float BleedTickInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Potion")
	int32 HealthPotionCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Potion")
	float HealthPotionRestorePercent = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Potion")
	int32 ManaPotionCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Potion")
	float ManaPotionRestorePercent = 0.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Equipment")
	FARPGSimpleEquipmentItem EquippedWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Equipment")
	FARPGSimpleEquipmentItem EquippedHelmet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Equipment")
	FARPGSimpleEquipmentItem EquippedArmor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Equipment")
	FARPGSimpleEquipmentItem EquippedLegs;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Equipment")
	FARPGSimpleEquipmentItem EquippedBoots;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Equipment")
	FARPGSimpleEquipmentItem EquippedAmulet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Equipment")
	FARPGSimpleEquipmentItem EquippedRing1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Equipment")
	FARPGSimpleEquipmentItem EquippedRing2;

private:
	FVector2D GetKeyboardMovementInput() const;
	void HandleLeftClickPressed();
	void HandleDodgePressed();
	void HandleAreaSkillPressed();
	void HandleEmpowerPressed();
	void HandleWhirlwindPressed();
	void HandlePiercingSkillPressed();
	void HandleHealthPotionPressed();
	void HandleManaPotionPressed();
	void ToggleEquipmentPanel();
	void CreateEquipmentPanelIfNeeded();
	void ShowEquipmentPanel();
	void HideEquipmentPanel();
	void InitializeDemoEquipmentInventory();
	void EquipItem(const FARPGSimpleEquipmentItem& Item);
	void RecalculateEquipmentBonuses();
	void StartEmpower();
	void EndEmpower();
	void ClearMoveSpeedSlow();
	void UpdateMovementSpeedModifiers();
	void StartFrozenState(float Duration);
	void ClearFrozenState();
	void StartShockedState(float Duration);
	void ClearShockedState();
	void ApplyTemporaryPlayerMaxHealthForBossTesting();
	void StartWhirlwind();
	void StopWhirlwind(const FString& Reason);
	void HandleWhirlwindTick(float DeltaTime);
	void PerformWhirlwindHit();
	FVector GetPiercingSkillDirection(const AARPGPlayerCharacter* ARPGCharacter);
	void PerformPiercingSkill(const FVector& Direction);
	bool ConsumeMana(float ManaCost);
	void HandleDodgeTick(float DeltaTime);
	void EndDodge();
	FVector GetCurrentDodgeDirection();
	void UpdateClickMoveMovement(float DeltaTime);
	void StopARPGCharacterMovement();
	void SmoothFaceDirection(const FVector& Direction, float DeltaTime);
	void UpdateMouseFacing(float DeltaTime);
	void UpdateActionInput();
	void PerformBasicAttack();
	FVector GetBasicAttackDirection(const AARPGPlayerCharacter* ARPGCharacter) const;
	FVector GetAreaSkillCenter(const AARPGPlayerCharacter* ARPGCharacter);
	void PerformAreaSkill(const FVector& AreaCenter);
	bool GetCursorWorldHit(FHitResult& OutHitResult);

	AARPGPlayerCharacter* GetARPGCharacter() const;
	UARPGManaComponent* GetManaComponent() const;

	FVector ClickMoveTarget = FVector::ZeroVector;
	bool bHasClickMoveTarget = false;
	float ClickMoveAcceptanceRadius = 35.f;
	float FacingInterpSpeed = 12.f;
	float BasicAttackRange = 220.f;
	float BasicAttackRadius = 80.f;
	float BasicAttackDamage = 25.f;
	float BasicAttackCooldown = 0.6f;
	float BasicAttackLockDuration = 0.35f;
	float LastBasicAttackTime = -1000.f;
	float BasicAttackLockEndTime = 0.f;
	uint64 LastHandledLeftClickFrame = 0;

	bool bIsDodging = false;
	float DodgeStartTime = 0.f;
	float DodgeEndTime = 0.f;
	float LastDodgeTime = -999.f;
	float ActiveDodgeSpeed = 0.f;
	FVector DodgeDirection = FVector::ZeroVector;
	ECollisionResponse SavedPawnCollisionResponse = ECR_Block;
	bool bSavedPawnCollisionResponseValid = false;

	bool bIsBasicAttackLocked = false;
	bool bWasBasicAttackPressed = false;

	bool bIsAreaSkillCasting = false;
	float AreaSkillCastEndTime = 0.f;
	float LastAreaSkillTime = -999.f;

	bool bIsEmpowered = false;
	float EmpowerEndTime = 0.f;
	float LastEmpowerTime = -999.f;

	bool bIsMoveSpeedSlowed = false;
	float MoveSpeedSlowEndTime = 0.f;
	float MoveSpeedSlowMultiplier = 1.0f;
	float BaseNormalMaxWalkSpeed = 0.f;

	TArray<FARPGSimpleEquipmentItem> DemoInventoryItems;
	bool bHasEquippedWeapon = false;
	bool bHasEquippedHelmet = false;
	bool bHasEquippedArmor = false;
	bool bHasEquippedLegs = false;
	bool bHasEquippedBoots = false;
	bool bHasEquippedAmulet = false;
	bool bHasEquippedRing1 = false;
	bool bHasEquippedRing2 = false;
	float EquipmentDamageMultiplierBonus = 0.f;
	float EquipmentMaxHealthBonus = 0.f;
	float AppliedEquipmentMaxHealthBonus = 0.f;
	float EquipmentMoveSpeedBonus = 0.f;
	bool bIsEquipmentPanelOpen = false;
	bool bDemoInventoryInitialized = false;

	UPROPERTY()
	TObjectPtr<UARPGEquipmentPanelWidget> EquipmentPanelWidget;

	float FreezeAccumulation = 0.f;
	bool bIsFrozen = false;
	float FrozenEndTime = 0.f;

	float ShockAccumulation = 0.f;
	bool bIsShocked = false;
	float ShockEndTime = 0.f;

	bool bHasAppliedTemporaryPlayerMaxHealth = false;

	bool bIsWhirlwinding = false;
	float LastWhirlwindHitTime = -999.f;

	bool bIsPiercingSkillCasting = false;
	float PiercingSkillCastEndTime = 0.f;
	float LastPiercingSkillTime = -999.f;
};
