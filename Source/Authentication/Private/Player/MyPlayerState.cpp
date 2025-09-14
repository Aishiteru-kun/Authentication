// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/MyPlayerState.h"

#include "Net/UnrealNetwork.h"

AMyPlayerState::AMyPlayerState(const FObjectInitializer& ObjectInitializer)
{
	bUseCustomPlayerNames = true;

	SetReplicates(true);

	SetNetUpdateFrequency(100.0f);
}

void AMyPlayerState::OnRep_PlayerChatId(FString InPlayerChatId)
{
	PlayerChatId = InPlayerChatId;
	OnPlayerChatIdChanged.Broadcast(PlayerChatId);
}

void AMyPlayerState::OnRep_SessionToken(FString InSessionToken)
{
	SessionToken = InSessionToken;
	OnSessionTokenChanged.Broadcast(SessionToken);
}

void AMyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMyPlayerState, PlayerChatId);
}
