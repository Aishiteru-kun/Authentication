// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

class AMyPlayerState;
/**
 * 
 */
UCLASS()
class AUTHENTICATION_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(Server, Reliable)
	void Server_ChangeDataForPlayerState(AMyPlayerState* InPlayerState, const FString& NewPlayerName, const FString& NewPlayerChatId, const FString& NewSessionToken);
};
