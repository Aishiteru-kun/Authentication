#pragma once

#include "CoreMinimal.h"

#include "GlobalStructs.generated.h"

USTRUCT(BlueprintType)
struct FServerItemInfo : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	FString ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	TObjectPtr<UTexture2D> Icon;
};
