// Fill out your copyright notice in the Description page of Project Settings.


#include "UMG/MainMenuWidget.h"

#include "Components/TextBlock.h"
#include "Player/MyPlayerState.h"

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PlayerNameText)
	{
		AMyPlayerState* PS = GetOwningPlayerState<AMyPlayerState>();
		PS->OnPlayerNameChanged.AddUniqueDynamic(this, &ThisClass::UpdatePlayerName);

		PlayerNameText->SetText(FText::FromString(PS->GetPlayerName()));
	}
}

void UMainMenuWidget::UpdatePlayerName(const FString& InPlayerName)
{
	if (PlayerNameText)
	{
		const AMyPlayerState* PS = GetOwningPlayerState<AMyPlayerState>();
		PlayerNameText->SetText(FText::FromString(PS->GetPlayerName()));
	}
}
