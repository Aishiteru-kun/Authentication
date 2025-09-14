// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystem/AuthApiClientSubsystem.h"

#include "HttpModule.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

/////////////////////////////////////////////////////
// Helpers

FString UAuthApiClientSubsystem::MakeUrl(const FString& InPath) const
{
	FString Base = ServerBaseUrl;
	Base.RemoveFromEnd(TEXT("/"));

	FString Path = InPath;
	if (!Path.StartsWith(TEXT("/")))
	{
		Path = TEXT("/") + Path;
	}
	return Base + Path;
}

void UAuthApiClientSubsystem::ParseJson(const FString& Text, TSharedPtr<FJsonObject>& OutObject, bool& bOutOK, FString& OutError) const
{
	bOutOK = false; OutError.Empty(); OutObject.Reset();
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	if (FJsonSerializer::Deserialize(Reader, OutObject) && OutObject.IsValid())
	{
		bOutOK = true;
	}
	else
	{
		OutError = TEXT("JSON parse error");
	}
}

/////////////////////////////////////////////////////
// Public API

void UAuthApiClientSubsystem::CheckHealth(TFunction<void(bool bOK)> OnDone)
{
	SendGET(TEXT("/health"), [OnDone](bool bOK, const FString& Body, const FString& Err)
	{
		if (!bOK)
		{
			UE_LOG(LogTemp, Warning, TEXT("Health check failed: %s | body=%s"), *Err, *Body);
			OnDone(false);
			return;
		}
		const bool bOkFlag = Body.Contains(TEXT("\"ok\":true"));
		OnDone(bOkFlag);
	});
}

// ----------- LOGIN -----------

void UAuthApiClientSubsystem::RequestLoginByLogin(const FString& InLogin, const FString& InDescription, FOnLoginRequested OnDone)
{
	const FString Login = InLogin.TrimStartAndEnd();

	UE_LOG(LogTemp, Log, TEXT("RequestLoginByLogin('%s')"), *Login);

	if (Login.IsEmpty())
	{
		OnDone.ExecuteIfBound(false, TEXT(""), TEXT("Empty login (client)"));
		return;
	}

	auto Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("login"), Login);
	Root->SetStringField(TEXT("description"), InDescription);

	SendJsonPOST(TEXT("/auth/request_login"), Root,
		[OnDone](bool bOK, const FString& Body, const FString& Err)
	{
		if (!bOK)
		{
			UE_LOG(LogTemp, Warning, TEXT("RequestLoginByLogin failed: %s | body=%s"), *Err, *Body);
			OnDone.ExecuteIfBound(false, TEXT(""), Err);
			return;
		}

		TSharedPtr<FJsonObject> Obj; bool bParseOK=false;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
		if (FJsonSerializer::Deserialize(Reader, Obj) && Obj.IsValid()) bParseOK = true;

		if (!bParseOK)
		{
			OnDone.ExecuteIfBound(false, TEXT(""), TEXT("JSON parse error"));
			return;
		}

		const FString RequestId = Obj->GetStringField(TEXT("request_id"));
		OnDone.ExecuteIfBound(true, RequestId, TEXT(""));
	});
}

