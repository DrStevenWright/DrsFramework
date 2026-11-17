// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "Components/TreeView.h"
#include "Interfaces/DrsListViewInterface.h"
#include "CoreTreeView.generated.h"

/**
 * Extension of TreeView widget that adds missing accessors needed by Core.
 */
UCLASS()
class DRSUSERINTERFACE_API UCoreTreeView : public UTreeView, public IDrsListViewInterface
{
	GENERATED_BODY()

protected:

	// Since access is not provided by the base class.
	virtual float GetWheelScrollMultiplier() const override { return WheelScrollMultiplier; }
	virtual bool IsDragScrollingEnabled() const override { return bEnableRightClickScrolling; }
};

