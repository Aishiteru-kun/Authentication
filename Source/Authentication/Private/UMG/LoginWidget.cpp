// Fill out your copyright notice in the Description page of Project Settings.


#include "UMG/LoginWidget.h"

#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Player/MyPlayerState.h"
#include "Subsystem/AuthApiClientSubsystem.h"

void ULoginWidget::OnLoginTextChanged(const FText& InText)
{
	if (ErrorMsg && ErrorMsg->GetVisibility() == ESlateVisibility::Visible)
	{
		ErrorMsg->SetVisibility(ESlateVisibility::Collapsed);
		ErrorMsg->SetText(FText::FromString(""));
	}
}

void ULoginWidget::OnLoginTextCommitted(const FText& InText, ETextCommit::Type InCommitType)
{
	if (ErrorMsg && ErrorMsg->GetVisibility() == ESlateVisibility::Visible)
	{
		ErrorMsg->SetVisibility(ESlateVisibility::Collapsed);
		ErrorMsg->SetText(FText::FromString(""));
	}

	if (InCommitType != ETextCommit::OnEnter)
	{
		return;
	}

	LoginTextBox->SetIsEnabled(false);

	const FString Login = InText.ToLower().ToString();

	GetGameInstance()->GetSubsystem<UAuthApiClientSubsystem>()->CheckHealth([this, Login](bool bOk)
	{
		OnProgress(bOk, Login);
	});
}

void ULoginWidget::OnProgress(const bool bOk, FString Id)
{
	if (!bOk)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server is not available"));

		ErrorMsg->SetVisibility(ESlateVisibility::Visible);
		ErrorMsg->SetText(FText::FromString("Server is not available"));

		LoginTextBox->SetIsEnabled(true);

		return;
	}

	UAuthApiClientSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UAuthApiClientSubsystem>();

	FOnLoginRequested Delegate;
	Delegate.BindUObject(this, &ThisClass::RequestLoginByLogin);

	Subsystem->RequestLoginByLogin(Id, TEXT("Unreal Engine client"), Delegate);
}

void ULoginWidget::RequestLoginByLogin(bool bOK, FString RequestId, FString ErrorText)
{
	if (!bOK)
	{
		UE_LOG(LogTemp, Warning, TEXT("RequestLoginByLogin failed: %s"), *ErrorText);

		ErrorMsg->SetVisibility(ESlateVisibility::Visible);
		ErrorMsg->SetText(FText::FromString(ErrorText));

		LoginTextBox->SetIsEnabled(true);

		return;
	}

	UAuthApiClientSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UAuthApiClientSubsystem>();

	FOnLoginApproved Delegate;
	Delegate.BindUObject(this, &ThisClass::RequestLoginApproved);

	Subsystem->PollLoginStatus(RequestId, Delegate, 65.0f);
}

void ULoginWidget::RequestLoginApproved(bool bOK, FString SessionToken, FString ErrorText)
{
	LoginTextBox->SetIsEnabled(true);

	if (!bOK)
	{
		UE_LOG(LogTemp, Warning, TEXT("RequestLoginApproved failed: %s"), *ErrorText);

		ErrorMsg->SetVisibility(ESlateVisibility::Visible);
		ErrorMsg->SetText(FText::FromString(ErrorText));

		return;
	}
	
	if (AMyPlayerState* PS = GetOwningPlayerState<AMyPlayerState>();
		PS)
	{
		FString Login = LoginTextBox->GetText().ToString();
		Login.TrimStartAndEndInline();

		FString Name, ChatId;
		Login.Split("_", &Name, &ChatId);

		PS->SetPlayerName(Name);
		PS->SetPlayerChatId(ChatId);
		PS->SetSessionToken(SessionToken);
	}

	RemoveFromParent();
}

void ULoginWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LoginTextBox)
	{
		LoginTextBox->OnTextCommitted.AddUniqueDynamic(this, &ThisClass::OnLoginTextCommitted);
		LoginTextBox->OnTextChanged.AddUniqueDynamic(this, &ThisClass::OnLoginTextChanged);
	}
}
