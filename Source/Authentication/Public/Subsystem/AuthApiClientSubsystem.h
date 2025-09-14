// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AuthApiClientSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FServerInventoryItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 Quantity = 0;
};

DECLARE_DELEGATE_ThreeParams(FOnLoginRequested, bool /*bOK*/, FString /*RequestId*/, FString /*ErrorText*/);
DECLARE_DELEGATE_ThreeParams(FOnLoginApproved,  bool /*bOK*/, FString /*SessionToken*/, FString /*ErrorText*/);
DECLARE_DELEGATE_FiveParams(FOnPrice,           bool /*bOK*/, int32 /*UnitPrice*/, int32 /*TotalPrice*/, FString /*ItemId*/, FString /*ErrorText*/);
DECLARE_DELEGATE_ThreeParams(FOnPurchaseRequested, bool /*bOK*/, FString /*RequestId*/, FString /*ErrorText*/);
DECLARE_DELEGATE_FourParams(FOnPurchaseStatus,  bool /*bOK*/, FString /*Status*/, FString /*HumanInfo*/, FString /*ErrorText*/);

// Діагностичне (отримати chat_id, verified, gold за логіном)
DECLARE_DELEGATE_SixParams(FOnUserByLogin, bool/*bOK*/, FString/*Login*/, int64/*ChatId*/, bool/*Verified*/, int64/*Gold*/, FString/*Error*/);

// Інвентар
DECLARE_DELEGATE_FourParams(FOnInventory, bool/*bOK*/, int64/*Gold*/, TArray<FServerInventoryItem>/*Items*/, FString/*Error*/);

UCLASS()
class AUTHENTICATION_API UAuthApiClientSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Наприклад, http://localhost:8080 */
	UPROPERTY(EditAnywhere, Category="AuthAPI")
	FString ServerBaseUrl = TEXT("http://localhost:8080");

	/** Інтервал полінгу (сек) */
	UPROPERTY(EditAnywhere, Category="AuthAPI")
	float PollIntervalSeconds = 1.0f;

	// --- Пінг ---
	void CheckHealth(TFunction<void(bool bOK)> OnDone);

	// --- Логін by login ---
	void RequestLoginByLogin(const FString& InLogin, const FString& InDescription, FOnLoginRequested OnDone);

	/** Полінг статусу логіну; повертає SessionToken, коли approved */
	void PollLoginStatus(const FString& InRequestId, FOnLoginApproved OnDone, float TimeoutSeconds = 60.f);

	// --- Магазин ---
	void GetItemPrice(const FString& InItemId, int32 InQuantity, FOnPrice OnDone);

	/** Надіслати заявку на покупку (Bearer потрібен) */
	void PurchaseRequest(const FString& InSessionToken, const FString& InItemId, int32 InQuantity, FOnPurchaseRequested OnDone);

	/** Полінг статусу покупки */
	void PollPurchaseStatus(const FString& InRequestId, FOnPurchaseStatus OnDone, float TimeoutSeconds = 60.f);

	// --- Користувач / інвентар ---
	void GetUserByLogin(const FString& InLogin, FOnUserByLogin OnDone);

	/** Якщо знаєш chat_id */
	void GetInventoryByChatId(int64 ChatId, FOnInventory OnDone);

	/** Зручно: за логіном дістає chat_id і вже потім інвентар */
	void GetInventoryByLogin(const FString& InLogin, FOnInventory OnDone);

private:
	void SendJsonPOST(const FString& UrlPath, const TSharedRef<FJsonObject>& JsonBody,
	                  TFunction<void(bool, const FString&, const FString&)> OnDone,
	                  const TMap<FString, FString>& ExtraHeaders = {});

	void SendGET(const FString& UrlPathWithQuery,
	             TFunction<void(bool, const FString&, const FString&)> OnDone,
	             const TMap<FString, FString>& ExtraHeaders = {});

	/** Нормалізує базу (без фінального слеша) і гарантує, що Path починається зі слеша */
	FString MakeUrl(const FString& Path) const;

	void ParseJson(const FString& Text, TSharedPtr<FJsonObject>& OutObject, bool& bOutOK, FString& OutError) const;

private:
	FTimerHandle LoginPollTimer;
	FTimerHandle PurchasePollTimer;
};
