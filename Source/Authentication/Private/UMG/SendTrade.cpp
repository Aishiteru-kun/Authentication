#include "UMG/SendTrade.h"

#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Player/MyPlayerState.h"
#include "Subsystem/AuthApiClientSubsystem.h"
#include "UMG/ItemSlot.h"
#include "UMG/ShopItemSlot.h"

// ==================== ЖИТТЄВИЙ ЦИКЛ ====================

void USendTrade::NativeConstruct()
{
	Super::NativeConstruct();

	if (OfferPlusOneButton)   OfferPlusOneButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnOfferPlusOneClicked);
	if (OfferMinusOneButton)  OfferMinusOneButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnOfferMinusOneClicked);
	if (OfferRemoveButton)    OfferRemoveButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnOfferRemoveClicked);

	if (RequestPlusOneButton)  RequestPlusOneButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnRequestPlusOneClicked);
	if (RequestMinusOneButton) RequestMinusOneButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnRequestMinusOneClicked);
	if (RequestRemoveButton)   RequestRemoveButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnRequestRemoveClicked);

	if (SendTradeButton)   SendTradeButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnSendTradeClicked);
	if (CancelButton)      CancelButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnCancelClicked);
}

void USendTrade::OnActivate_Implementation()
{
	ClearAllLists();

	// Отримуємо наші логін/токен з PlayerState
	const AMyPlayerState* PS = GetOwningPlayerState<AMyPlayerState>();
	if (PS)
	{
		// !!! якщо в тебе інші назви — заміни тут:
		MySessionToken = PS->GetSessionToken();
		MyLogin       =  FString::Printf(TEXT("%s_%s"), *PS->GetPlayerNameCustom(), *PS->GetPlayerChatId());   // <- має повертати логін, що використовувався при логіні
	}

	if (MyLogin.IsEmpty())
	{
		ShowError(TEXT("Your login is unknown."));
		return;
	}

	// тягнемо обидва інвентарі
	LoadMyInventory();
	LoadTargetInventoryAndChatId();
}

void USendTrade::OnDeactivate_Implementation()
{
	ClearAllLists();

	TargetLogin.Reset();
	TargetChatId = 0;
	MyInventoryCounts.Empty();
	PersonInventoryCounts.Empty();
	OfferMap.Empty();
	RequestMap.Empty();
	WeakSelectedOfferItem.Reset();
	WeakSelectedRequestItem.Reset();
}

void USendTrade::OnSelectedPerson(const FString& PersonLogin)
{
	TargetLogin = PersonLogin;
	// Якщо віджет уже активний — одразу підтягнемо інвентар
	if (IsInViewport() || IsVisible())
	{
		LoadTargetInventoryAndChatId();
	}
}

// ==================== ПОБУДОВА UI ====================

void USendTrade::ClearAllLists()
{
	if (YourInventoryPanel) YourInventoryPanel->ClearChildren();
	if (PersonInventoryPanel) PersonInventoryPanel->ClearChildren();
	if (OfferBox) OfferBox->ClearChildren();
	if (RequestBox) RequestBox->ClearChildren();

	OfferMap.Empty();
	RequestMap.Empty();
}

void USendTrade::RefreshUI()
{
	if (OfferBox)
	{
		for (UWidget* W : OfferBox->GetAllChildren())
		{
			if (UShopItemSlot* S = Cast<UShopItemSlot>(W))
			{
				const FString Name = S->GetItemName().ToString();
				if (const int32* Qty = OfferMap.Find(Name))
				{
					S->SetQuantity(*Qty);
				}
			}
		}
	}
	if (RequestBox)
	{
		for (UWidget* W : RequestBox->GetAllChildren())
		{
			if (UShopItemSlot* S = Cast<UShopItemSlot>(W))
			{
				const FString Name = S->GetItemName().ToString();
				if (const int32* Qty = RequestMap.Find(Name))
				{
					S->SetQuantity(*Qty);
				}
			}
		}
	}
}

