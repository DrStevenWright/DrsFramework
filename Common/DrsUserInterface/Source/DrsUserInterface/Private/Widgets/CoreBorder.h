// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "Components/Border.h"
#include "Interfaces/SlateLifecycleAware.h"
#include "Utils/WidgetUtils.h"
#include "CoreBorder.generated.h"

/**
 * Core extension of the native 'Border' widget class, adding support for Blueprint nodes that require a world context.
 */
UCLASS()
class DRSUSERINTERFACE_API UCoreBorder : public UBorder, public ISlateLifecycleAware
{
	GENERATED_BODY()

public:
	virtual void ReleaseSlateResources(bool bReleaseChildren) override
	{
		NotifySlateResourcesReleased(this);
		
		Super::ReleaseSlateResources(bReleaseChildren);
	}

	FORCEINLINE virtual UWorld* GetWorld() const override
	{
		return WidgetUtils::GetWorld(GetOuter());
	}
};

