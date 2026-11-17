// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "Components/ScrollBox.h"
#include "Interfaces/SlateLifecycleAware.h"
#include "Utils/WidgetUtils.h"
#include "CoreScrollBox.generated.h"

/**
 * Core extension of the native 'ScrollBox' widget class, adding support for Blueprint nodes that require a world context.
 */
UCLASS()
class DRSUSERINTERFACE_API UCoreScrollBox : public UScrollBox, public ISlateLifecycleAware
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