void USendTrade::BuildYourInventoryUI()
{
	if (!YourInventoryPanel || !InventoryItemSlotWidgetClass) return;

	YourInventoryPanel->ClearChildren();

	for (const auto& Kvp : MyInventoryCounts)
	{
		const FString& Name = Kvp.Key;
		const int32 Qty = Kvp.Value;

		UShopItemSlot* ThisSlot = CreateWidget<UShopItemSlot>(this, InventoryItemSlotWidgetClass);
		if (!ThisSlot) continue;

		const FServerItemInfo* Row = FindRowByName(Name);
		UTexture2D* Icon = Row ? Row->Icon : nullptr;

		ThisSlot->InitializeWidget(Name, Qty, Icon);
		ThisSlot->SetOnlyPriceMode(false);
		ThisSlot->OnSelectedItemChanged.AddUniqueDynamic(this, &ThisClass::OnYourInventoryItemSelected);

		YourInventoryPanel->AddChild(ThisSlot);
	}
}

void USendTrade::BuildPersonInventoryUI(const TArray<FServerInventoryItem>& Items)
{
	if (!PersonInventoryPanel || !InventoryItemSlotWidgetClass) return;

	PersonInventoryPanel->ClearChildren();
	PersonInventoryCounts.Empty();

	for (const FServerInventoryItem& It : Items)
	{
		PersonInventoryCounts.Add(It.Name, It.Quantity);

		UShopItemSlot* ThisSlot = CreateWidget<UShopItemSlot>(this, InventoryItemSlotWidgetClass);
		if (!ThisSlot) continue;

		const FServerItemInfo* Row = FindRowByName(It.Name);
		UTexture2D* Icon = Row ? Row->Icon : nullptr;

		ThisSlot->InitializeWidget(It.Name, It.Quantity, Icon);
		ThisSlot->SetOnlyPriceMode(false);
		ThisSlot->OnSelectedItemChanged.AddUniqueDynamic(this, &ThisClass::OnPersonInventoryItemSelected);

		PersonInventoryPanel->AddChild(ThisSlot);
	}
}

// ==================== ІНТЕРАКЦІЯ: ДОДАВАННЯ У OFFER/REQUEST ====================

void USendTrade::OnYourInventoryItemSelected(UShopItemSlot* ItemSlot)
{
	if (!ItemSlot) return;
	const FString Name = ItemSlot->GetItemName().ToString();

	// не дозволяємо виставити більше, ніж є у нас
	const int32* Have = MyInventoryCounts.Find(Name);
	const int32 Curr = OfferMap.FindRef(Name);
	if (Have && Curr >= *Have) { return; }

	AddOrIncOfferByName(Name);
}

void USendTrade::OnPersonInventoryItemSelected(UShopItemSlot* ItemSlot)
{
	if (!ItemSlot) return;
	const FString Name = ItemSlot->GetItemName().ToString();

	// не просимо більше, ніж у них є (не обов'язково, але зручно для UX)
	const int32* Have = PersonInventoryCounts.Find(Name);
	const int32 Curr = RequestMap.FindRef(Name);
	if (Have && Curr >= *Have) { return; }

	AddOrIncRequestByName(Name);
}

void USendTrade::AddOrIncOfferByName(const FString& ItemName)
{
	int32& Qty = OfferMap.FindOrAdd(ItemName);
	Qty = FMath::Max(0, Qty + 1);

	UShopItemSlot* Existing = FindItemSlotInBox(OfferBox, ItemName);
	if (Existing)
	{
		Existing->SetQuantity(Qty);
	}
	else
	{
		AddItemToBox(OfferBox, ItemName, Qty, true);
	}
}

void USendTrade::AddOrIncRequestByName(const FString& ItemName)
{
	int32& Qty = RequestMap.FindOrAdd(ItemName);
	Qty = FMath::Max(0, Qty + 1);

	UShopItemSlot* Existing = FindItemSlotInBox(RequestBox, ItemName);
	if (Existing)
	{
		Existing->SetQuantity(Qty);
	}
	else
	{
		AddItemToBox(RequestBox, ItemName, Qty, false);
	}
}

