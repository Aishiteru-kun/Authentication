#include "Subsystem/AuthApiClientSubsystem.h"

#include "Engine/World.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "TimerManager.h"

static FString EncodeUrl(const FString& S)
{
	FString Out = S;
	Out = FGenericPlatformHttp::UrlEncode(Out);
	return Out;
}

// --------------------- Core HTTP ---------------------
void UAuthApiClientSubsystem::SendJsonPOST(
	const FString& UrlPath,
	const TSharedRef<FJsonObject>& JsonBody,
	TFunction<void(bool, const FString&, const FString&)> OnDone,
	const TMap<FString, FString>& ExtraHeaders)
{
	const FString Url = MakeUrl(UrlPath);

	FString BodyStr;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyStr);
	FJsonSerializer::Serialize(JsonBody, Writer);

	auto Req = FHttpModule::Get().CreateRequest();
	Req->SetURL(Url);
	Req->SetVerb(TEXT("POST"));
	Req->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	for (const auto& Kvp : ExtraHeaders)
	{
		Req->SetHeader(Kvp.Key, Kvp.Value);
	}
	Req->SetContentAsString(BodyStr);

	Req->OnProcessRequestComplete().BindLambda(
		[OnDone](FHttpRequestPtr, FHttpResponsePtr Res, bool bOk)
		{
			if (!bOk || !Res.IsValid())
			{
				OnDone(false, FString(), TEXT("HTTP error"));
				return;
			}
			if (Res->GetResponseCode() < 200 || Res->GetResponseCode() >= 300)
			{
				OnDone(false, FString(), FString::Printf(TEXT("HTTP %d"), Res->GetResponseCode()));
				return;
			}
			OnDone(true, Res->GetContentAsString(), FString());
		});
	Req->ProcessRequest();
}

void UAuthApiClientSubsystem::SendGET(
	const FString& UrlPathWithQuery,
	TFunction<void(bool, const FString&, const FString&)> OnDone,
	const TMap<FString, FString>& ExtraHeaders)
{
	const FString Url = MakeUrl(UrlPathWithQuery);

	auto Req = FHttpModule::Get().CreateRequest();
	Req->SetURL(Url);
	Req->SetVerb(TEXT("GET"));
	for (const auto& Kvp : ExtraHeaders)
	{
		Req->SetHeader(Kvp.Key, Kvp.Value);
	}

	Req->OnProcessRequestComplete().BindLambda(
		[OnDone](FHttpRequestPtr, FHttpResponsePtr Res, bool bOk)
		{
			if (!bOk || !Res.IsValid())
			{
				OnDone(false, FString(), TEXT("HTTP error"));
				return;
			}
			if (Res->GetResponseCode() < 200 || Res->GetResponseCode() >= 300)
			{
				OnDone(false, FString(), FString::Printf(TEXT("HTTP %d"), Res->GetResponseCode()));
				return;
			}
			OnDone(true, Res->GetContentAsString(), FString());
		});
	Req->ProcessRequest();
}

void UAuthApiClientSubsystem::ParseJson(const FString& Text, TSharedPtr<FJsonObject>& OutObject, bool& bOutOK, FString& OutError) const
{
	OutObject.Reset();
	bOutOK = false;
	OutError.Empty();

	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	TSharedPtr<FJsonObject> Obj;
	if (!FJsonSerializer::Deserialize(Reader, Obj) || !Obj.IsValid())
	{
		OutError = TEXT("Invalid JSON");
		return;
	}
	OutObject = Obj;
	bOutOK = true;
}

// --------------------- Health ---------------------
void UAuthApiClientSubsystem::CheckHealth(TFunction<void(bool bOK)> OnDone)
{
	SendGET(TEXT("/health"),
		[OnDone](bool bOK, const FString& Body, const FString& Err)
		{
			OnDone(bOK && Body.Contains(TEXT("\"ok\":true")));
		});
}

// --------------------- Auth ---------------------
void UAuthApiClientSubsystem::RequestLoginByLogin(const FString& InLogin, const FString& InDescription, FOnLoginRequested OnDone)
{
	TSharedRef<FJsonObject> J = MakeShared<FJsonObject>();
	J->SetStringField(TEXT("login"), InLogin);
	if (!InDescription.IsEmpty())
	{
		J->SetStringField(TEXT("description"), InDescription);
	}
	SendJsonPOST(TEXT("/auth/request_login"), J,
		[this, OnDone](bool bOK, const FString& Body, const FString& Err)
		{
			if (!bOK) { OnDone.ExecuteIfBound(false, FString(), Err); return; }
			TSharedPtr<FJsonObject> Obj; bool PJ=false; FString PErr;
			ParseJson(Body, Obj, PJ, PErr);
			if (!PJ || !Obj.IsValid()) { OnDone.ExecuteIfBound(false, FString(), TEXT("Bad JSON")); return; }
			const FString ReqId = Obj->GetStringField(TEXT("request_id"));
			OnDone.ExecuteIfBound(true, ReqId, FString());
		});
}

