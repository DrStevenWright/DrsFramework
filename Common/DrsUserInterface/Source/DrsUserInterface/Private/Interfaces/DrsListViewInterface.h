// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DrsListViewInterface.generated.h"

UINTERFACE(MinimalAPI)
class UDrsListViewInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for plugging some gaps in the native ListView implementation.
 */
class DRSUSERINTERFACE_API IDrsListViewInterface
{
	GENERATED_BODY()

public:
	
	virtual float GetWheelScrollMultiplier() const = 0;

	virtual bool IsDragScrollingEnabled() const  = 0;
};
