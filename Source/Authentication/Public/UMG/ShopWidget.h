// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OwnerInventory.h"
#include "ShopWidget.generated.h"

class UButton;
class UScrollBox;
class UShopItemSlot;
class UTextBlock;

UCLASS()
class AUTHENTICATION_API UShopWidget : public UOwnerInventory
{
	GENERATED_BODY()

public:
	virtual void OnActivate_Implementation() override;
	virtual void OnDeactivate_Implementation() override;

protected:
	virtual void NativeConstruct() override;

protected:
	UFUNCTION()
	void OnItemPrice(bool bOK, int32 UnitPrice, int32 TotalPrice, FString ItemId, FString ErrorText);

	UFUNCTION()
	void OnItemPurchaseStatus(bool bOK, FString RequestId, FString ErrorText);

	UFUNCTION()
	void OnPurchaseStatus(bool bOK, FString Status, FString HumanInfo, FString ErrorText);

	UFUNCTION()
	void OnAddToCardClicked(UShopItemSlot* ItemSlot);

	UFUNCTION()
	void OnSelectedCartItem(UShopItemSlot* ItemSlot);

	UFUNCTION()
	void OnPlusOneClicked();

	UFUNCTION()
	void OnSubtractOneClicked();

	UFUNCTION()
	void OnRemoveItemClicked();

	UFUNCTION()
	void OnPurchaseClicked();

	UFUNCTION()
	void OnCancelClicked();

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UScrollBox> ShopItemsBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UScrollBox> CurrentCartBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> PlusOneButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> SubtractOneButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> RemoveItemButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> PurchaseButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> CancelButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UShopItemSlot> CurrentSelectedItem;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	TSubclassOf<UItemSlot> ItemShopSlotWidgetClass;

	TWeakObjectPtr<UShopItemSlot> WeakSelectedCartItem;
};
