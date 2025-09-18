// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interface/WidgetInterface.h"
#include "SelectPerson.generated.h"

class UButton;
class UPersonSlot;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSelectedPerson, const FString&, SelectedPerson);

UCLASS()
class AUTHENTICATION_API USelectPerson : public UUserWidget, public IWidgetInterface
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Person Event")
	FOnSelectedPerson OnSelectedPersonEvent;

public:
	virtual void OnActivate_Implementation() override;
	virtual void OnDeactivate_Implementation() override;

protected:
	virtual void NativeConstruct() override;

protected:
	UFUNCTION()
	void OnTradeClicked();

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UPanelWidget> Panel;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> TradeButton;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SelectPerson")
	TSubclassOf<UPersonSlot> PersonSlotWidgetClass;

private:
	FString SelectedPerson;
};