void UAuthApiClientSubsystem::PollLoginStatus(const FString& InRequestId, FOnLoginApproved OnDone, float TimeoutSeconds)
{
	if (!GetWorld()) { OnDone.ExecuteIfBound(false, FString(), TEXT("No World")); return; }

	float* TimeLeft = new float(TimeoutSeconds);
	FTimerDelegate Tick;
	Tick.BindLambda([this, InRequestId, OnDone, TimeLeft]()
	{
		SendGET(FString::Printf(TEXT("/auth/status?request_id=%s"), *EncodeUrl(InRequestId)),
			[this, OnDone, TimeLeft](bool bOK, const FString& Body, const FString& Err)
			{
				if (!bOK) { /* keep polling; only stop on timeout */ return; }
				TSharedPtr<FJsonObject> Obj; bool PJ=false; FString PErr;
				ParseJson(Body, Obj, PJ, PErr);
				if (!PJ || !Obj.IsValid()) return;

				const FString Status = Obj->GetStringField(TEXT("status"));
				if (!Status.Equals(TEXT("pending"), ESearchCase::IgnoreCase))
				{
					GetWorld()->GetTimerManager().ClearTimer(LoginPollTimer);
					FString Token;
					if (Status.Equals(TEXT("approved"), ESearchCase::IgnoreCase))
					{
						Token = Obj->GetStringField(TEXT("session_token"));
						OnDone.ExecuteIfBound(true, Token, FString());
					}
					else
					{
						OnDone.ExecuteIfBound(false, FString(), Status);
					}
					delete TimeLeft;
				}
			});

		*TimeLeft -= PollIntervalSeconds;
		if (*TimeLeft <= 0.f)
		{
			GetWorld()->GetTimerManager().ClearTimer(LoginPollTimer);
			OnDone.ExecuteIfBound(false, FString(), TEXT("timeout"));
			delete TimeLeft;
		}
	});

	GetWorld()->GetTimerManager().SetTimer(LoginPollTimer, Tick, PollIntervalSeconds, true);
}

// --------------------- Shop ---------------------
void UAuthApiClientSubsystem::GetItemPrice(const FString& InItemId, int32 InQuantity, FOnPrice OnDone)
{
	const int32 Qty = FMath::Max(1, InQuantity);
	SendGET(FString::Printf(TEXT("/shop/price?item_id=%s&qty=%d"), *EncodeUrl(InItemId), Qty),
		[this, OnDone, InItemId](bool bOK, const FString& Body, const FString& Err)
		{
			if (!bOK) { OnDone.ExecuteIfBound(false, 0, 0, InItemId, Err); return; }
			TSharedPtr<FJsonObject> Obj; bool PJ=false; FString PErr;
			ParseJson(Body, Obj, PJ, PErr);
			if (!PJ || !Obj.IsValid()) { OnDone.ExecuteIfBound(false, 0, 0, InItemId, TEXT("Bad JSON")); return; }
			const int32 Unit = Obj->GetIntegerField(TEXT("unit_price"));
			const int32 Total = Obj->GetIntegerField(TEXT("total_price"));
			OnDone.ExecuteIfBound(true, Unit, Total, InItemId, FString());
		});
}

void UAuthApiClientSubsystem::PurchaseRequest(const FString& InSessionToken, const FString& InItemId, int32 InQuantity,
                                              FOnPurchaseRequested OnDone)
{
	TSharedRef<FJsonObject> J = MakeShared<FJsonObject>();
	J->SetStringField(TEXT("item_id"), InItemId);
	J->SetNumberField(TEXT("quantity"), FMath::Max(1, InQuantity));

	TMap<FString,FString> Headers;
	Headers.Add(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *InSessionToken));

	SendJsonPOST(TEXT("/shop/purchase/request"), J,
		[this, OnDone](bool bOK, const FString& Body, const FString& Err)
		{
			if (!bOK) { OnDone.ExecuteIfBound(false, FString(), Err); return; }
			TSharedPtr<FJsonObject> Obj; bool PJ=false; FString PErr;
			ParseJson(Body, Obj, PJ, PErr);
			if (!PJ || !Obj.IsValid()) { OnDone.ExecuteIfBound(false, FString(), TEXT("Bad JSON")); return; }
			const FString ReqId = Obj->GetStringField(TEXT("request_id"));
			OnDone.ExecuteIfBound(true, ReqId, FString());
		}, Headers);
}

