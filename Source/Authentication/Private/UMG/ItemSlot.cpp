// Fill out your copyright notice in the Description page of Project Settings.


#include "UMG/ItemSlot.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

void UItemSlot::InitializeWidget(const FString& InItemName, int32 InQuantity, TObjectPtr<UTexture2D> InIcon) const
{
	ItemNameText->SetText(FText::FromString(InItemName));
	QuantityText->SetText(FText::AsNumber(InQuantity));
	IconImage->SetBrushFromTexture(InIcon);
}

void UItemSlot::SetQuantity(int32 InQuantity) const
{
	QuantityText->SetText(FText::AsNumber(InQuantity));
}

FText UItemSlot::GetQuantityText() const
{
	return QuantityText->GetText();
}
