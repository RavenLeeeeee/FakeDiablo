// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ARPGBasicEquipmentTypes.h"
#include "ARPGEquipmentPanelWidget.generated.h"

class AARPGPlayerController;
class UButton;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class UWidget;

UCLASS()
class MYGAME_API UARPGEquipmentPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetOwningARPGController(AARPGPlayerController* InController);

	UFUNCTION(BlueprintCallable, Category="Equipment")
	void EnsureBuiltOrRebuildPanel();

	UFUNCTION(BlueprintCallable, Category="Equipment")
	void RefreshEquipmentPanel();

protected:
	virtual void NativeConstruct() override;

private:
	void BuildEquipmentPanel();
	UTextBlock* CreateTextBlock(const FText& Text, const FSlateColor& Color, int32 FontSize);
	UButton* CreateTextButton(TObjectPtr<UTextBlock>& OutTextBlock);
	void AddInventoryButton(int32 ItemIndex, const FARPGSimpleEquipmentItem& Item);
	void AddEquipmentSlotButton(EARPGEquipmentSlot EquipmentSlot, const FString& Label, const FString& ItemName);
	void AddEquipmentEmptyText(const FString& Label);
	USizeBox* WrapRow(UWidget* Content, float Width, float Height);
	FString SlotToText(EARPGEquipmentSlot EquipmentSlot) const;
	FString BonusToText(const FARPGSimpleEquipmentItem& Item) const;
	FString InventoryLineText(const FARPGSimpleEquipmentItem& Item) const;

	UFUNCTION()
	void HandleInventoryItem0Clicked();

	UFUNCTION()
	void HandleInventoryItem1Clicked();

	UFUNCTION()
	void HandleInventoryItem2Clicked();

	UFUNCTION()
	void HandleWeaponSlotClicked();

	UFUNCTION()
	void HandleArmorSlotClicked();

	UFUNCTION()
	void HandleBootsSlotClicked();

	UPROPERTY()
	TObjectPtr<AARPGPlayerController> OwningARPGController;

	UPROPERTY()
	TObjectPtr<UVerticalBox> InventoryBox;

	UPROPERTY()
	TObjectPtr<UVerticalBox> EquipmentBox;

	UPROPERTY()
	TObjectPtr<UTextBlock> InventoryItem0Text;

	UPROPERTY()
	TObjectPtr<UTextBlock> InventoryItem1Text;

	UPROPERTY()
	TObjectPtr<UTextBlock> InventoryItem2Text;

	UPROPERTY()
	TObjectPtr<UTextBlock> WeaponSlotText;

	UPROPERTY()
	TObjectPtr<UTextBlock> ArmorSlotText;

	UPROPERTY()
	TObjectPtr<UTextBlock> BootsSlotText;
};
