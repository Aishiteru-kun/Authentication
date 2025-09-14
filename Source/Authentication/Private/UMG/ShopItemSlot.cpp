// Fill out your copyright notice in the Description page of Project Settings.


#include "UMG/ShopItemSlot.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UShopItemSlot::SetGoldPrice(int32 InGoldPrice)
{
	GoldPrice = InGoldPrice;

	if (bShowOnlyPrice)
	{
		GoldPriceText->SetText(FText::AsNumber(GoldPrice));
	}
}

void UShopItemSlot::SetQuantity(int32 InQuantity) const
{
	Super::SetQuantity(InQuantity);

	if (bShowOnlyPrice)
	{
		QuantityText->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	QuantityText->SetVisibility(ESlateVisibility::Visible);

	GoldPriceText->SetText(FText::AsNumber(InQuantity * GoldPrice));
}

void UShopItemSlot::SetOnlyPriceMode(bool bInShowOnlyPrice)
{
	bShowOnlyPrice = bInShowOnlyPrice;

	QuantityText->SetVisibility(bShowOnlyPrice ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

FText UShopItemSlot::GetItemName() const
{
	return ItemNameText->GetText();
}

FText UShopItemSlot::GetQuantity() const
{
	return QuantityText->GetText();
}

FSlateBrush UShopItemSlot::GetIconBrush() const
{
	return IconImage->GetBrush();
}

void UShopItemSlot::NativeConstruct()
{
	Super::NativeConstruct();

	if (SelectedWidget)
	{
		SelectedWidget->OnClicked.AddUniqueDynamic(this, &ThisClass::OnClicked);
	}
}

void UShopItemSlot::OnClicked()
{
	OnSelectedItemChanged.Broadcast(this);
}
