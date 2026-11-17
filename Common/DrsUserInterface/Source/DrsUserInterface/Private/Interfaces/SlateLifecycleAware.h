// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SlateLifecycleAware.generated.h"

UINTERFACE(BlueprintType)
class DRSUSERINTERFACE_API USlateLifecycleAware : public UInterface
{
	GENERATED_BODY()
};

/**
 * Generic interface for widgets that want to react to Slate lifecycle notifications.
 */
class DRSUSERINTERFACE_API ISlateLifecycleAware
{
	GENERATED_BODY()

public:
	/**
	 * Called when the widget is releasing its Slate resources.
	 * This can happen more than once during a widget's lifetime.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DRS|UI")
	void OnSlateResourcesReleased();

protected:
	static void NotifySlateResourcesReleased(UObject* object)
	{
		const bool canNotifyBlueprint =
			object &&
			!object->IsUnreachable() &&
			!object->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed) &&
			!GExitPurge;

		if (canNotifyBlueprint && object->GetClass()->ImplementsInterface(USlateLifecycleAware::StaticClass()))
		{
			Execute_OnSlateResourcesReleased(object);
		}
	}
};
