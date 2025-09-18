// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PersonSlot.generated.h"

class UTextBlock;
class UButton;

DECLARE_DELEGATE_OneParam(FOnSelectedPersonSlot, const FString& /*PersonName*/);

UCLASS()
class AUTHENTICATION_API UPersonSlot : public UUserWidget
{
	GENERATED_BODY()

public:
	FOnSelectedPersonSlot OnSelectedPerson;

public:
	void InitializeWidget(const FString& InFullInfo);

protected:
	virtual void NativeConstruct() override;

protected:
	UFUNCTION()
	void OnClicked();

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> PersonNameText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> SelectButton;

private:
	FString FullInfo;
};
