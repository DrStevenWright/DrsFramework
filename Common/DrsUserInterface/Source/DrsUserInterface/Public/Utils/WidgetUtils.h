// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

class UDrsWidgetInteractionComponent;
class UWidgetComponent;
class UWidget;

namespace WidgetUtils
{
	/**
	 * Helper function that provides a "GetWorld()" for widgets, which was removed in UE 5.2. We use this function,
	 * via some custom widget base classes, to allow nodes that require a world context to be used in Blueprints
	 * derived from widget classes.
	 */
	DRSUSERINTERFACE_API UWorld* GetWorld(UObject* outer); // SWTODO - check if this is still needed.
	
	DRSUSERINTERFACE_API FVector GetWidgetCenter(const UWidgetComponent* widgetComponent);
	DRSUSERINTERFACE_API FVector GetWidgetNormalAtPosition(const UWidgetComponent* widgetComponent, const FVector& position);
	DRSUSERINTERFACE_API bool IsPositionWithinWidgetInteractionBounds(const UWidgetComponent* widgetComponent, const FVector& position);
	DRSUSERINTERFACE_API bool IsPositionWithinPlanarWidgetBounds(const UWidgetComponent* widgetComponent, const FVector& position);
	DRSUSERINTERFACE_API bool IsPositionWithinCylindricalWidgetBounds(const UWidgetComponent* widgetComponent, const FVector& position);
	DRSUSERINTERFACE_API bool IsPositionWithinCylindricalWidgetAngleBounds(const UWidgetComponent* widgetComponent, const FVector& position);

	DRSUSERINTERFACE_API UDrsWidgetInteractionComponent* FindFocusedWidgetInteractionFromWidget(const UObject* worldContextObject, const UWidget* widget);
};
