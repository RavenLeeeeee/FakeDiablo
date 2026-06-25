// Copyright Epic Games, Inc. All Rights Reserved.

#include "ARPGEquipmentPanelWidget.h"
#include "ARPGPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UARPGEquipmentPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UE_LOG(LogTemp, Warning, TEXT("EquipmentPanelWidget NativeConstruct called"));
	EnsureBuiltOrRebuildPanel();
	RefreshEquipmentPanel();
}

void UARPGEquipmentPanelWidget::SetOwningARPGController(AARPGPlayerController* InController)
{
	OwningARPGController = InController;
}

void UARPGEquipmentPanelWidget::EnsureBuiltOrRebuildPanel()
{
	if (!WidgetTree)
	{
		UE_LOG(LogTemp, Error, TEXT("EquipmentPanelWidget EnsureBuilt failed: WidgetTree is null."));
		return;
	}

	if (!WidgetTree->RootWidget || !InventoryBox || !EquipmentBox)
	{
		BuildEquipmentPanel();
	}

	UE_LOG(LogTemp, Warning, TEXT("EquipmentPanelWidget EnsureBuilt completed."));
}

void UARPGEquipmentPanelWidget::BuildEquipmentPanel()
{
	if (!WidgetTree)
	{
		return;
	}

	InventoryBox = nullptr;
	EquipmentBox = nullptr;
	InventoryItem0Text = nullptr;
	InventoryItem1Text = nullptr;
	InventoryItem2Text = nullptr;
	WeaponSlotText = nullptr;
	ArmorSlotText = nullptr;
	BootsSlotText = nullptr;

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("EquipmentRootCanvas"));
	RootCanvas->SetVisibility(ESlateVisibility::Visible);
	WidgetTree->RootWidget = RootCanvas;

	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("EquipmentPanelBorder"));
	PanelBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.025f, 0.94f));
	PanelBorder->SetPadding(FMargin(24.f));
	PanelBorder->SetVisibility(ESlateVisibility::Visible);

	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(PanelBorder);
	PanelSlot->SetAutoSize(false);
	PanelSlot->SetPosition(FVector2D(200.f, 120.f));
	PanelSlot->SetSize(FVector2D(760.f, 520.f));

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EquipmentRootBox"));
	RootBox->SetVisibility(ESlateVisibility::Visible);
	PanelBorder->SetContent(RootBox);

	UTextBlock* TitleText = CreateTextBlock(FText::FromString(TEXT("装备")), FSlateColor(FLinearColor::White), 24);
	RootBox->AddChildToVerticalBox(TitleText)->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));

	UHorizontalBox* ColumnsBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("EquipmentColumnsBox"));
	UVerticalBoxSlot* ColumnsSlot = RootBox->AddChildToVerticalBox(ColumnsBox);
	ColumnsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	USizeBox* InventorySizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InventorySizeBox"));
	InventorySizeBox->SetWidthOverride(360.f);
	UHorizontalBoxSlot* InventoryColumnSlot = ColumnsBox->AddChildToHorizontalBox(InventorySizeBox);
	InventoryColumnSlot->SetPadding(FMargin(0.f, 0.f, 36.f, 0.f));

	InventoryBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InventoryBox"));
	InventoryBox->SetVisibility(ESlateVisibility::Visible);
	InventorySizeBox->SetContent(InventoryBox);

	USizeBox* EquipmentSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("EquipmentSizeBox"));
	EquipmentSizeBox->SetWidthOverride(320.f);
	ColumnsBox->AddChildToHorizontalBox(EquipmentSizeBox);

	EquipmentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EquipmentBox"));
	EquipmentBox->SetVisibility(ESlateVisibility::Visible);
	EquipmentSizeBox->SetContent(EquipmentBox);

	UTextBlock* HintText = CreateTextBlock(FText::FromString(TEXT("点击背包装备穿戴，点击装备栏装备脱下，按 I 关闭")), FSlateColor(FLinearColor::Yellow), 16);
	RootBox->AddChildToVerticalBox(HintText)->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));

	UE_LOG(LogTemp, Warning, TEXT("EquipmentPanelWidget BuildEquipmentPanel completed"));
}

UTextBlock* UARPGEquipmentPanelWidget::CreateTextBlock(const FText& Text, const FSlateColor& Color, int32 FontSize)
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TextBlock->SetText(Text);
	TextBlock->SetColorAndOpacity(Color);
	TextBlock->SetJustification(ETextJustify::Left);
	TextBlock->SetVisibility(ESlateVisibility::Visible);

	FSlateFontInfo FontInfo = TextBlock->GetFont();
	FontInfo.Size = FontSize;
	TextBlock->SetFont(FontInfo);

	return TextBlock;
}

UButton* UARPGEquipmentPanelWidget::CreateTextButton(TObjectPtr<UTextBlock>& OutTextBlock)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->SetClickMethod(EButtonClickMethod::DownAndUp);
	Button->SetVisibility(ESlateVisibility::Visible);

	OutTextBlock = CreateTextBlock(FText::GetEmpty(), FSlateColor(FLinearColor::White), 16);
	OutTextBlock->SetJustification(ETextJustify::Left);
	Button->SetContent(OutTextBlock);

	return Button;
}

