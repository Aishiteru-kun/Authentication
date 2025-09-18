// Fill out your copyright notice in the Description page of Project Settings.


#include "UMG/PersonSlot.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UPersonSlot::InitializeWidget(const FString& InFullInfo)
{
	if (PersonNameText)
	{
		FullInfo = InFullInfo;
		FString InPersonName;
		InFullInfo.Split(TEXT("_"), &InPersonName, nullptr);

		PersonNameText->SetText(FText::FromString(InPersonName));
	}
}

void UPersonSlot::NativeConstruct()
{
	Super::NativeConstruct();

	if (SelectButton)
	{
		SelectButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnClicked);
	}
}

void UPersonSlot::OnClicked()
{
	if (OnSelectedPerson.IsBound())
	{
		OnSelectedPerson.Execute(FullInfo);
	}
}
