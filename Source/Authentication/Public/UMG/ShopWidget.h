// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OwnerInventory.h"
#include "ShopWidget.generated.h"

class UVerticalBox;
class UTextBlock;

UCLASS()
class AUTHENTICATION_API UShopWidget : public UOwnerInventory
{
	GENERATED_BODY()

public:
	virtual void OnActivate_Implementation() override;
	virtual void OnDeactivate_Implementation() override;

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UVerticalBox> ShopItemsBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UVerticalBox> CurrentCartBox;
};
