// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/MyPlayerController.h"

#include "Player/MyPlayerState.h"

void AMyPlayerController::Server_ChangeDataForPlayerState_Implementation(AMyPlayerState* InPlayerState, const FString& NewPlayerName, const FString& NewPlayerChatId, const FString& NewSessionToken)
{
	if (!HasAuthority())
	{
		return;
	}

	if (InPlayerState)
	{
		InPlayerState->SetPlayerName(NewPlayerName);
		InPlayerState->SetPlayerChatId(NewPlayerChatId);
		InPlayerState->SetSessionToken(NewSessionToken);
	}
}
