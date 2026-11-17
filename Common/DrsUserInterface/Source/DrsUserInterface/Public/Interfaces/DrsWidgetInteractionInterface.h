// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DrsWidgetInteractionInterface.generated.h"

class UWidget;

UENUM()
enum class EAllowClickEvents : uint8
{
	Yes,
	No,
	WithMouseDeviationCheck
};

/**
 * Interface class for communicating between DRS widget interaction system and custom widget types.
 */
UINTERFACE(MinimalAPI)
class UDrsWidgetInteractionInterface : public UInterface
{
	GENERATED_BODY()
};

class DRSUSERINTERFACE_API IDrsWidgetInteractionInterface
{
	GENERATED_BODY()

public:
	
	// Returns whether a given widget is currently being hovered by this component.
	virtual bool IsHovering(const UWidget* widget) const { return false; }

	// Returns whether a given widget is currently being selected by this component.
	virtual bool IsSelecting(const UWidget* widget) const { return false; }

	// Returns whether button click events should be allowed by this component.
	virtual EAllowClickEvents AllowButtonClickEvents() const { return EAllowClickEvents::No; }
	
	// Returns the maximum amount that the mouse position has deviated since entering a pressed state.
	virtual float GetMouseDeviation() const { return 0.0f; }

	// Allows retrieval of the input keys used in current widget selection.
	virtual TArray<FKey> GetSelectInputKeys() const { return {}; }
};