UShopItemSlot* USendTrade::FindItemSlotInBox(UScrollBox* Box, const FString& ItemName) const
{
	if (!Box) return nullptr;
	for (UWidget* W : Box->GetAllChildren())
	{
		if (UShopItemSlot* S = Cast<UShopItemSlot>(W))
		{
			if (S->GetItemName().ToString() == ItemName) return S;
		}
	}
	return nullptr;
}

UShopItemSlot* USendTrade::AddItemToBox(UScrollBox* Box, const FString& ItemName, int32 StartQty, bool bBindToOffer)
{
	if (!Box || !InventoryItemSlotWidgetClass) return nullptr;

	UShopItemSlot* ThisSlot = CreateWidget<UShopItemSlot>(this, InventoryItemSlotWidgetClass);
	if (!ThisSlot) return nullptr;

	const FServerItemInfo* Row = FindRowByName(ItemName);
	UTexture2D* Icon = Row ? Row->Icon : nullptr;

	ThisSlot->InitializeWidget(ItemName, StartQty, Icon);
	ThisSlot->SetOnlyPriceMode(false);
	ThisSlot->ShowPrice(false);

	if (bBindToOffer)
	{
		ThisSlot->OnSelectedItemChanged.AddUniqueDynamic(this, &ThisClass::OnSelectedOfferItem);
	}
	else
	{
		ThisSlot->OnSelectedItemChanged.AddUniqueDynamic(this, &ThisClass::OnSelectedRequestItem);
	}

	Box->AddChild(ThisSlot);
	return ThisSlot;
}

// ==================== ВИБІР ПОЗИЦІЇ ДЛЯ +/–/REMOVE ====================

void USendTrade::OnSelectedOfferItem(UShopItemSlot* ItemSlot)
{
	if (!OfferBox || !ItemSlot) return;

	// зробимо disable тільки вибраний
	for (UWidget* W : OfferBox->GetAllChildren())
	{
		if (W == ItemSlot) continue;
		W->SetIsEnabled(true);
	}
	ItemSlot->SetIsEnabled(false);
	WeakSelectedOfferItem = ItemSlot;
}

void USendTrade::OnSelectedRequestItem(UShopItemSlot* ItemSlot)
{
	if (!RequestBox || !ItemSlot) return;

	for (UWidget* W : RequestBox->GetAllChildren())
	{
		if (W == ItemSlot) continue;
		W->SetIsEnabled(true);
	}
	ItemSlot->SetIsEnabled(false);
	WeakSelectedRequestItem = ItemSlot;
}

void USendTrade::ChangeQtyInBox(UScrollBox* Box, const FString& ItemName, int32 NewQty)
{
	if (!Box) return;
	if (NewQty <= 0)
	{
		// видалення
		for (int32 i = Box->GetChildrenCount() - 1; i >= 0; --i)
		{
			if (UShopItemSlot* S = Cast<UShopItemSlot>(Box->GetChildAt(i)))
			{
				if (S->GetItemName().ToString() == ItemName)
				{
					Box->RemoveChildAt(i);
				}
			}
		}
		return;
	}
	if (UShopItemSlot* S = FindItemSlotInBox(Box, ItemName))
	{
		S->SetQuantity(NewQty);
	}
}

// ==================== КНОПКИ OFFER ====================

void USendTrade::OnOfferPlusOneClicked()
{
	if (!WeakSelectedOfferItem.IsValid()) return;
	const FString Name = WeakSelectedOfferItem->GetItemName().ToString();

	// не більше, ніж маємо
	const int32* Have = MyInventoryCounts.Find(Name);
	int32& Qty = OfferMap.FindOrAdd(Name);
	if (Have && Qty >= *Have) return;

	Qty++;
	WeakSelectedOfferItem->SetQuantity(Qty);
}

