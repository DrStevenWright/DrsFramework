// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "Components/TextBlock.h"
#include "Utils/WidgetUtils.h"
#include "CoreTextBlock.generated.h"

/**
 * Core extension of the native 'TextBlock' widget class, adding support for Blueprint nodes that require a world context.
 */
UCLASS(meta=(DisplayName="CoreText"))
class DRSUSERINTERFACE_API UCoreTextBlock : public UTextBlock
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintCallable, Category = "Appearance")
	void SetLineHeightPercentage(float value)
	{
		LineHeightPercentage = value;
	}
	
	FORCEINLINE virtual UWorld* GetWorld() const override
	{
		return WidgetUtils::GetWorld(GetOuter());
	}
};

