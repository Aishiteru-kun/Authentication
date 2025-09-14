// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UMG/ItemSlot.h"
#include "ShopItemSlot.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSelectedItemChanged, UShopItemSlot*, SelectedItem);

UCLASS()
class AUTHENTICATION_API UShopItemSlot : public UItemSlot
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnSelectedItemChanged OnSelectedItemChanged;

public:
	void SetGoldPrice(int32 InGoldPrice);
	virtual void SetQuantity(int32 InQuantity) const override;

	void SetOnlyPriceMode(bool bInShowOnlyPrice);

	FText GetItemName() const;
	FText GetQuantity() const;
	int32 GetGoldPrice() const { return GoldPrice; }

	FSlateBrush GetIconBrush() const;

protected:
	virtual void NativeConstruct() override;

protected:
	UFUNCTION()
	void OnClicked();

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> GoldPriceText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> SelectedWidget;

private:
	int32 GoldPrice = 0;

	bool bShowOnlyPrice = false;
};
