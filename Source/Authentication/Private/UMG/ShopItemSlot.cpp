// Fill out your copyright notice in the Description page of Project Settings.


#include "UMG/ShopItemSlot.h"

#include "Components/Button.h"
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
		return;
	}

	GoldPriceText->SetText(FText::AsNumber(InQuantity * GoldPrice));
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
	SetIsEnabled(false);

	OnSelectedItemChanged.Broadcast(this);
}
