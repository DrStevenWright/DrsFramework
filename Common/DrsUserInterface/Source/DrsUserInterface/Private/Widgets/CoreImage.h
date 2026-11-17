// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "Components/Image.h"
#include "Interfaces/SlateLifecycleAware.h"
#include "Utils/WidgetUtils.h"
#include "CoreImage.generated.h"

/**
 * Core extension of the native 'Image' widget class, adding support for Blueprint nodes that require a world context.
 */
UCLASS()
class DRSUSERINTERFACE_API UCoreImage : public UImage, public ISlateLifecycleAware
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