void USendTrade::OnOfferMinusOneClicked()
{
	if (!WeakSelectedOfferItem.IsValid()) return;
	const FString Name = WeakSelectedOfferItem->GetItemName().ToString();
	int32& Qty = OfferMap.FindOrAdd(Name);
	if (Qty <= 1) return;
	Qty--;
	WeakSelectedOfferItem->SetQuantity(Qty);
}

void USendTrade::OnOfferRemoveClicked()
{
	if (!WeakSelectedOfferItem.IsValid()) return;
	const FString Name = WeakSelectedOfferItem->GetItemName().ToString();
	OfferMap.Remove(Name);
	ChangeQtyInBox(OfferBox, Name, 0);
	WeakSelectedOfferItem.Reset();
}

// ==================== КНОПКИ REQUEST ====================

void USendTrade::OnRequestPlusOneClicked()
{
	if (!WeakSelectedRequestItem.IsValid()) return;
	const FString Name = WeakSelectedRequestItem->GetItemName().ToString();

	const int32* Have = PersonInventoryCounts.Find(Name);
	int32& Qty = RequestMap.FindOrAdd(Name);
	if (Have && Qty >= *Have) return;

	Qty++;
	WeakSelectedRequestItem->SetQuantity(Qty);
}

void USendTrade::OnRequestMinusOneClicked()
{
	if (!WeakSelectedRequestItem.IsValid()) return;
	const FString Name = WeakSelectedRequestItem->GetItemName().ToString();
	int32& Qty = RequestMap.FindOrAdd(Name);
	if (Qty <= 1) return;
	Qty--;
	WeakSelectedRequestItem->SetQuantity(Qty);
}

void USendTrade::OnRequestRemoveClicked()
{
	if (!WeakSelectedRequestItem.IsValid()) return;
	const FString Name = WeakSelectedRequestItem->GetItemName().ToString();
	RequestMap.Remove(Name);
	ChangeQtyInBox(RequestBox, Name, 0);
	WeakSelectedRequestItem.Reset();
}

// ==================== ВІДПРАВКА ТРЕЙДУ ====================

void USendTrade::BuildTradeArrays(TArray<FString>& OutOfferIds, TArray<FString>& OutRequestIds) const
{
	auto Push = [this](TArray<FString>& Arr, const FString& Name, int32 Qty)
	{
		const FServerItemInfo* Row = FindRowByName(Name);
		const FString ItemId = Row ? Row->ItemId : Name; // фолбек — ім'я як id
		for (int32 i=0; i<Qty; ++i) Arr.Add(ItemId);
	};

	for (const auto& K : OfferMap)
	{
		if (K.Value > 0) Push(OutOfferIds, K.Key, K.Value);
	}
	for (const auto& K : RequestMap)
	{
		if (K.Value > 0) Push(OutRequestIds, K.Key, K.Value);
	}
}

void USendTrade::OnSendTradeClicked()
{
	if (TargetChatId <= 0)
	{
		ShowError(TEXT("Target user is unknown."));
		return;
	}
	if (OfferMap.Num()==0 && RequestMap.Num()==0)
	{
		ShowError(TEXT("Trade is empty."));
		return;
	}
	if (MySessionToken.IsEmpty())
	{
		ShowError(TEXT("No session token."));
		return;
	}

	TArray<FString> OfferIds, RequestIds;
	BuildTradeArrays(OfferIds, RequestIds);

	UAuthApiClientSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UAuthApiClientSubsystem>();
	if (!Subsystem)
	{
		ShowError(TEXT("Subsystem not found."));
		return;
	}

	Subsystem->TradeCreate(
		MySessionToken, TargetChatId,
		OfferIds, RequestIds,
		FOnTradeCreated::CreateWeakLambda(this, [this, Subsystem](bool bOK, FString TradeId, FString ErrorText)
		{
			if (!bOK)
			{
				ShowError(FString::Printf(TEXT("Trade create failed: %s"), *ErrorText));
				return;
			}

			// Чекаємо рішення у Телеграмі
			Subsystem->PollTradeStatus(
				TradeId,
				FOnTradeStatus::CreateWeakLambda(this, [this](bool bApproved, FString Status, FString ErrorText)
				{
					if (!bApproved)
					{
						ShowError(Status.IsEmpty() ? TEXT("Trade declined or timeout.") : Status);
						return;
					}
					// успіх: очистимо вибір і оновимо інвентарі
					ClearAllLists();
					LoadMyInventory();
					LoadTargetInventoryAndChatId();
				}),
				90.f
			);
		})
	);
}

