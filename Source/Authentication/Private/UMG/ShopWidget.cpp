// Fill out your copyright notice in the Description page of Project Settings.


#include "UMG/ShopWidget.h"

#include "GlobalStructs.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Player/MyPlayerState.h"
#include "Subsystem/AuthApiClientSubsystem.h"
#include "UMG/ItemSlot.h"
#include "UMG/ShopItemSlot.h"

class UAuthApiClientSubsystem;

void UShopWidget::OnActivate_Implementation()
{
	Super::OnActivate_Implementation();

	if (!ItemDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemDataTable is not set in %s"), *GetName());
		return;
	}

	if (!ItemShopSlotWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemShopSlotWidgetClass is not set in %s"), *GetName());
		return;
	}

	TArray<FServerItemInfo*> Items;
	ItemDataTable->GetAllRows<FServerItemInfo>(TEXT(""), Items);

	UAuthApiClientSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UAuthApiClientSubsystem>();

	for (FServerItemInfo* const& Item : Items)
	{
		Subsystem->GetItemPrice(Item->ItemId, 1, FOnPrice::CreateUObject(this, &ThisClass::OnItemPrice));
	}
}

void UShopWidget::OnDeactivate_Implementation()
{
	Super::OnDeactivate_Implementation();

	ShopItemsBox->ClearChildren();
	CurrentCartBox->ClearChildren();
}

void UShopWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PlusOneButton)
	{
		PlusOneButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnPlusOneClicked);
	}

	if (SubtractOneButton)
	{
		SubtractOneButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnSubtractOneClicked);
	}

	if (RemoveItemButton)
	{
		RemoveItemButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnRemoveItemClicked);
	}

	if (PurchaseButton)
	{
		PurchaseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnPurchaseClicked);
	}

	if (CancelButton)
	{
		CancelButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnCancelClicked);
	}
}

void UShopWidget::OnItemPrice(bool bOK, int32 UnitPrice, int32 TotalPrice, FString ItemId, FString ErrorText)
{
	if (!bOK)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnItemPrice is not set"));

		ErrorMsg->SetText(FText::FromString(ErrorText));
		ErrorMsg->SetVisibility(ESlateVisibility::Visible);

		return;
	}

	const FServerItemInfo* Found = ItemDataTable->FindRow<FServerItemInfo>(*ItemId, TEXT(""));
	if (!Found)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemId %s not found in ItemDataTable"), *ItemId);
		return;
	}

	UItemSlot* ItemSlot = CreateWidget<UItemSlot>(this, ItemShopSlotWidgetClass);
	UShopItemSlot* ShopItemSlot = Cast<UShopItemSlot>(ItemSlot);
	if (!ShopItemSlot)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to create ItemSlot widget"));
		return;
	}

	ShopItemSlot->OnSelectedItemChanged.AddUniqueDynamic(this, &ThisClass::OnAddToCardClicked);

	ShopItemSlot->InitializeWidget(Found->Name, 1, Found->Icon);
	ShopItemSlot->SetOnlyPriceMode(true);
	ShopItemSlot->SetGoldPrice(UnitPrice);

	ShopItemsBox->AddChild(ShopItemSlot);
}

void UShopWidget::OnItemPurchaseStatus(bool bOK, FString RequestId, FString ErrorText)
{
	if (!bOK)
	{
		ErrorMsg->SetText(FText::FromString(ErrorText));

		UE_LOG(LogTemp, Warning, TEXT("OnItemPurchaseStatus is not set"));
		return;
	}

	UAuthApiClientSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UAuthApiClientSubsystem>();

	Subsystem->PollPurchaseStatus(RequestId, FOnPurchaseStatus::CreateUObject(this, &ThisClass::OnPurchaseStatus), 65.0f);
}

void UShopWidget::OnPurchaseStatus(bool bOK, FString Status, FString HumanInfo, FString ErrorText)
{
	if (!bOK)
	{
		ErrorMsg->SetText(FText::FromString(ErrorText));

		UE_LOG(LogTemp, Warning, TEXT("OnPurchaseStatus is not set"));
		return;
	}

	if (Status == TEXT("approved"))
	{
		OnDeactivate_Implementation();
		OnActivate_Implementation();
	}
}