USizeBox* UARPGEquipmentPanelWidget::WrapRow(UWidget* Content, float Width, float Height)
{
	USizeBox* RowBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	RowBox->SetWidthOverride(Width);
	RowBox->SetHeightOverride(Height);
	RowBox->SetVisibility(ESlateVisibility::Visible);
	RowBox->SetContent(Content);
	return RowBox;
}

void UARPGEquipmentPanelWidget::RefreshEquipmentPanel()
{
	UE_LOG(LogTemp, Warning, TEXT("EquipmentPanelWidget RefreshEquipmentPanel called"));

	if (!InventoryBox || !EquipmentBox)
	{
		EnsureBuiltOrRebuildPanel();
	}

	if (!OwningARPGController)
	{
		return;
	}

	if (!InventoryBox || !EquipmentBox)
	{
		UE_LOG(LogTemp, Error, TEXT("EquipmentPanelWidget Refresh failed: InventoryBox or EquipmentBox is null."));
		return;
	}

	InventoryBox->ClearChildren();
	EquipmentBox->ClearChildren();

	InventoryBox->AddChildToVerticalBox(CreateTextBlock(FText::FromString(TEXT("背包")), FSlateColor(FLinearColor::White), 22));

	int32 VisibleInventoryCount = 0;
	const TArray<FARPGSimpleEquipmentItem>& Items = OwningARPGController->GetDemoInventoryItems();
	for (int32 ItemIndex = 0; ItemIndex < Items.Num(); ++ItemIndex)
	{
		const FARPGSimpleEquipmentItem& Item = Items[ItemIndex];
		if (OwningARPGController->IsItemEquipped(Item))
		{
			continue;
		}

		AddInventoryButton(ItemIndex, Item);
		++VisibleInventoryCount;
	}

	if (VisibleInventoryCount == 0)
	{
		UTextBlock* EmptyText = CreateTextBlock(FText::FromString(TEXT("背包为空")), FSlateColor(FLinearColor(0.72f, 0.72f, 0.72f, 1.f)), 16);
		InventoryBox->AddChildToVerticalBox(WrapRow(EmptyText, 340.f, 38.f))->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
	}

	EquipmentBox->AddChildToVerticalBox(CreateTextBlock(FText::FromString(TEXT("装备栏")), FSlateColor(FLinearColor::White), 22));

	if (OwningARPGController->HasEquippedWeapon())
	{
		AddEquipmentSlotButton(EARPGEquipmentSlot::Weapon, TEXT("Weapon"), OwningARPGController->GetEquippedWeapon().DisplayName.ToString());
	}
	else
	{
		AddEquipmentEmptyText(TEXT("Weapon"));
	}

	AddEquipmentEmptyText(TEXT("Helmet"));

	if (OwningARPGController->HasEquippedArmor())
	{
		AddEquipmentSlotButton(EARPGEquipmentSlot::Armor, TEXT("Armor"), OwningARPGController->GetEquippedArmor().DisplayName.ToString());
	}
	else
	{
		AddEquipmentEmptyText(TEXT("Armor"));
	}

	AddEquipmentEmptyText(TEXT("Legs"));

	if (OwningARPGController->HasEquippedBoots())
	{
		AddEquipmentSlotButton(EARPGEquipmentSlot::Boots, TEXT("Boots"), OwningARPGController->GetEquippedBoots().DisplayName.ToString());
	}
	else
	{
		AddEquipmentEmptyText(TEXT("Boots"));
	}

	AddEquipmentEmptyText(TEXT("Amulet"));
	AddEquipmentEmptyText(TEXT("Ring 1"));
	AddEquipmentEmptyText(TEXT("Ring 2"));
}

void UARPGEquipmentPanelWidget::AddInventoryButton(int32 ItemIndex, const FARPGSimpleEquipmentItem& Item)
{
	TObjectPtr<UTextBlock> ButtonText = nullptr;
	UButton* Button = CreateTextButton(ButtonText);
	ButtonText->SetText(FText::FromString(InventoryLineText(Item)));

	switch (ItemIndex)
	{
	case 0:
		Button->OnClicked.AddDynamic(this, &UARPGEquipmentPanelWidget::HandleInventoryItem0Clicked);
		InventoryItem0Text = ButtonText;
		break;
	case 1:
		Button->OnClicked.AddDynamic(this, &UARPGEquipmentPanelWidget::HandleInventoryItem1Clicked);
		InventoryItem1Text = ButtonText;
		break;
	case 2:
		Button->OnClicked.AddDynamic(this, &UARPGEquipmentPanelWidget::HandleInventoryItem2Clicked);
		InventoryItem2Text = ButtonText;
		break;
	default:
		break;
	}

	InventoryBox->AddChildToVerticalBox(WrapRow(Button, 340.f, 40.f))->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
}