void UAuthApiClientSubsystem::PollLoginStatus(const FString& InRequestId, FOnLoginApproved OnDone, float TimeoutSeconds)
{
	if (!GetWorld())
	{
		OnDone.ExecuteIfBound(false, TEXT(""), TEXT("No World"));
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(LoginPollTimer);

	const double StartTime = FPlatformTime::Seconds();
	TWeakObjectPtr<UAuthApiClientSubsystem> WeakThis(this);

	FTimerDelegate Tick;
	Tick.BindLambda([this, WeakThis, InRequestId, OnDone, StartTime, TimeoutSeconds]()
	{
		if (!WeakThis.IsValid() || !GetWorld()) return;

		const FString Path = FString::Printf(TEXT("/auth/status?request_id=%s"), *InRequestId);
		SendGET(Path, [this, WeakThis, InRequestId, OnDone, StartTime, TimeoutSeconds]
		(bool bOK, const FString& Body, const FString& Err)
		{
			if (!WeakThis.IsValid() || !GetWorld()) return;

			if (!bOK)
			{
				if (FPlatformTime::Seconds() - StartTime >= TimeoutSeconds)
				{
					GetWorld()->GetTimerManager().ClearTimer(LoginPollTimer);
					OnDone.ExecuteIfBound(false, TEXT(""), Err.IsEmpty()?TEXT("Login timeout"):Err);
				}
				return;
			}

			TSharedPtr<FJsonObject> Obj; bool bParseOK=false;
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
			if (FJsonSerializer::Deserialize(Reader, Obj) && Obj.IsValid()) bParseOK = true;

			if (!bParseOK)
			{
				if (FPlatformTime::Seconds() - StartTime >= TimeoutSeconds)
				{
					GetWorld()->GetTimerManager().ClearTimer(LoginPollTimer);
					OnDone.ExecuteIfBound(false, TEXT(""), TEXT("JSON parse error"));
				}
				return;
			}

			const FString Status = Obj->GetStringField(TEXT("status"));

			if (Status.Equals(TEXT("approved")))
			{
				GetWorld()->GetTimerManager().ClearTimer(LoginPollTimer);
				const FString SessionToken = Obj->HasField(TEXT("session_token"))
					? Obj->GetStringField(TEXT("session_token"))
					: TEXT("");
				OnDone.ExecuteIfBound(true, SessionToken, TEXT(""));
			}
			else if (Status.Equals(TEXT("declined")) || Status.Equals(TEXT("already_registered")))
			{
				GetWorld()->GetTimerManager().ClearTimer(LoginPollTimer);
				OnDone.ExecuteIfBound(false, TEXT(""), FString::Printf(TEXT("Login %s"), *Status));
			}
			else
			{
				// pending
				if (FPlatformTime::Seconds() - StartTime >= TimeoutSeconds)
				{
					GetWorld()->GetTimerManager().ClearTimer(LoginPollTimer);
					OnDone.ExecuteIfBound(false, TEXT(""), TEXT("Login timeout"));
				}
			}
		});
	});

	GetWorld()->GetTimerManager().SetTimer(LoginPollTimer, Tick, PollIntervalSeconds, true);
}

// ----------- SHOP -----------

void UAuthApiClientSubsystem::GetItemPrice(const FString& InItemId, int32 InQuantity, FOnPrice OnDone)
{
	const int32 Qty = FMath::Max(1, InQuantity);
	const FString EncodedId = FGenericPlatformHttp::UrlEncode(InItemId);
	const FString Path = FString::Printf(TEXT("/shop/price?item_id=%s&qty=%d"), *EncodedId, Qty);

	SendGET(Path, [OnDone, InItemId](bool bOK, const FString& Body, const FString& Err)
	{
		if (!bOK)
		{
			UE_LOG(LogTemp, Warning, TEXT("GetItemPrice failed: %s | body=%s"), *Err, *Body);
			OnDone.ExecuteIfBound(false, 0, 0, InItemId, Err);
			return;
		}

		TSharedPtr<FJsonObject> Obj; bool bParseOK=false;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
		if (FJsonSerializer::Deserialize(Reader, Obj) && Obj.IsValid()) bParseOK = true;

		if (!bParseOK)
		{
			OnDone.ExecuteIfBound(false, 0, 0, InItemId, TEXT("JSON parse error"));
			return;
		}

		const int32 Unit = Obj->GetIntegerField(TEXT("unit_price"));
		const int32 Total = Obj->GetIntegerField(TEXT("total_price"));
		OnDone.ExecuteIfBound(true, Unit, Total, InItemId, TEXT(""));
	});
}

void UAuthApiClientSubsystem::PurchaseRequest(const FString& InSessionToken, const FString& InItemId, int32 InQuantity, FOnPurchaseRequested OnDone)
{
	const int32 Qty = FMath::Max(1, InQuantity);

	auto Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("item_id"), InItemId);
	Root->SetNumberField(TEXT("quantity"), Qty);

	TMap<FString,FString> Headers;
	Headers.Add(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *InSessionToken));

	SendJsonPOST(TEXT("/shop/purchase/request"), Root,
		[OnDone](bool bOK, const FString& Body, const FString& Err)
	{
		if (!bOK)
		{
			UE_LOG(LogTemp, Warning, TEXT("PurchaseRequest failed: %s | body=%s"), *Err, *Body);
			OnDone.ExecuteIfBound(false, TEXT(""), Err);
			return;
		}

		TSharedPtr<FJsonObject> Obj; bool bParseOK=false;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
		if (FJsonSerializer::Deserialize(Reader, Obj) && Obj.IsValid()) bParseOK = true;

		if (!bParseOK)
		{
			OnDone.ExecuteIfBound(false, TEXT(""), TEXT("JSON parse error"));
			return;
		}

		const FString RequestId = Obj->GetStringField(TEXT("request_id"));
		OnDone.ExecuteIfBound(true, RequestId, TEXT(""));
	},
	Headers);
}

