// Fill out your copyright notice in the Description page of Project Settings.


#include "UMG/SelectPerson.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "GameFramework/GameStateBase.h"
#include "Player/MyPlayerState.h"
#include "UMG/PersonSlot.h"

void USelectPerson::OnActivate_Implementation()
{
	IWidgetInterface::OnActivate_Implementation();

	if (!PersonSlotWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("PersonSlotWidgetClass is not set in %s"), *GetName());
		return;
	}

	for (const TObjectPtr<APlayerState>& PlayerState : GetWorld()->GetGameState()->PlayerArray)
	{
		AMyPlayerState* ThisPlayerState = Cast<AMyPlayerState>(PlayerState);
		if (!ThisPlayerState)
		{
			continue;
		}

		UPersonSlot* NewPersonSlot = CreateWidget<UPersonSlot>(GetWorld(), PersonSlotWidgetClass);
		if (!NewPersonSlot)
		{
			continue;
		}

		NewPersonSlot->InitializeWidget(FString::Printf(TEXT("%s_%s"), *ThisPlayerState->GetPlayerName(),
		                                                *ThisPlayerState->GetPlayerChatId()));
		NewPersonSlot->OnSelectedPerson.BindLambda([this](const FString& PersonName)
		{
			SelectedPerson = PersonName;
		});

		Panel->AddChild(NewPersonSlot);
	}
}

void USelectPerson::OnDeactivate_Implementation()
{
	IWidgetInterface::OnDeactivate_Implementation();

	Panel->ClearChildren();
	SelectedPerson.Empty();
}

void USelectPerson::NativeConstruct()
{
	Super::NativeConstruct();

	if (TradeButton)
	{
		TradeButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnTradeClicked);
	}
}

void USelectPerson::OnTradeClicked()
{
	if (SelectedPerson.IsEmpty())
	{
		return;
	}

	OnSelectedPersonEvent.Broadcast(SelectedPerson);
}
