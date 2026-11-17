// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "Components/DrsWidgetInteractionComponent.h"
#include "Interfaces/ConstrainedComponentInterface.h"
#include "Interfaces/SurfaceConstraintActuatorInterface.h"
#include "TouchWidgetInteractionComponent.generated.h"


/**
 * Base class for touch-based widget interaction components.
 */	
UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = (Interaction))
class DRSUSERINTERFACE_API UTouchWidgetInteractionComponent
	: public UDrsWidgetInteractionComponent
	, public IConstrainedComponentInterface
{
	GENERATED_BODY()

public:

	UTouchWidgetInteractionComponent(const FObjectInitializer& objectInitializer);

protected:

	// Engine
	virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;
#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* property) const override;
#endif

	// IValidateComponentInterface
	virtual void OnComponentValidate() override;

	// IConstrainedComponentInterface
	virtual FVector GetConstrainedLocation() const override { return GetComponentLocation(); }
	
	// UCoreWidgetInteractionComponent
	virtual void OnBindingChanged() override;
	virtual void AddInputActions(UDrsEnhancedInputComponent* inputComponent) override final;
	virtual void UpdateSelectedState() override final;
	virtual void OnResetState() override;
	virtual bool CanDrag() const override final;
	virtual bool CanHover_Implementation(const UWidgetComponent* widgetComponent) const override;
	virtual FHitResult CreateCustomHitResult() const override final;
	virtual void DrawDebugGeometry(const UWorld* world, const FWidgetInteractionDebugInfo& debugInfo) const override;
	virtual void AppendDebugText(FString& outText) const override;
	virtual float GetInteractionRadius() const override final { return TouchRadius; }
	
	// UTouchWidgetInteractionComponent
	virtual void OnTouchResistanceChanged(bool isResisting) {}
	
protected:

	//-------------------------------------------------------------------------
	// Properties
	//-------------------------------------------------------------------------
	
	// Register hovering while the component is above this distance from the surface.
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Widget Interaction|Touch", meta = (Units = "cm"))
	float MinHoverDistance = -5.f;
	
	// Register hovering while the component is below this distance from the surface.
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Widget Interaction|Touch", meta = (ClampMin = "0", Units = "cm"))
	float MaxHoverDistance = 10.f;

	// Register selection when the component is below this distance from the surface.
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Widget Interaction|Touch", meta = (ClampMin = "0", Units = "cm"))
	float MaxSelectDistance = 0.f;

	// Minimum hover distance the component must clear before another selection can be registered, to discourage
	// unintentional selections.
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Widget Interaction|Touch", meta = (ClampMin = "0", Units = "cm"))
	float MinClearDistance = 0.2f;

	// Optional buffer radius (in cms) around the interactor position. Where an interactor is better described by a rounded
	// tip than by a point, this is the radius of that tip. Touch distances and touch resistance are both measured from its
	// surface rather than from this component's origin, so that the tip rests against a widget instead of sinking into it.
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Widget Interaction|Touch", meta = (ClampMin = "0", Units = "cm"))
	float TouchRadius = 0.0f;

	// Touch resistance clamps an interactor's visual representation (e.g. hand mesh or stylus) against a widget surface,
	// so that contact gives some feeling of resistance rather than letting the interactor silently pass right through.
	// This property allows touch resistance to be disabled for individual widget interactors.
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Widget Interaction|Touch")
	bool bEnableTouchResistance = true;

	// Maximum deviation (in cms) permitted between this interactor's motion-tracked location and one that has been
	// constrained against a widget's surface, beyond which the clamp is released (0.0 disables tolerance checks.
	// -1.0 falls back to project defaults.)
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Widget Interaction|Touch", meta = (EditCondition = "bEnableTouchResistance", ClampMin = "-1", Units = "cm"))
	float TouchResistanceErrorTolerance = -1.f;

	// Identifies the widget component that is currently being hovered over by this component.
	UFUNCTION(BlueprintNativeEvent)
	UWidgetComponent* GetCustomHoveredWidgetComponent() const;
	virtual UWidgetComponent* GetCustomHoveredWidgetComponent_Implementation() const;

	// Whether a hover distance falls within the band this component treats as hovering.
	bool IsWithinHoverBand(float hoverDistance) const;
	
private:
	
	// Touch resistance
	void UpdateTouchResistance();
	void ClearTouchResistance();
	void SetIsTouchResisting(bool isResisting);
	bool BuildTouchResistanceConstraint(FSurfaceConstraint& outConstraint);
	
	// Handle to the bound interactor's surface constraint API, if it has one.
	UPROPERTY(Transient)
	TScriptInterface<ISurfaceConstraintActuatorInterface> SurfaceConstraintActuator = nullptr;
	
	// Cached results from GetCustomHoveredWidgetComponent() to save unnecessary recalculations during CreateCustomHitResult().
	// This is the closest point on the widget's COLLISION, which for a curved widget is not a point on its surface.
	mutable TWeakObjectPtr<UWidgetComponent> CachedClosestWidget = nullptr;
	mutable FVector CachedCollisionPoint = FVector::ZeroVector;

	bool bCanSelect = false;
	mutable bool bCanDrag = false;
	bool bIsTouchResisting = false; // Whether this component's visuals are currently clamped against the touched surface. 

};
