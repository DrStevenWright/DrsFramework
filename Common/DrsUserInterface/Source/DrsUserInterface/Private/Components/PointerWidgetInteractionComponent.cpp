// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#include "Components/PointerWidgetInteractionComponent.h"

#include "Components/CoreWidgetComponent.h"
#include "CoreEnhancedInputComponent.h"
#include "CoreInputMappingContext.h"
#include "CoreInputSettings.h"
#include "CoreUserInterfaceEditorSettings.h"


void UPointerWidgetInteractionComponent::BeginPlay()
{
#if WITH_EDITOR
	if (GIsEditor && IsLocallyControlled() && UCoreUserInterfaceEditorSettings::Get().bDisableInertialScrollUsingPointer)
	{
		UE_LOG(LogWidgetInteraction, Log, TEXT("Disabling inertial scrolling for editor on Core WidgetInteraction Component '%s'."), *GetName());
		bIsInertialScrollingEnabled = false;
	}
#endif
	
	Super::BeginPlay();
}


void UPointerWidgetInteractionComponent::AddInputActions(UDrsEnhancedInputComponent* inputComponent)
{
	if (ensure(IsValid(inputComponent)) && InteractorHand.IsSet())
	{
		if (const UDrsInputMappingContext* context = UDrsInputSettings::Get().UIInteractionControls.GetContextForHand(InteractorHand.GetValue()))
		{
			inputComponent->BindActionID(context, InputActionID_Select, ETriggerEvent::Started, this, &UPointerWidgetInteractionComponent::OnSelect);
			inputComponent->BindActionID(context, InputActionID_Select, ETriggerEvent::Completed, this, &UPointerWidgetInteractionComponent::OnRelease);
			
			SelectInputKeys = context->FindKeysForActionID(InputActionID_Select);
		}

		if (const UDrsInputMappingContext* context = UDrsInputSettings::Get().UIScrollControls.GetContextForHand(InteractorHand.GetValue()))
		{
			inputComponent->BindActionID(context, InputActionID_Scroll_Vertical, ETriggerEvent::Triggered, this, &UPointerWidgetInteractionComponent::OnScrollVertical);
			inputComponent->BindActionID(context, InputActionID_Scroll_Horizontal, ETriggerEvent::Triggered, this, &UPointerWidgetInteractionComponent::OnScrollHorizontal);
		}
	}
}


bool UPointerWidgetInteractionComponent::CanDrag() const
{
	// Custom-dragging with a pointer requires the selection input to be held down.
	// Native dragging, meanwhile, requires the virtual RMB to be held down.
	return (UseCustomDragScrolling() ? bIsSelecting : bIsRightClicking) && Super::CanDrag(); 
}


bool UPointerWidgetInteractionComponent::CanHover_Implementation(const UWidgetComponent* widgetComponent) const
{
	if (const UCoreWidgetComponent* coreWidgetComponent = Cast<UCoreWidgetComponent>(widgetComponent))
	{
		if (!coreWidgetComponent->CanPointerInteract())
		{
			return false;
		}
	}
	return Super::CanHover_Implementation(widgetComponent);
}


FHitResult UPointerWidgetInteractionComponent::CreateCustomHitResult() const
{
	if (ensure(InteractionSource == EWidgetInteractionSource::Custom))
	{
		return CustomHitResult;
	}
	return Super::CreateCustomHitResult();
}