void UAuthApiClientSubsystem::PollPurchaseStatus(const FString& InRequestId, FOnPurchaseStatus OnDone, float TimeoutSeconds)
{
	if (!GetWorld()) { OnDone.ExecuteIfBound(false, TEXT(""), TEXT(""), TEXT("No World")); return; }

	float* TimeLeft = new float(TimeoutSeconds);
	FTimerDelegate Tick;
	Tick.BindLambda([this, InRequestId, OnDone, TimeLeft]()
	{
		SendGET(FString::Printf(TEXT("/shop/purchase/status?request_id=%s"), *EncodeUrl(InRequestId)),
			[this, OnDone, TimeLeft](bool bOK, const FString& Body, const FString& Err)
			{
				if (!bOK) { return; }
				TSharedPtr<FJsonObject> Obj; bool PJ=false; FString PErr;
				ParseJson(Body, Obj, PJ, PErr);
				if (!PJ || !Obj.IsValid()) return;

				const FString Status = Obj->GetStringField(TEXT("status"));
				if (!Status.Equals(TEXT("pending"), ESearchCase::IgnoreCase))
				{
					GetWorld()->GetTimerManager().ClearTimer(PurchasePollTimer);

					// скомпонуємо короткий human text з полів
					FString Human;
					if (Obj->HasTypedField<EJson::String>(TEXT("item_id")))
					{
						const FString ItemId = Obj->GetStringField(TEXT("item_id"));
						const int32 Qty = Obj->GetIntegerField(TEXT("quantity"));
						const int32 Unit = Obj->GetIntegerField(TEXT("unit_price"));
						const int32 Total = Obj->GetIntegerField(TEXT("total_price"));
						Human = FString::Printf(TEXT("%s x%d @ %d = %d"), *ItemId, Qty, Unit, Total);
					}

					OnDone.ExecuteIfBound(Status.Equals(TEXT("approved"), ESearchCase::IgnoreCase), Status, Human, FString());
					delete TimeLeft;
				}
			});

		*TimeLeft -= PollIntervalSeconds;
		if (*TimeLeft <= 0.f)
		{
			GetWorld()->GetTimerManager().ClearTimer(PurchasePollTimer);
			OnDone.ExecuteIfBound(false, TEXT("timeout"), TEXT(""), TEXT(""));
			delete TimeLeft;
		}
	});

	GetWorld()->GetTimerManager().SetTimer(PurchasePollTimer, Tick, PollIntervalSeconds, true);
}

// --------------------- User / Inventory ---------------------
void UAuthApiClientSubsystem::GetUserByLogin(const FString& InLogin,
	TFunction<void(bool, int64, const FString&)> OnDone)
{
	SendGET(FString::Printf(TEXT("/users/by_login?login=%s"), *EncodeUrl(InLogin)),
		[this, OnDone](bool bOK, const FString& Body, const FString& Err)
		{
			if (!bOK) { OnDone(false, 0, Err); return; }
			TSharedPtr<FJsonObject> Obj; bool PJ=false; FString PErr;
			ParseJson(Body, Obj, PJ, PErr);
			if (!PJ || !Obj.IsValid()) { OnDone(false, 0, TEXT("Bad JSON")); return; }
			const int64 ChatId = (int64)Obj->GetNumberField(TEXT("telegram_chat_id"));
			OnDone(true, ChatId, FString());
		});
}

void UAuthApiClientSubsystem::GetInventoryByChatId(int64 ChatId,
	TFunction<void(bool, int64, const TArray<FServerInventoryItem>&, const FString&)> OnDone)
{
	SendGET(FString::Printf(TEXT("/users/%lld/inventory"), (long long)ChatId),
		[this, OnDone](bool bOK, const FString& Body, const FString& Err)
		{
			if (!bOK) { OnDone(false, 0, {}, Err); return; }
			TSharedPtr<FJsonObject> Obj; bool PJ=false; FString PErr;
			ParseJson(Body, Obj, PJ, PErr);
			if (!PJ || !Obj.IsValid()) { OnDone(false, 0, {}, TEXT("Bad JSON")); return; }

			const int64 Gold = (int64)Obj->GetNumberField(TEXT("gold"));
			TArray<FServerInventoryItem> Items;

			const TArray<TSharedPtr<FJsonValue>>* Arr;
			if (Obj->TryGetArrayField(TEXT("items"), Arr))
			{
				for (const auto& V : *Arr)
				{
					const TSharedPtr<FJsonObject>* ItObj;
					if (V->TryGetObject(ItObj))
					{
						FServerInventoryItem It;
						It.Id = (*ItObj)->GetStringField(TEXT("id"));
						It.Name = (*ItObj)->GetStringField(TEXT("name"));
						It.Quantity = (int32)(*ItObj)->GetNumberField(TEXT("quantity"));
						Items.Add(It);
					}
				}
			}
			OnDone(true, Gold, Items, FString());
		});
}

