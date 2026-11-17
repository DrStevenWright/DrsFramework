// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DrsWidgetInteractorInterface.generated.h"

class UDrsWidgetInteractionComponent;
enum class EControllerHand : uint8;

/**
 * API required from 'interactor' components that DrsWidgetInteractionComponents can be bound to.
 */
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UDrsWidgetInteractorInterface : public UInterface
{
	GENERATED_BODY()
};

class DRSUSERINTERFACE_API IDrsWidgetInteractorInterface
{
	GENERATED_BODY()

public:

	// Specifies whether this interactor is owned by a local pawn.
	virtual bool IsLocalWidgetInteractor() const { return false; }
	
	// Specifies which hand, if any, this interactor is associated with.
	virtual TOptional<EControllerHand> GetWidgetInteractorHand() const { return TOptional<EControllerHand>(); }

	// Specifies whether a given widget interaction component can be bound to this interactor.
	virtual bool AllowWidgetInteractorBinding(const UDrsWidgetInteractionComponent* widgetInteractionComponent) const { return true; }
};
