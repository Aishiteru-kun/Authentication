// Fill out your copyright notice in the Description page of Project Settings.


#include "UMG/OwnerInventory.h"

#include "GlobalStructs.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Player/MyPlayerState.h"
#include "Subsystem/AuthApiClientSubsystem.h"
#include "UMG/ItemSlot.h"

void UOwnerInventory::OnActivate_Implementation()
{
	ErrorMsg->SetVisibility(ESlateVisibility::Hidden);

	const AMyPlayerState* PS = GetOwningPlayerState<AMyPlayerState>();

	const FString PlayerName = PS->GetPlayerNameCustom();
	const FString PlayerChatId = PS->GetPlayerChatId();
	const FString FullId = FString::Printf(TEXT("%s_%s"), *PlayerName, *PlayerChatId);

	UAuthApiClientSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UAuthApiClientSubsystem>();

	Subsystem->GetInventoryByLogin(FullId, [this](bool bOK, int64 Gold, TArray<FServerInventoryItem> Items, FString Err) {
		OnInventory(bOK, Gold, Items, Err);
	});
}

void UOwnerInventory::OnDeactivate_Implementation()
{
	ItemsBox->ClearChildren();
}

void UOwnerInventory::OnInventory(bool bOK, int64 Gold, TArray<FServerInventoryItem> Items, FString Err)
{
	if (!bOK)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnInventory failed: %s"), *Err);

		if (ErrorMsg)
		{
			ErrorMsg->SetVisibility(ESlateVisibility::Visible);
			ErrorMsg->SetText(FText::FromString(Err));
		}

		return;
	}

	if (GoldAmount)
	{
		const FString GoldStr = FString::Printf(TEXT("%lld"), Gold);
		const FString ZeroStr = FString(5 - GoldStr.Len(), "0");
		const FString CurrentGold = FString::Printf(TEXT("%s%s"), *ZeroStr, *GoldStr);

		GoldAmount->SetText(FText::FromString(CurrentGold));
	}

	if (!ItemDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemDataTable is not set"));
		return;
	}

	if (!ItemSlotWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemSlotWidgetClass is not set"));
		return;
	}

	for (const auto& [Id, Name, Quantity] : Items)
	{
		const FServerItemInfo* Found = ItemDataTable->FindRow<FServerItemInfo>(*Id, TEXT(""));
		if (!Found) continue;

		UItemSlot* ThisWidgetSlot = CreateWidget<UItemSlot>(this, ItemSlotWidgetClass);
		if (!ThisWidgetSlot) continue;

		ThisWidgetSlot->InitializeWidget(Name, Quantity, Found->Icon);

		ItemsBox->AddChild(ThisWidgetSlot);
	}
}