void UAuthApiClientSubsystem::GetInventoryByLogin(const FString& InLogin,
	TFunction<void(bool, int64, const TArray<FServerInventoryItem>&, const FString&)> OnDone)
{
	GetUserByLogin(InLogin, [this, OnDone](bool bOK, int64 ChatId, const FString& Err)
	{
		if (!bOK) { OnDone(false, 0, {}, Err); return; }
		GetInventoryByChatId(ChatId, OnDone);
	});
}

// --------------------- Trades ---------------------
void UAuthApiClientSubsystem::TradeCreate(const FString& InSessionToken, int64 ToChatId,
	const TArray<FString>& OfferItems, const TArray<FString>& RequestItems, FOnTradeCreated OnDone)
{
	TSharedRef<FJsonObject> J = MakeShared<FJsonObject>();
	J->SetNumberField(TEXT("to"), (double)ToChatId);

	// offer_items
	{
		TArray<TSharedPtr<FJsonValue>> Arr;
		for (const FString& S : OfferItems)
		{
			Arr.Add(MakeShared<FJsonValueString>(S));
		}
		J->SetArrayField(TEXT("offer_items"), Arr);
	}
	// request_items
	{
		TArray<TSharedPtr<FJsonValue>> Arr;
		for (const FString& S : RequestItems)
		{
			Arr.Add(MakeShared<FJsonValueString>(S));
		}
		J->SetArrayField(TEXT("request_items"), Arr);
	}

	TMap<FString,FString> Headers;
	Headers.Add(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *InSessionToken));

	SendJsonPOST(TEXT("/trade/create"), J,
		[this, OnDone](bool bOK, const FString& Body, const FString& Err)
		{
			if (!bOK) { OnDone.ExecuteIfBound(false, FString(), Err); return; }
			TSharedPtr<FJsonObject> Obj; bool PJ=false; FString PErr;
			ParseJson(Body, Obj, PJ, PErr);
			if (!PJ || !Obj.IsValid()) { OnDone.ExecuteIfBound(false, FString(), TEXT("Bad JSON")); return; }
			const FString TradeId = Obj->GetStringField(TEXT("trade_id"));
			OnDone.ExecuteIfBound(true, TradeId, FString());
		}, Headers);
}

void UAuthApiClientSubsystem::PollTradeStatus(const FString& InTradeId, FOnTradeStatus OnDone, float TimeoutSeconds)
{
	if (!GetWorld()) { OnDone.ExecuteIfBound(false, TEXT(""), TEXT("No World")); return; }

	float* TimeLeft = new float(TimeoutSeconds);
	FTimerDelegate Tick;
	Tick.BindLambda([this, InTradeId, OnDone, TimeLeft]()
	{
		SendGET(FString::Printf(TEXT("/trade/status?trade_id=%s"), *EncodeUrl(InTradeId)),
			[this, OnDone, TimeLeft](bool bOK, const FString& Body, const FString& Err)
			{
				if (!bOK) { return; }
				TSharedPtr<FJsonObject> Obj; bool PJ=false; FString PErr;
				ParseJson(Body, Obj, PJ, PErr);
				if (!PJ || !Obj.IsValid()) return;

				const FString Status = Obj->GetStringField(TEXT("status"));
				if (!Status.Equals(TEXT("pending"), ESearchCase::IgnoreCase))
				{
					GetWorld()->GetTimerManager().ClearTimer(TradePollTimer);
					OnDone.ExecuteIfBound(Status.Equals(TEXT("approved"), ESearchCase::IgnoreCase), Status, FString());
					delete TimeLeft;
				}
			});

		*TimeLeft -= PollIntervalSeconds;
		if (*TimeLeft <= 0.f)
		{
			GetWorld()->GetTimerManager().ClearTimer(TradePollTimer);
			OnDone.ExecuteIfBound(false, TEXT("timeout"), TEXT(""));
			delete TimeLeft;
		}
	});

	GetWorld()->GetTimerManager().SetTimer(TradePollTimer, Tick, PollIntervalSeconds, true);
}
