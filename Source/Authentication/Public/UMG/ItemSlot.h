// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemSlot.generated.h"

class UImage;
class UTextBlock;

UCLASS()
class AUTHENTICATION_API UItemSlot : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeWidget(const FString& InItemName, int32 InQuantity, TObjectPtr<UTexture2D> InIcon) const;

	virtual void SetQuantity(int32 InQuantity) const;
	FText GetQuantityText() const;

	bool IsEqual(const UItemSlot* Other) const;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> QuantityText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IconImage;
};
