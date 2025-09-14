// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoginWidget.generated.h"

class UTextBlock;
class UEditableTextBox;
/**
 * 
 */
UCLASS()
class AUTHENTICATION_API ULoginWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UFUNCTION()
	void OnLoginTextChanged(const FText& InText);

	UFUNCTION()
	void OnLoginTextCommitted(const FText& InText, ETextCommit::Type InCommitType);

	void OnProgress(const bool bOk, FString Id);

	void RequestLoginByLogin(bool bOK, FString RequestId, FString ErrorText);

	void RequestLoginApproved(bool bOK, FString SessionToken, FString ErrorText);

protected:
	virtual void NativeConstruct() override;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> LoginTextBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ErrorMsg;
};
