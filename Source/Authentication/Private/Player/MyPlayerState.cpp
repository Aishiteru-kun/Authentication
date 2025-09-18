// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/MyPlayerState.h"

#include "Net/UnrealNetwork.h"

AMyPlayerState::AMyPlayerState(const FObjectInitializer& ObjectInitializer)
{
	bUseCustomPlayerNames = true;

	SetReplicates(true);

	SetNetUpdateFrequency(100.0f);
}

void AMyPlayerState::SetPlayerChatId(const FString& InPlayerChatId)
{
	if (HasAuthority())
	{
		PlayerChatId = InPlayerChatId;
		OnRep_PlayerChatId();
	}
}

void AMyPlayerState::SetSessionToken(const FString& InSessionToken)
{
	if (HasAuthority())
	{
		SessionToken = InSessionToken;
		OnRep_SessionToken();
	}
}

void AMyPlayerState::OnRep_PlayerChatId()
{
	OnPlayerChatIdChanged.Broadcast(GetPlayerChatId());
}

void AMyPlayerState::OnRep_SessionToken()
{
	OnSessionTokenChanged.Broadcast(GetSessionToken());
}

void AMyPlayerState::OnRep_PlayerName()
{
	Super::OnRep_PlayerName();

	OnPlayerNameChanged.Broadcast(PlayerChatId);
}

void AMyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMyPlayerState, PlayerChatId);
	DOREPLIFETIME(AMyPlayerState, SessionToken);
}
