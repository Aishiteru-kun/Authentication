// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AuthApiClientSubsystem.generated.h"

// ---- Delegates (залишені як були) ----
DECLARE_DELEGATE_ThreeParams(FOnLoginRequested, bool /*bOK*/, FString /*RequestId*/, FString /*ErrorText*/);
DECLARE_DELEGATE_ThreeParams(FOnLoginApproved, bool /*bOK*/, FString /*SessionToken*/, FString /*ErrorText*/);
DECLARE_DELEGATE_FiveParams(FOnPrice, bool /*bOK*/, int32 /*UnitPrice*/, int32 /*TotalPrice*/, FString /*ItemId*/,
                            FString /*ErrorText*/);
DECLARE_DELEGATE_ThreeParams(FOnPurchaseRequested, bool /*bOK*/, FString /*RequestId*/, FString /*ErrorText*/);
DECLARE_DELEGATE_FourParams(FOnPurchaseStatus, bool /*bOK*/, FString /*Status*/, FString /*HumanInfo*/,
                            FString /*ErrorText*/);

// ---- Нові типи ----
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

DECLARE_DELEGATE_ThreeParams(FOnTradeCreated, bool /*bOK*/, FString /*TradeId*/, FString /*ErrorText*/);
DECLARE_DELEGATE_ThreeParams(FOnTradeStatus, bool /*bOK*/, FString /*Status*/, FString /*ErrorText*/);

UCLASS()
class AUTHENTICATION_API UAuthApiClientSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="AuthAPI")
	FString ServerBaseUrl = TEXT("http://localhost:8080");

	// Інтервал полінгу (сек)
	UPROPERTY(EditAnywhere, Category="AuthAPI")
	float PollIntervalSeconds = 1.0f;

	// ---------- Health ----------
	void CheckHealth(TFunction<void(bool bOK)> OnDone);

	// ---------- Логін ----------
	void RequestLoginByLogin(const FString& InLogin, const FString& InDescription, FOnLoginRequested OnDone);
	void PollLoginStatus(const FString& InRequestId, FOnLoginApproved OnDone, float TimeoutSeconds = 60.f);

	// ---------- Магазин ----------
	void GetItemPrice(const FString& InItemId, int32 InQuantity, FOnPrice OnDone);

	/** Надіслати заявку на покупку (Bearer потрібен) */
	void PurchaseRequest(const FString& InSessionToken, const FString& InItemId, int32 InQuantity,
	                     FOnPurchaseRequested OnDone);

	void PollPurchaseStatus(const FString& InRequestId, FOnPurchaseStatus OnDone, float TimeoutSeconds = 60.f);

	// ---------- Користувач / Інвентар ----------
	/** Дає telegram_chat_id за логіном (для /users/<chat_id>/inventory) */
	void GetUserByLogin(const FString& InLogin,
	                    TFunction<void(bool /*bOK*/, int64 /*ChatId*/, const FString& /*ErrorText*/)> OnDone);

	/** Прочитати інвентар за chat_id */
	void GetInventoryByChatId(int64 ChatId,
		TFunction<void(bool /*bOK*/, int64 /*Gold*/, const TArray<FServerInventoryItem>& /*Items*/, const FString& /*ErrorText*/)> OnDone);

	/** Зручний хелпер: логін -> chat_id -> інвентар */
	void GetInventoryByLogin(const FString& InLogin,
		TFunction<void(bool /*bOK*/, int64 /*Gold*/, const TArray<FServerInventoryItem>& /*Items*/, const FString& /*ErrorText*/)> OnDone);

	// ---------- Трейди ----------
	/** Створити трейд (Bearer) */
	void TradeCreate(const FString& InSessionToken, int64 ToChatId,
	                 const TArray<FString>& OfferItems, const TArray<FString>& RequestItems,
	                 FOnTradeCreated OnDone);

	/** Полінг статусу трейду */
	void PollTradeStatus(const FString& InTradeId, FOnTradeStatus OnDone, float TimeoutSeconds = 60.f);

private:
	// ---- HTTP helpers ----
	void SendJsonPOST(const FString& UrlPath, const TSharedRef<FJsonObject>& JsonBody,
	                  TFunction<void(bool, const FString&, const FString&)> OnDone,
	                  const TMap<FString, FString>& ExtraHeaders = {});

	void SendGET(const FString& UrlPathWithQuery,
	             TFunction<void(bool, const FString&, const FString&)> OnDone,
	             const TMap<FString, FString>& ExtraHeaders = {});

	FString MakeUrl(const FString& Path) const { return ServerBaseUrl.TrimEnd() + Path; }
	void ParseJson(const FString& Text, TSharedPtr<FJsonObject>& OutObject, bool& bOutOK, FString& OutError) const;

	// ---- Timers ----
	FTimerHandle LoginPollTimer;
	FTimerHandle PurchasePollTimer;
	FTimerHandle TradePollTimer;
};