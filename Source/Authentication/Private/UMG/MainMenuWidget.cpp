// Fill out your copyright notice in the Description page of Project Settings.


#include "UMG/MainMenuWidget.h"

#include "Components/TextBlock.h"
#include "Player/MyPlayerState.h"

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PlayerNameText)
	{
		const AMyPlayerState* PS = GetOwningPlayerState<AMyPlayerState>();
		PlayerNameText->SetText(FText::FromString(PS->GetPlayerNameCustom()));
	}
}