void USendTrade::OnCancelClicked()
{
	OfferMap.Empty();
	RequestMap.Empty();
	if (OfferBox) OfferBox->ClearChildren();
	if (RequestBox) RequestBox->ClearChildren();
	WeakSelectedOfferItem.Reset();
	WeakSelectedRequestItem.Reset();
	ShowError(TEXT("")); // прибрати помилку, якщо була
}

// ==================== ДОПОМОЖНІ ====================

const FServerItemInfo* USendTrade::FindRowByName(const FString& Name) const
{
	if (!ItemDataTable) return nullptr;
	static const FString Ctx(TEXT("Trade_FindByName"));
	TArray<FServerItemInfo*> Rows;
	ItemDataTable->GetAllRows<FServerItemInfo>(Ctx, Rows); // (хитрість не зайде) — зробимо звичайно:

	Rows.Reset();
	ItemDataTable->GetAllRows<FServerItemInfo>(Ctx, Rows);
	for (const FServerItemInfo* R : Rows)
	{
		if (R && R->Name == Name) return R;
	}
	return nullptr;
}

const FServerItemInfo* USendTrade::FindRowById(const FString& ItemId) const
{
	if (!ItemDataTable) return nullptr;
	static const FString Ctx(TEXT("Trade_FindById"));
	TArray<FServerItemInfo*> Rows;
	ItemDataTable->GetAllRows<FServerItemInfo>(Ctx, Rows);
	for (const FServerItemInfo* R : Rows)
	{
		if (R && R->ItemId == ItemId) return R;
	}
	return nullptr;
}

void USendTrade::ShowError(const FString& Text)
{
	if (ErrorMsg)
	{
		ErrorMsg->SetText(FText::FromString(Text));
		ErrorMsg->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
	}
}

// ----------- Завантаження інвентарів -----------

void USendTrade::LoadMyInventory()
{
	UAuthApiClientSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UAuthApiClientSubsystem>();
	if (!Subsystem) { ShowError(TEXT("Subsystem not found.")); return; }

	Subsystem->GetInventoryByLogin(
		MyLogin,
		[this](bool bOK, int64 Gold, const TArray<FServerInventoryItem>& Items, const FString& Err)
		{
			if (!bOK)
			{
				ShowError(FString::Printf(TEXT("Failed to load your inventory: %s"), *Err));
				return;
			}
			MyInventoryCounts.Empty();
			for (const FServerInventoryItem& It : Items)
			{
				MyInventoryCounts.Add(It.Name, It.Quantity);
			}
			BuildYourInventoryUI();
		}
	);
}

void USendTrade::LoadTargetInventoryAndChatId()
{
	if (TargetLogin.IsEmpty()) return;

	UAuthApiClientSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UAuthApiClientSubsystem>();
	if (!Subsystem) { ShowError(TEXT("Subsystem not found.")); return; }

	Subsystem->GetUserByLogin(TargetLogin,
		[this, Subsystem](bool bOK, int64 ChatId, const FString& Err)
		{
			if (!bOK)
			{
				ShowError(FString::Printf(TEXT("User not found: %s"), *Err));
				return;
			}
			TargetChatId = ChatId;

			Subsystem->GetInventoryByChatId(ChatId,
				[this](bool bOK2, int64 Gold, const TArray<FServerInventoryItem>& Items, const FString& Err2)
				{
					if (!bOK2)
					{
						ShowError(FString::Printf(TEXT("Failed to load target inventory: %s"), *Err2));
						return;
					}
					BuildPersonInventoryUI(Items);
				}
			);
		}
	);
}