void UARPGEquipmentPanelWidget::AddEquipmentSlotButton(EARPGEquipmentSlot EquipmentSlot, const FString& Label, const FString& ItemName)
{
	TObjectPtr<UTextBlock> ButtonText = nullptr;
	UButton* Button = CreateTextButton(ButtonText);
	ButtonText->SetText(FText::FromString(FString::Printf(TEXT("%s: %s"), *Label, *ItemName)));

	switch (EquipmentSlot)
	{
	case EARPGEquipmentSlot::Weapon:
		Button->OnClicked.AddDynamic(this, &UARPGEquipmentPanelWidget::HandleWeaponSlotClicked);
		WeaponSlotText = ButtonText;
		break;
	case EARPGEquipmentSlot::Armor:
		Button->OnClicked.AddDynamic(this, &UARPGEquipmentPanelWidget::HandleArmorSlotClicked);
		ArmorSlotText = ButtonText;
		break;
	case EARPGEquipmentSlot::Boots:
		Button->OnClicked.AddDynamic(this, &UARPGEquipmentPanelWidget::HandleBootsSlotClicked);
		BootsSlotText = ButtonText;
		break;
	default:
		break;
	}

	EquipmentBox->AddChildToVerticalBox(WrapRow(Button, 300.f, 36.f))->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
}

void UARPGEquipmentPanelWidget::AddEquipmentEmptyText(const FString& Label)
{
	UTextBlock* EmptyText = CreateTextBlock(FText::FromString(FString::Printf(TEXT("%s: Empty"), *Label)), FSlateColor(FLinearColor::White), 16);
	EquipmentBox->AddChildToVerticalBox(WrapRow(EmptyText, 300.f, 34.f))->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
}

FString UARPGEquipmentPanelWidget::SlotToText(EARPGEquipmentSlot EquipmentSlot) const
{
	switch (EquipmentSlot)
	{
	case EARPGEquipmentSlot::Weapon:
		return TEXT("武器");
	case EARPGEquipmentSlot::Helmet:
		return TEXT("头盔");
	case EARPGEquipmentSlot::Armor:
		return TEXT("护甲");
	case EARPGEquipmentSlot::Legs:
		return TEXT("腿甲");
	case EARPGEquipmentSlot::Boots:
		return TEXT("鞋子");
	case EARPGEquipmentSlot::Amulet:
		return TEXT("项链");
	case EARPGEquipmentSlot::Ring1:
		return TEXT("戒指 1");
	case EARPGEquipmentSlot::Ring2:
		return TEXT("戒指 2");
	default:
		return TEXT("--");
	}
}

FString UARPGEquipmentPanelWidget::BonusToText(const FARPGSimpleEquipmentItem& Item) const
{
	if (Item.DamageMultiplierBonus > 0.f)
	{
		return FString::Printf(TEXT("伤害 +%.0f%%"), Item.DamageMultiplierBonus * 100.f);
	}
	if (Item.MaxHealthBonus > 0.f)
	{
		return FString::Printf(TEXT("生命 +%.0f"), Item.MaxHealthBonus);
	}
	if (Item.MoveSpeedBonus > 0.f)
	{
		return FString::Printf(TEXT("移速 +%.0f"), Item.MoveSpeedBonus);
	}
	return TEXT("");
}

FString UARPGEquipmentPanelWidget::InventoryLineText(const FARPGSimpleEquipmentItem& Item) const
{
	return FString::Printf(TEXT("%s  %s  %s"),
		*Item.DisplayName.ToString(),
		*SlotToText(Item.Slot),
		*BonusToText(Item));
}

void UARPGEquipmentPanelWidget::HandleInventoryItem0Clicked()
{
	if (OwningARPGController)
	{
		OwningARPGController->EquipDemoInventoryItemByIndex(0);
		RefreshEquipmentPanel();
	}
}

void UARPGEquipmentPanelWidget::HandleInventoryItem1Clicked()
{
	if (OwningARPGController)
	{
		OwningARPGController->EquipDemoInventoryItemByIndex(1);
		RefreshEquipmentPanel();
	}
}

void UARPGEquipmentPanelWidget::HandleInventoryItem2Clicked()
{
	if (OwningARPGController)
	{
		OwningARPGController->EquipDemoInventoryItemByIndex(2);
		RefreshEquipmentPanel();
	}
}

void UARPGEquipmentPanelWidget::HandleWeaponSlotClicked()
{
	if (OwningARPGController && OwningARPGController->HasEquippedWeapon())
	{
		OwningARPGController->UnequipSlot(EARPGEquipmentSlot::Weapon);
		RefreshEquipmentPanel();
	}
}

void UARPGEquipmentPanelWidget::HandleArmorSlotClicked()
{
	if (OwningARPGController && OwningARPGController->HasEquippedArmor())
	{
		OwningARPGController->UnequipSlot(EARPGEquipmentSlot::Armor);
		RefreshEquipmentPanel();
	}
}

void UARPGEquipmentPanelWidget::HandleBootsSlotClicked()
{
	if (OwningARPGController && OwningARPGController->HasEquippedBoots())
	{
		OwningARPGController->UnequipSlot(EARPGEquipmentSlot::Boots);
		RefreshEquipmentPanel();
	}
}
