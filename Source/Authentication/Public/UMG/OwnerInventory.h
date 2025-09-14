// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interface/WidgetInterface.h"
#include "OwnerInventory.generated.h"

class UTextBlock;
class UItemSlot;
class UVerticalBox;

UCLASS()
class AUTHENTICATION_API UOwnerInventory : public UUserWidget, public IWidgetInterface
{
	GENERATED_BODY()

public:
	virtual void OnActivate_Implementation() override;
	virtual void OnDeactivate_Implementation() override;

protected:
	UFUNCTION()
	void OnInventory(bool bOK, int64 Gold, TArray<FServerInventoryItem> Items, FString Err);

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UVerticalBox> ItemsBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> GoldAmount;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ErrorMsg;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TObjectPtr<UDataTable> ItemDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TSubclassOf<UItemSlot> ItemSlotWidgetClass;
};
