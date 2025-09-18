#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DataTable.h"
#include "GlobalStructs.h"            // FServerItemInfo (ItemId, Name, Icon)
#include "Interface/WidgetInterface.h"
#include "SendTrade.generated.h"

class UTextBlock;
class UButton;
class UScrollBox;
class UShopItemSlot;

UCLASS()
class AUTHENTICATION_API USendTrade : public UUserWidget, public IWidgetInterface
{
	GENERATED_BODY()

public:
	/** Викликаєш перед відкриттям трейд-вікна. PersonLogin = логін іншого гравця */
	UFUNCTION(BlueprintCallable)
	void OnSelectedPerson(const FString& PersonLogin);

	// IWidgetInterface
	virtual void OnActivate_Implementation() override;
	virtual void OnDeactivate_Implementation() override;

protected:
	// --------- BindWidget (зв'яжи в дизайнері) ---------
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UScrollBox> YourInventoryPanel = nullptr;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UScrollBox> PersonInventoryPanel = nullptr;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UScrollBox> OfferBox = nullptr;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UScrollBox> RequestBox = nullptr;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> OfferPlusOneButton = nullptr;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> OfferMinusOneButton = nullptr;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> OfferRemoveButton = nullptr;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> RequestPlusOneButton = nullptr;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> RequestMinusOneButton = nullptr;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> RequestRemoveButton = nullptr;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> SendTradeButton = nullptr;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> CancelButton = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ErrorMsg = nullptr;

	// --------- Налаштування ---------
	/** Слот для рендеру елементів інвентаря/вибраних предметів */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade")
	TSubclassOf<UShopItemSlot> InventoryItemSlotWidgetClass;

	/** Таблиця для Name <-> ItemId <-> Icon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade")
	TObjectPtr<UDataTable> ItemDataTable = nullptr;

protected:
	// --------- Handlers ---------
	UFUNCTION()
	void OnOfferPlusOneClicked();
	UFUNCTION()
	void OnOfferMinusOneClicked();
	UFUNCTION()
	void OnOfferRemoveClicked();

	UFUNCTION()
	void OnRequestPlusOneClicked();
	UFUNCTION()
	void OnRequestMinusOneClicked();
	UFUNCTION()
	void OnRequestRemoveClicked();

	UFUNCTION()
	void OnSendTradeClicked();
	UFUNCTION()
	void OnCancelClicked();

	// Клік по предмету в інвентарі (додаємо у пропозицію/запит)
	UFUNCTION()
	void OnYourInventoryItemSelected(UShopItemSlot* ItemSlot);
	UFUNCTION()
	void OnPersonInventoryItemSelected(UShopItemSlot* ItemSlot);

	// Клік по вже доданому в Offer / Request (виділення для +/–/Remove)
	UFUNCTION()
	void OnSelectedOfferItem(UShopItemSlot* ItemSlot);
	UFUNCTION()
	void OnSelectedRequestItem(UShopItemSlot* ItemSlot);

protected:
	// --------- Внутрішній стан ---------
	/** Логін цілі (те, що прийшло у OnSelectedPerson) */
	FString TargetLogin;

	/** ChatId цілі (для TradeCreate) */
	int64 TargetChatId = 0;

	/** Наш логін і токен із PlayerState */
	FString MyLogin;
	FString MySessionToken;

	/** Мапа «назва → доступна кількість» у нас і в цілі (для інформації) */
	TMap<FString,int32> MyInventoryCounts;
	TMap<FString,int32> PersonInventoryCounts;

	/** Що ми пропонуємо і що просимо: «назва → кількість» */
	TMap<FString,int32> OfferMap;
	TMap<FString,int32> RequestMap;

	/** Поточно виділені елементи у списках Offer/Request */
	TWeakObjectPtr<UShopItemSlot> WeakSelectedOfferItem;
	TWeakObjectPtr<UShopItemSlot> WeakSelectedRequestItem;

protected:
	// --------- Допоміжні ---------
	virtual void NativeConstruct() override;

	void RefreshUI();
	void ClearAllLists();

	void BuildYourInventoryUI();
	void BuildPersonInventoryUI(const TArray<struct FServerInventoryItem>& Items);

	/** Додати/змінити позицію в Offer/Request за назвою (із коректним апдейтом віджетів) */
	void AddOrIncOfferByName(const FString& ItemName);
	void AddOrIncRequestByName(const FString& ItemName);
	void ChangeQtyInBox(UScrollBox* Box, const FString& ItemName, int32 NewQty);

	/** Побудувати/знайти віджет у Box за назвою */
	UShopItemSlot* FindItemSlotInBox(UScrollBox* Box, const FString& ItemName) const;
	UShopItemSlot* AddItemToBox(UScrollBox* Box, const FString& ItemName, int32 StartQty, bool bBindToOffer);

	/** Допоміжне: знайти ItemId/Row по імені */
	const FServerItemInfo* FindRowByName(const FString& Name) const;
	const FServerItemInfo* FindRowById(const FString& ItemId) const;

	/** Показ помилки (опціональний текстовий блок) */
	void ShowError(const FString& Text);

	/** Запит інвентарів через сабсистему */
	void LoadMyInventory();
	void LoadTargetInventoryAndChatId();

	/** Збирання масивів рядків для API (повторення id за кількістю) */
	void BuildTradeArrays(TArray<FString>& OutOfferIds, TArray<FString>& OutRequestIds) const;
};
