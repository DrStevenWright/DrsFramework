// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "Components/TileView.h"
#include "Interfaces/DrsListViewInterface.h"
#include "CoreTileView.generated.h"

/**
 * Extension of TileView widget that adds missing accessors needed by Core.
 */
UCLASS()
class DRSUSERINTERFACE_API UCoreTileView : public UTileView, public IDrsListViewInterface
{
	GENERATED_BODY()

protected:

	// Since access is not provided by the base class.
	virtual float GetWheelScrollMultiplier() const override { return WheelScrollMultiplier; }
	virtual bool IsDragScrollingEnabled() const override { return bEnableRightClickScrolling; }
};