void UShopWidget::OnAddToCardClicked(UShopItemSlot* ItemSlot)
{
	for (UWidget* ThisWidget : CurrentCartBox->GetAllChildren())
	{
		const UItemSlot* ThisItemSlot = Cast<UItemSlot>(ThisWidget);
		if (!ThisItemSlot)
		{
			continue;
		}

		if (ItemSlot->IsEqual(ThisItemSlot))
		{
			return;
		}
	}

	UItemSlot* ThisItemSlot = CreateWidget<UItemSlot>(this, ItemShopSlotWidgetClass);
	UShopItemSlot* ShopItemSlot = Cast<UShopItemSlot>(ThisItemSlot);
	if (!ShopItemSlot)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to create ItemSlot widget"));
		return;
	}

	ShopItemSlot->InitializeWidget(ItemSlot->GetItemName().ToString(), 0, Cast<UTexture2D>(ItemSlot->GetIconBrush().GetResourceObject()));
	ShopItemSlot->SetOnlyPriceMode(false);
	ShopItemSlot->SetGoldPrice(ItemSlot->GetGoldPrice());
	ShopItemSlot->SetQuantity(1);

	ShopItemSlot->OnSelectedItemChanged.AddUniqueDynamic(this, &ThisClass::OnSelectedCartItem);

	CurrentCartBox->AddChild(ShopItemSlot);
}

void UShopWidget::OnSelectedCartItem(UShopItemSlot* ItemSlot)
{
	for (UWidget* ThisWidget : CurrentCartBox->GetAllChildren())
	{
		if (ThisWidget == ItemSlot)
		{
			continue;
		}

		ThisWidget->SetIsEnabled(true);
	}

	ItemSlot->SetIsEnabled(false);

	WeakSelectedCartItem = ItemSlot;

	int32 Quantity;
	LexTryParseString(Quantity, *ItemSlot->GetQuantity().ToString());

	CurrentSelectedItem->InitializeWidget(ItemSlot->GetItemName().ToString(), 0, Cast<UTexture2D>(ItemSlot->GetIconBrush().GetResourceObject()));
	CurrentSelectedItem->SetGoldPrice(ItemSlot->GetGoldPrice());
	CurrentSelectedItem->SetOnlyPriceMode(false);
	CurrentSelectedItem->SetQuantity(Quantity);

	CurrentSelectedItem->SetVisibility(ESlateVisibility::Visible);
}

void UShopWidget::OnPlusOneClicked()
{
	if (!WeakSelectedCartItem.IsValid())
	{
		return;
	}

	int32 Quantity;
	LexTryParseString(Quantity, *WeakSelectedCartItem->GetQuantity().ToString());

	WeakSelectedCartItem->SetQuantity(Quantity + 1);
	CurrentSelectedItem->SetQuantity(Quantity + 1);
}

void UShopWidget::OnSubtractOneClicked()
{
	if (!WeakSelectedCartItem.IsValid())
	{
		return;
	}

	int32 Quantity;
	LexTryParseString(Quantity, *WeakSelectedCartItem->GetQuantity().ToString());

	if (Quantity == 1)
	{
		return;
	}

	WeakSelectedCartItem->SetQuantity(Quantity - 1);
	CurrentSelectedItem->SetQuantity(Quantity - 1);
}

void UShopWidget::OnRemoveItemClicked()
{
	CurrentSelectedItem->SetVisibility(ESlateVisibility::Hidden);
	CurrentCartBox->RemoveChild(WeakSelectedCartItem.Get());
	WeakSelectedCartItem.Reset();
}

void UShopWidget::OnPurchaseClicked()
{
	const AMyPlayerState* PS = GetOwningPlayerState<AMyPlayerState>();

	UAuthApiClientSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UAuthApiClientSubsystem>();

	for (UWidget* ThisWidget : CurrentCartBox->GetAllChildren())
	{
		const UItemSlot* ThisItemSlot = Cast<UItemSlot>(ThisWidget);
		if (!ThisItemSlot)
		{
			continue;
		}

		const UShopItemSlot* ShopItemSlot = Cast<UShopItemSlot>(ThisItemSlot);
		if (!ShopItemSlot)
		{
			continue;
		}

		int32 Quantity;
		LexTryParseString(Quantity, *ShopItemSlot->GetQuantity().ToString());

		TArray<FServerItemInfo*> Items;
		ItemDataTable->GetAllRows<FServerItemInfo>(TEXT(""), Items);

		FServerItemInfo* const * FoundItem = Items.FindByPredicate(
			[ItemName = ShopItemSlot->GetItemName().ToString()](const FServerItemInfo* Item) -> bool
		{
			return Item->Name == ItemName;
		});

		if (!FoundItem)
		{
			continue;
		}

		Subsystem->PurchaseRequest(PS->GetSessionToken(), (*FoundItem)->ItemId, Quantity, FOnPurchaseRequested::CreateUObject(this, &ThisClass::OnItemPurchaseStatus));
	}
}

void UShopWidget::OnCancelClicked()
{
	CurrentSelectedItem->SetVisibility(ESlateVisibility::Hidden);
	CurrentCartBox->ClearChildren();
	WeakSelectedCartItem.Reset();
}
