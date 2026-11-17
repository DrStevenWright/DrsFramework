// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "Components/Overlay.h"
#include "Utils/WidgetUtils.h"
#include "CoreOverlay.generated.h"

/**
 * Core extension of the native 'Overlay' widget class, adding support for Blueprint nodes that require a world context.
 */
UCLASS()
class DRSUSERINTERFACE_API UCoreOverlay : public UOverlay
{
	GENERATED_BODY()

public:
	
	FORCEINLINE virtual UWorld* GetWorld() const override
	{
		return WidgetUtils::GetWorld(GetOuter());
	}
};
