// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "Components/DrsWidgetInteractionComponent.h"
#include "PointerWidgetInteractionComponent.generated.h"


/**
 * Base class for pointer-based widget interaction components.
 */	
UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = (Interaction))
class DRSUSERINTERFACE_API UPointerWidgetInteractionComponent : public UDrsWidgetInteractionComponent
{
	GENERATED_BODY()

protected:

	// Engine
	virtual void BeginPlay() override;
	
	// IDrsWidgetInteractionInterface
	virtual TArray<FKey> GetSelectInputKeys() const override { return SelectInputKeys; }
	
	// UCoreWidgetInteractionComponent
	virtual void AddInputActions(UDrsEnhancedInputComponent* inputComponent) override final;
	virtual bool CanDrag() const override final;
	virtual bool CanHover_Implementation(const UWidgetComponent* widgetComponent) const override;
	virtual FHitResult CreateCustomHitResult() const override;
	
private:
	
	TArray<FKey> SelectInputKeys;
};