void UAuthApiClientSubsystem::PollPurchaseStatus(const FString& InRequestId, FOnPurchaseStatus OnDone, float TimeoutSeconds)
{
	if (!GetWorld())
	{
		OnDone.ExecuteIfBound(false, TEXT("unknown"), TEXT(""), TEXT("No World"));
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(PurchasePollTimer);

	const double StartTime = FPlatformTime::Seconds();
	TWeakObjectPtr<UAuthApiClientSubsystem> WeakThis(this);

	FTimerDelegate Tick;
	Tick.BindLambda([this, WeakThis, InRequestId, OnDone, StartTime, TimeoutSeconds]()
	{
		if (!WeakThis.IsValid() || !GetWorld()) return;

		const FString Path = FString::Printf(TEXT("/shop/purchase/status?request_id=%s"), *InRequestId);
		SendGET(Path, [this, WeakThis, InRequestId, OnDone, StartTime, TimeoutSeconds]
		(bool bOK, const FString& Body, const FString& Err)
		{
			if (!WeakThis.IsValid() || !GetWorld()) return;

			if (!bOK)
			{
				if (FPlatformTime::Seconds() - StartTime >= TimeoutSeconds)
				{
					GetWorld()->GetTimerManager().ClearTimer(PurchasePollTimer);
					OnDone.ExecuteIfBound(false, TEXT("unknown"), TEXT(""), Err.IsEmpty()?TEXT("Purchase timeout"):Err);
				}
				return;
			}

			TSharedPtr<FJsonObject> Obj; bool bParseOK=false;
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
			if (FJsonSerializer::Deserialize(Reader, Obj) && Obj.IsValid()) bParseOK = true;

			if (!bParseOK)
			{
				if (FPlatformTime::Seconds() - StartTime >= TimeoutSeconds)
				{
					GetWorld()->GetTimerManager().ClearTimer(PurchasePollTimer);
					OnDone.ExecuteIfBound(false, TEXT("unknown"), TEXT(""), TEXT("JSON parse error"));
				}
				return;
			}

			const FString Status = Obj->GetStringField(TEXT("status"));

			FString HumanInfo;
			if (Obj->HasField(TEXT("item_id")))
			{
				const FString ItemId = Obj->GetStringField(TEXT("item_id"));
				const int32 Qty = Obj->GetIntegerField(TEXT("quantity"));
				const int32 Total = Obj->GetIntegerField(TEXT("total_price"));
				HumanInfo = FString::Printf(TEXT("Item: %s x%d, Total: %d"), *ItemId, Qty, Total);
			}

			if (Status.Equals(TEXT("approved")) || Status.Equals(TEXT("declined")))
			{
				GetWorld()->GetTimerManager().ClearTimer(PurchasePollTimer);
				OnDone.ExecuteIfBound(true, Status, HumanInfo, TEXT(""));
			}
			else
			{
				// pending
				if (FPlatformTime::Seconds() - StartTime >= TimeoutSeconds)
				{
					GetWorld()->GetTimerManager().ClearTimer(PurchasePollTimer);
					OnDone.ExecuteIfBound(false, TEXT("timeout"), HumanInfo, TEXT("Purchase timeout"));
				}
			}
		});
	});

	GetWorld()->GetTimerManager().SetTimer(PurchasePollTimer, Tick, PollIntervalSeconds, true);
}

// ----------- Users / Inventory -----------

void UAuthApiClientSubsystem::GetUserByLogin(const FString& InLogin, FOnUserByLogin OnDone)
{
	const FString Login = InLogin.TrimStartAndEnd();
	if (Login.IsEmpty())
	{
		OnDone.ExecuteIfBound(false, TEXT(""), 0, false, 0, TEXT("Empty login"));
		return;
	}

	const FString Enc = FGenericPlatformHttp::UrlEncode(Login);
	const FString Path = FString::Printf(TEXT("/users/by_login?login=%s"), *Enc);

	SendGET(Path, [OnDone](bool bOK, const FString& Body, const FString& Err)
	{
		if (!bOK)
		{
			OnDone.ExecuteIfBound(false, TEXT(""), 0, false, 0, Err);
			return;
		}

		TSharedPtr<FJsonObject> Obj;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
		if (!FJsonSerializer::Deserialize(Reader, Obj) || !Obj.IsValid())
		{
			OnDone.ExecuteIfBound(false, TEXT(""), 0, false, 0, TEXT("JSON parse error"));
			return;
		}

		const FString LoginOut   = Obj->GetStringField(TEXT("login"));
		const int64   ChatIdOut  = static_cast<int64>(Obj->GetNumberField(TEXT("telegram_chat_id")));
		const bool    Verified   = Obj->GetBoolField(TEXT("verified"));
		const int64   Gold       = static_cast<int64>(Obj->GetNumberField(TEXT("gold")));

		OnDone.ExecuteIfBound(true, LoginOut, ChatIdOut, Verified, Gold, TEXT(""));
	});
}

void UAuthApiClientSubsystem::GetInventoryByChatId(int64 ChatId, FOnInventory OnDone)
{
	if (ChatId <= 0)
	{
		OnDone.ExecuteIfBound(false, 0, {}, TEXT("Bad chat id"));
		return;
	}

	const FString Path = FString::Printf(TEXT("/users/%lld/inventory"), ChatId);

	SendGET(Path, [OnDone](bool bOK, const FString& Body, const FString& Err)
	{
		if (!bOK)
		{
			OnDone.ExecuteIfBound(false, 0, {}, Err);
			return;
		}

		TSharedPtr<FJsonObject> Obj;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
		if (!FJsonSerializer::Deserialize(Reader, Obj) || !Obj.IsValid())
		{
			OnDone.ExecuteIfBound(false, 0, {}, TEXT("JSON parse error"));
			return;
		}

		const int64 Gold = static_cast<int64>(Obj->GetNumberField(TEXT("gold")));

		TArray<FServerInventoryItem> Items;
		const TArray<TSharedPtr<FJsonValue>>* JsonItemsPtr = nullptr;
		if (Obj->TryGetArrayField(TEXT("items"), JsonItemsPtr) && JsonItemsPtr)
		{
			for (const TSharedPtr<FJsonValue>& V : *JsonItemsPtr)
			{
				const TSharedPtr<FJsonObject> ItObj = V->AsObject();
				if (!ItObj.IsValid()) continue;

				FServerInventoryItem It;
				It.Id = ItObj->GetStringField(TEXT("id"));
				It.Name = ItObj->GetStringField(TEXT("name"));
				It.Quantity = ItObj->GetIntegerField(TEXT("quantity"));
				Items.Add(MoveTemp(It));
			}
		}

		OnDone.ExecuteIfBound(true, Gold, MoveTemp(Items), TEXT(""));
	});
}

void UAuthApiClientSubsystem::GetInventoryByLogin(const FString& InLogin, FOnInventory OnDone)
{
	GetUserByLogin(InLogin, FOnUserByLogin::CreateLambda(
		[this, OnDone](bool bOK, FString /*Login*/, int64 ChatId, bool /*Verified*/, int64 /*Gold*/, FString Err)
	{
		if (!bOK)
		{
			OnDone.ExecuteIfBound(false, 0, {}, Err);
			return;
		}
		this->GetInventoryByChatId(ChatId, OnDone);
	}));
}

/////////////////////////////////////////////////////
// HTTP helpers

void UAuthApiClientSubsystem::SendJsonPOST(const FString& UrlPath, const TSharedRef<FJsonObject>& JsonBody,
	TFunction<void(bool, const FString&, const FString&)> OnDone,
	const TMap<FString, FString>& ExtraHeaders)
{
	FString BodyString;
	{
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
		FJsonSerializer::Serialize(JsonBody, Writer);
	}

	const FString Url = MakeUrl(UrlPath);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	for (const auto& Kvp : ExtraHeaders)
	{
		Request->SetHeader(Kvp.Key, Kvp.Value);
	}
	Request->SetContentAsString(BodyString);

	Request->OnProcessRequestComplete().BindLambda(
		[OnDone, Url](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSucceeded)
	{
		if (!bSucceeded || !Resp.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("HTTP POST failed: %s | no response"), *Url);
			OnDone(false, TEXT(""), TEXT("HTTP error (no response)"));
			return;
		}

		const int32 Code = Resp->GetResponseCode();
		const FString RespBody = Resp->GetContentAsString();
		UE_LOG(LogTemp, Verbose, TEXT("HTTP %d %s -> %s"), Code, *Url, *RespBody);

		if (Code >= 200 && Code < 300)
		{
			OnDone(true, RespBody, TEXT(""));
		}
		else
		{
			OnDone(false, RespBody, FString::Printf(TEXT("HTTP %d"), Code));
		}
	});
	Request->ProcessRequest();
}

void UAuthApiClientSubsystem::SendGET(const FString& UrlPathWithQuery,
	TFunction<void(bool, const FString&, const FString&)> OnDone,
	const TMap<FString, FString>& ExtraHeaders)
{
	const FString Url = MakeUrl(UrlPathWithQuery);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("GET"));
	for (const auto& Kvp : ExtraHeaders)
	{
		Request->SetHeader(Kvp.Key, Kvp.Value);
	}

	Request->OnProcessRequestComplete().BindLambda(
		[OnDone, Url](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSucceeded)
	{
		if (!bSucceeded || !Resp.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("HTTP GET failed: %s | no response"), *Url);
			OnDone(false, TEXT(""), TEXT("HTTP error (no response)"));
			return;
		}

		const int32 Code = Resp->GetResponseCode();
		const FString RespBody = Resp->GetContentAsString();
		UE_LOG(LogTemp, Verbose, TEXT("HTTP %d %s -> %s"), Code, *Url, *RespBody);

		if (Code >= 200 && Code < 300)
		{
			OnDone(true, RespBody, TEXT(""));
		}
		else
		{
			OnDone(false, RespBody, FString::Printf(TEXT("HTTP %d"), Code));
		}
	});
	Request->ProcessRequest();
}
