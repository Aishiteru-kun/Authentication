// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MyPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDataChanged, const FString&, NewPlayerChatId);

UCLASS()
class AUTHENTICATION_API AMyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AMyPlayerState(const FObjectInitializer& ObjectInitializer);

public:
	UPROPERTY(BlueprintAssignable)
	FOnDataChanged OnPlayerChatIdChanged;

	UPROPERTY(BlueprintAssignable)
	FOnDataChanged OnSessionTokenChanged;

public:
	FORCEINLINE const FString& GetPlayerChatId() const { return PlayerChatId; }
	void SetPlayerChatId(const FString& InPlayerChatId) { PlayerChatId = InPlayerChatId; }

	FORCEINLINE const FString& GetSessionToken() const { return SessionToken; }
	void SetSessionToken(const FString& InSessionToken) { SessionToken = InSessionToken; }

protected:
	UFUNCTION()
	void OnRep_PlayerChatId(FString InPlayerChatId);

	UFUNCTION()
	void OnRep_SessionToken(FString InSessionToken);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_PlayerChatId)
	FString PlayerChatId;

	UPROPERTY(ReplicatedUsing = OnRep_SessionToken)
	FString SessionToken;
};
