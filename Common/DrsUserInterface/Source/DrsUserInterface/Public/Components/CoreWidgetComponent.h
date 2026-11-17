// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "CoreWidgetComponent.generated.h"


/**
 * Different supported widget interaction types.
 */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EWidgetInteractionType : uint8
{
	None = 0 UMETA(Hidden),
	Touch = (1 << 0),
	Pointer = (1 << 1),

	All = Touch | Pointer UMETA(Hidden)
};
ENUM_CLASS_FLAGS(EWidgetInteractionType)


/**
 * Base class for widget components with Core extensions.
 */
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = (Interaction))
class DRSUSERINTERFACE_API UCoreWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:

	UCoreWidgetComponent() {}
	
	void SetSupportsInteractionType(EWidgetInteractionType type, bool isSupported)
	{
		if (isSupported)
		{
			SupportedInteractionTypes |= static_cast<int32>(type);
		}
		else
		{
			SupportedInteractionTypes &= ~static_cast<int32>(type);
		}
	}
	
	void SetSupportedInteractionTypes(int32 types)					{ SupportedInteractionTypes = types; }
	void SetSupportedInteractionTypes(EWidgetInteractionType type)	{ SupportedInteractionTypes = static_cast<int32>(type); }
	bool SupportsInteractionType(EWidgetInteractionType type) const	{ return (SupportedInteractionTypes & static_cast<int32>(type)) != 0; }
	bool CanTouchInteract() const									{ return SupportsInteractionType(EWidgetInteractionType::Touch); }
	bool CanPointerInteract() const									{ return SupportsInteractionType(EWidgetInteractionType::Pointer); }
	
	bool IsTouchResistanceEnabled() const							{ return bEnableTouchResistance; }
	float GetTouchResistanceErrorTolerance() const					{ return TouchResistanceErrorTolerance; }
	
protected:
	
	// Types of interaction allowed for this widget.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = "/Script/UserInterface.EWidgetInteractionType"), Category = "Core")
	int32 SupportedInteractionTypes = static_cast<int32>(EWidgetInteractionType::All);

	// Touch resistance clamps an interactor's visual representation (e.g. hand mesh or stylus) against a widget surface,
	// so that contact gives some feeling of resistance rather than letting the interactor silently pass right through.
	// This property allows touch resistance to be disabled for individual widgets.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Core|Touch Resistance")
	bool bEnableTouchResistance = true;

	// Maximum deviation (in cms) permitted between the interactor's motion-tracked location and one that has been
	// constrained against this widget's surface, beyond which the clamp is released. Larger widgets may want more
	// generous tolerance, for example. (0.0 disables tolerance checks. -1.0 falls back to the interactor's own default,
	// and from there to the project settings.)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Core|Touch Resistance", meta = (EditCondition = "bEnableTouchResistance", ClampMin = "-1", Units = "cm"))
	float TouchResistanceErrorTolerance = -1.f;
};
