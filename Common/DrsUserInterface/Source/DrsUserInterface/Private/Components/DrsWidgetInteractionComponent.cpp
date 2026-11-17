// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "Components/DrsWidgetInteractionComponent.h"

#include "BaseDelegates.h"
#include "BaseMacros.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ListView.h"
#include "Components/ScrollBox.h"
#include "Components/WidgetComponent.h"
#include "DrsEnhancedInputComponent.h"
#include "DrsInputSettings.h"
#include "DrsInputUtils.h"
#include "Components/WidgetInteractionDebug.h"
#include "Engine/World.h"
#include "Framework/Application/SlateUser.h"
// #include "HapticsFunctionLibrary.h" // SWTODO
#include "Interfaces/DrsListViewInterface.h"
#include "Interfaces/DrsWidgetInteractorInterface.h"
#include "UserInterfaceSubsystem.h"
#include "EnumToString.h"
#include "Utils/WidgetUtils.h"
#include "Widgets/Buttons/CoreButton.h"
#include "Widgets/ScrollBox/SScrollBox2D.h"

//-----------------------------------------------------------
// Logging
//-----------------------------------------------------------
DEFINE_LOG_CATEGORY(LogWidgetInteraction);

//-----------------------------------------------------------
// Constants
//-----------------------------------------------------------
namespace
{
	const TMap<FName, EInteractableWidgetType> SelectableWidgetLookup =
	{
		{"SButton",                             EInteractableWidgetType::Button},
		{"SSlider",                             EInteractableWidgetType::Slider},
		{"SRadialSlider",                       EInteractableWidgetType::RadialSlider},
		{"SScrollBar",                          EInteractableWidgetType::ScrollBar},
		{"SEditableText",                       EInteractableWidgetType::TextBox},
		{"SMultiLineEditableText",              EInteractableWidgetType::TextBox},
		{"SCoreEditableTextBox",                EInteractableWidgetType::TextBox},
		{"SCoreMultiLineEditableTextBox",       EInteractableWidgetType::TextBox},
	};

	// FIXME: We need a more robust identification of templated widget types.
	const TMap<FName, EInteractableWidgetType> ScrollableWidgetLookup =
	{
		{"SScrollBox",                          EInteractableWidgetType::ScrollBox},
		{"SCappedScrollBox",                    EInteractableWidgetType::ScrollBox},
		{"SScrollBox2D",                        EInteractableWidgetType::ScrollBox2D},
		{"ListViewT<ItemType>",                 EInteractableWidgetType::ListView},
		{"TreeViewT<ItemType>",                 EInteractableWidgetType::ListView},
		{"TileViewT<ItemType>",                 EInteractableWidgetType::ListView},
		{"SComboBox<FName>",                    EInteractableWidgetType::ComboBox},
		{"SComboBox< TSharedPtr<FString> >",    EInteractableWidgetType::ComboBox},
	};

	const FName SObjectWidgetType("SObjectWidget");
	
	const FHitResult NoHit = FHitResult();

	const FName SuspendReason_Unbound("Unbound");
	const FName SuspendReason_RemoteUser("Remote");

	// From experimentation, this value seems to convert local-space mouse deltas to appropriate ScrollWheel input.
	constexpr float DragToScrollWheelFactor = 0.03125f;
	
	// Debug helpers
	FString ComponentPrintName(const UActorComponent* comp)
	{ 
		return comp ? FString::Printf(TEXT("%s::%s"), *comp->GetOwner()->GetName(), *comp->GetName()) : TEXT("None");
	}	
}


UDrsWidgetInteractionComponent::UDrsWidgetInteractionComponent(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer.DoNotCreateDefaultSubobject(TEXT("ArrowComponent0")))
{
	// UActorComponent
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bTickEvenWhenPaused = true;
	SetIsReplicatedByDefault(false);
	bAutoActivate = false;
	bWantsInitializeComponent = true;
}


void UDrsWidgetInteractionComponent::PreSave(FObjectPreSaveContext saveContext)
{
#if WITH_EDITOR
	DoPreSaveComponentValidation(saveContext);
#endif

	Super::PreSave(saveContext);
}


void UDrsWidgetInteractionComponent::InitializeComponent()
{
	Super::InitializeComponent();

	RUNTIME_ONLY_MARKER

	// Validate properties.
	DoRuntimeComponentValidation();
	
	if (IsActive())
	{
		UE_LOG(LogWidgetInteraction, Error, TEXT("Widget interaction component '%s' was auto-activated before being bound to a user."), *ComponentPrintName(this));
		Deactivate();
	}
	
	SetSuspended(true, SuspendReason_Unbound);
	SetSuspended(true, SuspendReason_RemoteUser);
	
	UpdateInteractionDebug();
}


void UDrsWidgetInteractionComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	SetShowInteractionDebug(false);

	Super::EndPlay(endPlayReason);
	
	Unbind();
	
	if (bPreserveVirtualUser)
	{
		UE_LOG(LogWidgetInteraction, Verbose, TEXT("Cleaning up widget interaction component '%s'%s."), *ComponentPrintName(this), VirtualUser.IsValid() ? TEXT(" and unregistering virtual user") : TEXT(""));
		Deactivate();
		ReleaseUserIndex();
	}
	
	ensure(!bVirtualUserClaimed);
}


void UDrsWidgetInteractionComponent::TickComponent(const float deltaTime, const ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UBaseWidgetInteractionComponent::TickComponent)

	// Sanity check that this component has not been detached from a user without unbinding it.
	if (!HasValidBinding())
	{
		UE_LOG(LogWidgetInteraction, Warning, TEXT("Widget interaction component '%s' has invalid binding to '%s'."), *ComponentPrintName(this), *ComponentPrintName(InteractorComponent));
		Unbind();
		return;
	}

	Super::TickComponent(deltaTime, tickType, thisTickFunction);
	
	// Check for stale widget handles.
	ConsolidateWidgetCaches();
	
	// Detect if we're hovering over an interactable widget.
	UpdateHoveredState();
	
	// Detect if we're selecting an interactable widget.
	UpdateSelectedState();
	
	// Detect dragging inputs and apply scrolling.
	UpdateDragging();
}


EAllowClickEvents UDrsWidgetInteractionComponent::AllowButtonClickEvents() const
{
	if (!bIsLeftClicking)
	{
		return EAllowClickEvents::No;
	}
	else
	{
		return IsDragging() ? EAllowClickEvents::WithMouseDeviationCheck : EAllowClickEvents::Yes;
	}
}


UWidgetInteractionComponent::FWidgetTraceResult UDrsWidgetInteractionComponent::PerformTrace() const
{
	return const_cast<UDrsWidgetInteractionComponent*>(this)->PerformTraceInternal();
}


UWidgetInteractionComponent::FWidgetTraceResult UDrsWidgetInteractionComponent::PerformTraceInternal()
{
	// For custom interaction sources, we need to create a custom hit result and supply it before the base PerformTrace
	// code is executed.
	if (InteractionSource == EWidgetInteractionSource::Custom)
	{
		SetCustomHitResult(CreateCustomHitResult());
	}

	// For custom interaction sources, this step simply processes the custom hit result supplied previously. For non-custom
	// sources, this step must first perform a line trace to obtain a hit result. The trace will be configured from this
	// component's position, forward direction and InteractionDistance property.
	FWidgetTraceResult result = Super::PerformTrace();

	// Cache some info about the hit result.
	const FVector componentPos = GetComponentLocation();
	HoverDistance		 = result.bWasHit ? FVector::Distance(componentPos, result.HitResult.ImpactPoint) : TNumericLimits<float>::Max();
	CurrentMousePosition = result.bWasHit ? result.HitResult.ImpactPoint : componentPos;
	MouseDeviation		 = bIsSelecting ? FMath::Max(MouseDeviation, FVector::Distance(MousePositionOnSelect, CurrentMousePosition)) : 0.0f;

	// Report degenerate hit results.
	UE_CLOG(result.bWasHit && !FMath::IsFinite(HoverDistance), LogWidgetInteraction, Warning,
		TEXT("Degenerate trace result on %s-hand interactor '%s' - location: %s, impact: %s, trace: [%s -> %s], widget: '%s', geometry: %s.")
		, *GetInteractorHandString()
		, *GetName()
		, *componentPos.ToString()
		, *result.HitResult.ImpactPoint.ToString()
		, *result.LineStartLocation.ToString()
		, *result.LineEndLocation.ToString()
		, *GetNameSafe(result.HitWidgetComponent)
		, result.HitWidgetComponent ? *RW1::EnumToString(result.HitWidgetComponent->GetGeometryMode()) : TEXT("unknown")
		);
	
	if (const USceneComponent* interactor = Cast<USceneComponent>(InteractorComponent))
	{
		if (result.bWasHit && result.HitWidgetComponent)
		{
			CurrentInteractorLocalPosition = result.HitWidgetComponent->GetComponentTransform().InverseTransformPositionNoScale(interactor->GetComponentLocation());
			CurrentInteractorLocalPosition.X = 0.0f;
			InteractorDeviation	= bIsSelecting ? FMath::Max(InteractorDeviation, FVector::Distance(InteractorLocalPositionOnSelect, CurrentInteractorLocalPosition)) : 0.0f;
		}
	}
	
	// Detect penetration through the widget surface (from touch interaction) and adjust the hover distance accordingly.
	if (result.bWasHit && result.HitWidgetComponent && ((result.HitResult.ImpactPoint - componentPos) | result.HitWidgetComponent->GetForwardVector()) > 0.0f)
	{
		HoverDistance = -HoverDistance;
	}
	
	// Report the hover distance from this interactor's outer surface rather than from its origin, so that the hover and
	// selection thresholds describe where the visible interactor meets the widget. Applied to the signed distance, and
	// necessarily before the filtering below, because CanHover() tests the result.
	if (result.bWasHit)
	{
		HoverDistance -= GetInteractionRadius();
	}
	
	// Filter the returned trace result.
	result.bWasHit &= CanHover(result.HitWidgetComponent);
	result.bWasHit &= !IsInteractionConflict(result.HitWidgetPath);
	
	return result.bWasHit ? result : FWidgetTraceResult();
}


FHitResult UDrsWidgetInteractionComponent::CreateCustomHitResult() const
{
	return NoHit;
}


bool UDrsWidgetInteractionComponent::CanBindTo(TScriptInterface<IDrsWidgetInteractorInterface> interactor) const
{
	return interactor && interactor.GetObject()->IsA<UActorComponent>() && interactor->AllowWidgetInteractorBinding(this);
}


void UDrsWidgetInteractionComponent::BindTo(TScriptInterface<IDrsWidgetInteractorInterface> interactor)
{
	if (IsBoundTo(interactor))
	{
		UE_LOG(LogWidgetInteraction, Verbose, TEXT("Widget interaction component '%s' is already bound to '%s'."), *ComponentPrintName(this), *ComponentPrintName(InteractorComponent));
		return;
	}

	// Remove any existing binding.
	if (IsBound())
	{
		Unbind();
	}
	
	// Validate the new binding.
	if (!CanBindTo(interactor))
	{
		UE_LOG(LogWidgetInteraction, Warning, TEXT("Disallowed binding of widget interaction component '%s' to '%s'."), *ComponentPrintName(this), *GetNameSafe(interactor.GetObject()));
		return;
	}
	
	// Cache binding info.
	if (UActorComponent* interactorComponent = CastChecked<UActorComponent>(interactor.GetObject()))
	{
		UE_LOG(LogWidgetInteraction, Log, TEXT("Binding widget interaction component '%s' to '%s'."), *ComponentPrintName(this), *ComponentPrintName(interactorComponent));
		
		InteractorBinding		= interactor;
		InteractorComponent		= interactorComponent;
		bIsLocallyControlled	= interactor->IsLocalWidgetInteractor();
		InteractorHand			= interactor->GetWidgetInteractorHand();
		
		AddTickPrerequisiteComponent(interactorComponent);
	}
	
	// Subscribe to delegates.
	if (bIsLocallyControlled)
	{
		check(InteractorComponent);
		if (APawn* interactorPawn = Cast<APawn>(InteractorComponent->GetOwner()))
		{
			OnSetupInputComponent(interactorPawn, interactorPawn->InputComponent); // TODO: Explore using a belated delegate for input setup.
			FBaseDelegates::UserSetupInputMessage.AddUObject(this, &UDrsWidgetInteractionComponent::OnSetupInputComponent);
		}
		OnHoveredWidgetChanged.AddDynamic(this, &UDrsWidgetInteractionComponent::OnHoveredWidgetComponentChanged);
	}

	// Register this interactor.
	if (UUserInterfaceSubsystem* subsystem = UUserInterfaceSubsystem::Get(this))
	{
		subsystem->RegisterWidgetInteractor(this);
	}
	
	// Update enabled state.
	SetSuspended(false, SuspendReason_Unbound);
	SetSuspended(!bIsLocallyControlled, SuspendReason_RemoteUser);
	
	// Notifications.
	if (IsBound())
	{
		OnBindingChanged();
		OnBind.Broadcast(interactor);
	}
}


void UDrsWidgetInteractionComponent::UnbindFrom(TScriptInterface<IDrsWidgetInteractorInterface> interactor)
{
	if (IsBoundTo(interactor))
	{
		Unbind();
	}
}


void UDrsWidgetInteractionComponent::Unbind()
{
	UE_CLOG(InteractorComponent, LogWidgetInteraction, Log, TEXT("Unbinding widget interaction component '%s' from '%s'."), *ComponentPrintName(this), *ComponentPrintName(InteractorComponent));
	
	// Update enabled state. (This must be done before clearing the binding info.)
	SetSuspended(true, SuspendReason_Unbound);
	SetSuspended(true, SuspendReason_RemoteUser);
	
	// Clear binding info.
	const TScriptInterface<IDrsWidgetInteractorInterface> previousBinding = InteractorBinding;
	if (InteractorComponent)
	{
		RemoveTickPrerequisiteComponent(InteractorComponent);
		
		InteractorBinding	= nullptr;
		InteractorComponent = nullptr;
		InteractorHand.Reset();
	}

	// Unsubscribe delegates.
	if (bIsLocallyControlled)
	{
		FBaseDelegates::UserSetupInputMessage.RemoveAll(this);
		OnHoveredWidgetChanged.RemoveAll(this);
		bIsLocallyControlled = false;
	}

	// Unregister this interactor.
	if (UUserInterfaceSubsystem* subsystem = UUserInterfaceSubsystem::Get(this))
	{
		subsystem->UnregisterWidgetInteractor(this);
	}
	
	// Clear any input actions that may have been bound.
	RemoveInputActions(CurrentInputComponent);
	CurrentInputComponent = nullptr;
	
	// Notifications.
	if (previousBinding)
	{
		OnBindingChanged();
		OnUnbind.Broadcast(previousBinding);
	}
}


bool UDrsWidgetInteractionComponent::IsBoundTo(TScriptInterface<IDrsWidgetInteractorInterface> interactor) const
{
	return InteractorBinding == interactor;
}


bool UDrsWidgetInteractionComponent::IsBoundToPawn(const APawn* pawn) const
{
	return (InteractorComponent && InteractorComponent->GetOwner() == pawn);
}


bool UDrsWidgetInteractionComponent::HasValidBinding() const
{
	return InteractorBinding && InteractorBinding->AllowWidgetInteractorBinding(this);
}


void UDrsWidgetInteractionComponent::ClaimUserIndex()
{
	if (!bVirtualUserClaimed)
	{
		if (UUserInterfaceSubsystem* subsystem = UUserInterfaceSubsystem::Get(this))
		{
			VirtualUserIndex = subsystem->ClaimVirtualUserIndex();
			PointerIndex = VirtualUserIndex;
		}

		UE_LOG(LogWidgetInteraction, Verbose, TEXT("Claimed user index '%d' and pointer index '%d' for %s widget interaction component '%s'."), VirtualUserIndex, PointerIndex, IsActive() ? TEXT("active") : TEXT("inactive"), *ComponentPrintName(this));
		bVirtualUserClaimed = true;
	}
}


void UDrsWidgetInteractionComponent::ReleaseUserIndex()
{
	if (bVirtualUserClaimed)
	{
		if (UUserInterfaceSubsystem* subsystem = UUserInterfaceSubsystem::Get(this))
		{
			subsystem->ReleaseVirtualUserIndex(VirtualUserIndex);
		}
		
		UE_LOG(LogWidgetInteraction, Verbose, TEXT("Released user index '%d' and pointer index '%d' for %s widget interaction component '%s'."), VirtualUserIndex, PointerIndex, IsActive() ? TEXT("active") : TEXT("inactive"), *ComponentPrintName(this));
		VirtualUserIndex = 0;
		PointerIndex = 0;
		bVirtualUserClaimed = false;
	}
}


void UDrsWidgetInteractionComponent::OnSetupInputComponent(const APawn* pawn, UInputComponent* inputComponent)
{
	if (UDrsEnhancedInputComponent* enhancedInputComponent = Cast<UDrsEnhancedInputComponent>(inputComponent))
	{
		if (IsBoundToPawn(pawn) && CurrentInputComponent == nullptr)
		{
			AddInputActions(enhancedInputComponent);
			CurrentInputComponent = enhancedInputComponent;
		}
	}
}


void UDrsWidgetInteractionComponent::RemoveInputActions(UDrsEnhancedInputComponent* inputComponent)
{
	if (inputComponent)
	{
		inputComponent->ClearBindingsForObject(this);
	}
}


void UDrsWidgetInteractionComponent::SetSuspended(bool suspend, FName suspendReason)
{
	if (SuspendedState.SetVoteCastState(suspendReason, suspend))
	{
		OnActiveStateChanged();
	}
}


void UDrsWidgetInteractionComponent::OnActiveStateChanged()
{
	if (!IsSuspended())
	{
		// Make sure we have claimed a unique user index prior to activation.
		ClaimUserIndex();

		// Enable component tick and registers the virtual user.
		UE_LOG(LogWidgetInteraction, Verbose, TEXT("Activating widget interaction component '%s'%s."), *ComponentPrintName(this), !VirtualUser.IsValid() ? TEXT(" and registering virtual user") : TEXT(""));
		Activate();
	}
	else if (bPreserveVirtualUser)
	{
		// Clear widget interaction state
		OnResetState();
		
		// Disable component tick without unregistering the virtual user.
		UE_LOG(LogWidgetInteraction, Verbose, TEXT("Deactivating widget interaction component '%s' and preserving virtual user."), *ComponentPrintName(this));
		UActorComponent::Deactivate();
	}
	else
	{
		// Clear widget interaction state (BEFORE we unregister the virtual user).
		OnResetState();
		
		// Disable component tick and unregisters virtual user.
		UE_LOG(LogWidgetInteraction, Verbose, TEXT("Deactivating widget interaction component '%s'%s."), *ComponentPrintName(this), VirtualUser.IsValid() ? TEXT(" and unregistering virtual user") : TEXT(""));
		Deactivate();

		// To prevent the virtual user list growing without end, we free up the user index for potential re-use by other
		// components.
		ReleaseUserIndex();
	}
}


void UDrsWidgetInteractionComponent::OnHoveredWidgetComponentChanged(UWidgetComponent* prev, UWidgetComponent* next)
{
	WidgetObjectLookup.Reset();
}


void UDrsWidgetInteractionComponent::OnResetState()
{
	// Clear any active widget interaction.
	ClearLastHitResult();
	
	// Stop pressing.
	if (bIsSelecting)
	{
		SetIsSelecting(false);
	}

	// Stop hovering.
	if (bIsHovering)
	{
		SetIsHovering(false);
		SetIsHoveringScrollableWidget(false);
	}

	// Stop right-clicking
	if (bIsRightClicking)
	{
		SetIsRightClicking(false);
	}
	
	// Reset state.
	WidgetObjectLookup.Reset();
	WeakHoveredWidgetComponent = nullptr;
	HoverDistance = TNumericLimits<float>::Max();
	
	CurrentMousePosition = FVector::ZeroVector;
	MousePositionOnSelect = FVector::ZeroVector;
	MouseDeviation = 0.0f;
	
	CurrentInteractorLocalPosition = FVector::ZeroVector;
	InteractorLocalPositionOnSelect = FVector::ZeroVector;
	InteractorDeviation = 0.0f;
	
	bIsDragging = false;
}


void UDrsWidgetInteractionComponent::OnSelect()
{
	if (IsActive() && bIsHovering && LastWidgetPath.IsValid() && IsValid(WeakHoveredWidgetComponent.Get()) && CanSelect(WeakHoveredWidgetComponent.Get()))
	{
		SetIsSelecting(true);
	}
}


void UDrsWidgetInteractionComponent::OnRelease()
{
	if (bIsSelecting && GetSelectedWidgetType() != EInteractableWidgetType::None)
	{
		LastSelectTime = FApp::GetGameTime();
	}
	SetIsSelecting(false);
}


void UDrsWidgetInteractionComponent::OnScrollVertical(const FInputActionValue& value)
{
	if (CanScroll())
	{
		const float scrollDelta = value.Get<float>();

		// Allow scrolling of any unidentified scrollable widgets. (Since we cannot know their orientation, we will assume they are vertical.)
		if (!HoveredScrollableSlateWidget.IsValid() || HoveredScrollableWidgetType == EInteractableWidgetType::None)
		{
			ScrollWheel(scrollDelta * ScrollSpeedMultiplier);
			return;
		}
		
		switch (HoveredScrollableWidgetType)
		{
		case EInteractableWidgetType::ListView:
			// SListView is templated to some unknown type, which means we cannot cast the slate widget directly. Instead,
			// we need to go via the corresponding UWidget object.
			if (const UListView* listViewObject = Cast<UListView>(GetScrollableWidgetObject()))
			{
				if (listViewObject->GetOrientation() == EOrientation::Orient_Vertical)
				{
					ScrollWheel(scrollDelta * ScrollSpeedMultiplier);
				}
			}
			break;
			
		case EInteractableWidgetType::ScrollBox:
			if (SScrollBox* scrollbox = static_cast<SScrollBox*>(HoveredScrollableSlateWidget.Pin().Get()))
			{
				if (scrollbox->GetOrientation() == EOrientation::Orient_Vertical)
				{
					ScrollWheel(scrollDelta * ScrollSpeedMultiplier);
				}
			}
			break;
		
		case EInteractableWidgetType::ScrollBox2D:
			if (SScrollBox2D* scrollbox = static_cast<SScrollBox2D*>(HoveredScrollableSlateWidget.Pin().Get()))
			{
				scrollbox->ScrollVertical(scrollDelta * ScrollSpeedMultiplier);
			}
			break;
		
		case EInteractableWidgetType::ComboBox:
			ScrollWheel(scrollDelta * ScrollSpeedMultiplier);
			break;
			
		default:
			ensureMsgf(false, TEXT("Unhandled scrollable widget type."));
			break;
		}
	}
}


void UDrsWidgetInteractionComponent::OnScrollHorizontal(const FInputActionValue& value)
{
	if (CanScroll() && HoveredScrollableSlateWidget.IsValid())
	{
		const float scrollDelta = value.Get<float>();
		
		switch (HoveredScrollableWidgetType)
		{
		case EInteractableWidgetType::ScrollBox:
			if (SScrollBox* scrollbox = static_cast<SScrollBox*>(HoveredScrollableSlateWidget.Pin().Get()))
			{
				if (scrollbox->GetOrientation() == EOrientation::Orient_Horizontal)
				{
					ScrollWheel(scrollDelta * ScrollSpeedMultiplier);
				}
			}
			break;

		case EInteractableWidgetType::ListView:
			// SListView is templated to some unknown type, which means we cannot cast the slate widget directly. Instead,
			// we need to go via the corresponding UWidget object.
			if (const UListView* listView = Cast<UListView>(GetScrollableWidgetObject()))
			{
				if (listView->GetOrientation() == EOrientation::Orient_Horizontal)
				{
					ScrollWheel(scrollDelta * ScrollSpeedMultiplier);
				}
			}
			break;

		case EInteractableWidgetType::ScrollBox2D:
			if (SScrollBox2D* scrollbox = static_cast<SScrollBox2D*>(HoveredScrollableSlateWidget.Pin().Get()))
			{
				scrollbox->ScrollHorizontal(scrollDelta * ScrollSpeedMultiplier);
			}
			break;

		case EInteractableWidgetType::ComboBox:
			// Combo-boxes are vertical only.
			break;
			
		default:
			ensureMsgf(false, TEXT("Unhandled scrollable widget type."));
			break;
		}
	}
}


bool UDrsWidgetInteractionComponent::CanScroll() const
{
	// Need to prevent conflicts between manual scrolling via the scrollbox and automatic scrolling via the scrollbar.
	return IsActive()
		&& bIsHovering
		&& WeakHoveredWidgetComponent.IsValid()
		&& SelectedWidgetType != EInteractableWidgetType::ScrollBar;
}


bool UDrsWidgetInteractionComponent::CanDrag() const
{
	return bIsDragScrollingEnabled
		&& bIsHoveringScrollableWidget
		&& HoveredScrollableSlateWidget.IsValid()
		&& CanScroll();
}


bool UDrsWidgetInteractionComponent::IsInertialScrollEnabled() const
{
	return bIsDragScrollingEnabled && bIsInertialScrollingEnabled;
}


bool UDrsWidgetInteractionComponent::UseCustomDragScrolling() const
{
	return !bIsInertialScrollingEnabled;
}


bool UDrsWidgetInteractionComponent::IsHoveredWidgetDraggable() const
{
	if (bIsHoveringScrollableWidget && bIsDragScrollingEnabled)
	{
		switch (HoveredScrollableWidgetType)
		{
		case EInteractableWidgetType::ListView:
			if (const IDrsListViewInterface* listViewInterface = Cast<IDrsListViewInterface>(GetScrollableWidgetObject()))
			{
				return listViewInterface->IsDragScrollingEnabled();
			}
			break;
			
		case EInteractableWidgetType::ScrollBox:
			if (const UScrollBox* scrollbox = Cast<UScrollBox>(GetScrollableWidgetObject()))
			{
PRAGMA_DISABLE_DEPRECATION_WARNINGS
				return scrollbox->bAllowRightClickDragScrolling; // NOTE: The accessor for this property currently returns the wrong value (UE 5.3.2)!
PRAGMA_ENABLE_DEPRECATION_WARNINGS
			}
			break;
			
		case EInteractableWidgetType::ScrollBox2D:
			if (const SScrollBox2D* scrollbox = static_cast<SScrollBox2D*>(HoveredScrollableSlateWidget.Pin().Get()))
			{
				return scrollbox->IsDragScrollingEnabled();
			}
			break;

		case EInteractableWidgetType::ComboBox:
			// ComboBoxes do not have an 'AllowRightClickDragScrolling' property to check.
			// Typically, clicking on a combobox should select one of its items, so drag scrolling should not be required.
			return false;
			
		default:
			ensureMsgf(false, TEXT("Unhandled scrollable widget type."));
			break;
		} 
	}
	
	return false;
}


TOptional<EOrientation> UDrsWidgetInteractionComponent::GetHoveredWidgetScrollDirection() const
{
	if (bIsHoveringScrollableWidget)
	{
		switch (HoveredScrollableWidgetType)
		{
			case EInteractableWidgetType::ListView:
				if (const UListView* listView = Cast<UListView>(GetScrollableWidgetObject()))
				{
					return listView->GetOrientation();
				}
				break;
			
			case EInteractableWidgetType::ScrollBox:
				if (const UScrollBox* scrollbox = Cast<UScrollBox>(GetScrollableWidgetObject()))
				{
					return scrollbox->GetOrientation();
				}
				break;
			
			case EInteractableWidgetType::ComboBox:
				return Orient_Vertical;
			
			default:
				ensureMsgf(false, TEXT("Unhandled scrollable widget type."));
			case EInteractableWidgetType::ScrollBox2D:
				break;
		} 
	}
	
	return {};
}


bool UDrsWidgetInteractionComponent::IsDragScrollPreferredOverSelection() const
{
	// For cases where UI selection and scrolling can conflict, this function aims to arbitrate between the two by
	// trying to predict the user's intentions. This is done by measuring the amount by which the mouse position has
	// deviated between selecting and releasing the widget. In some cases we want to be extra sure that the user is
	// intending to scroll rather than select. In addition to measuring the mouse movement, we also measure how much
	// the interactor component has moved in the local space of the widget component.

	// For now, we only handle buttons.
	if (SelectedWidgetType == EInteractableWidgetType::Button)
	{
		const float minDeviation = FMath::Max(MinimumMouseDeviationForDragging, UCoreButton::GetMaxMouseDeviation(Cast<UCoreButton>(GetSelectedWidgetObject())));
		float deviation = bMoreGenerousDragArbitration ? FMath::Min(MouseDeviation, InteractorDeviation) : MouseDeviation;
		
		if (deviation > minDeviation)
		{
			// Once some appreciable movement has been detected, compare it against the allowed scroll direction.
			TOptional<EOrientation> direction = GetHoveredWidgetScrollDirection();
			if (direction.IsSet())
			{
				if (deviation == InteractorDeviation)
				{
					const FVector localDelta = CurrentInteractorLocalPosition - InteractorLocalPositionOnSelect;
					deviation = (direction.GetValue() == Orient_Vertical) ? localDelta.Z : localDelta.Y;
				}
				else
				{
					const FVector localDelta = WeakHoveredWidgetComponent->GetComponentTransform().InverseTransformPositionNoScale(CurrentMousePosition) - WeakHoveredWidgetComponent->GetComponentTransform().InverseTransformPositionNoScale(MousePositionOnSelect);
					deviation = (direction.GetValue() == Orient_Vertical) ? localDelta.Z : localDelta.Y;
				}
			}

			return (FMath::Abs(deviation) > minDeviation);
		}
	}
	
	return false;
}


void UDrsWidgetInteractionComponent::UpdateDragging()
{
	if (!CanDrag())
	{
		bIsDragging = false;
	}
	else if (UseCustomDragScrolling())
	{
		//-----------------------------------------------------------
		// Custom drag scrolling (no inertia)
		//-----------------------------------------------------------
		const FVector   dragPosition = WeakHoveredWidgetComponent->GetComponentTransform().InverseTransformPosition(GetMousePosition());
		const FVector2D dragDelta	 = FVector2D(MouseDragPosition.Y - dragPosition.Y, MouseDragPosition.Z - dragPosition.Z);
		
		if (bIsDragging)
		{
			switch (HoveredScrollableWidgetType)
			{
			case EInteractableWidgetType::ListView:
				// For drag-scrolling, we need to cancel out any wheel-scroll multipliers configured on this widget.
				// However, because SListView is templated to some unknown type, we can only cast the corresponding UWidget
				// object. Annoyingly, UListView does not provide public access to its properties. To work around this,
				// we provide custom ListView classes that supply the missing accessors via a common interface.
				if (const UListView* listView = Cast<UListView>(GetScrollableWidgetObject()))
				{
					const IDrsListViewInterface* listViewInterface = Cast<IDrsListViewInterface>(listView);
					UE_CLOG(!listViewInterface, LogWidgetInteraction, Warning, TEXT("Native ListView widget detected. Please instead use the Core ListView widgets."));
	
					if (listViewInterface && listViewInterface->IsDragScrollingEnabled())
					{
						if (listView->GetOrientation() == EOrientation::Orient_Vertical)
						{
							ScrollWheel(dragDelta.Y * DragToScrollWheelFactor / listViewInterface->GetWheelScrollMultiplier());
						}
						else
						{
							ScrollWheel(dragDelta.X * DragToScrollWheelFactor / listViewInterface->GetWheelScrollMultiplier());
						}
					}
				}
				break;
				
			case EInteractableWidgetType::ScrollBox:
				// For drag-scrolling, we need to cancel out any wheel-scroll multipliers configured on this widget.
				// However, we can't access this multiplier from the slate widget, and so need to get it from the
				// corresponding UWidget object instead.
				if (const UScrollBox* scrollbox = Cast<UScrollBox>(GetScrollableWidgetObject()))
				{
PRAGMA_DISABLE_DEPRECATION_WARNINGS
					if (scrollbox->bAllowRightClickDragScrolling) // NOTE: The accessor for this property currently returns the wrong value (UE 5.3.2)!
PRAGMA_ENABLE_DEPRECATION_WARNINGS
					{
						if (scrollbox->GetOrientation() == EOrientation::Orient_Vertical)
						{
							ScrollWheel(dragDelta.Y * DragToScrollWheelFactor / scrollbox->GetWheelScrollMultiplier()); 
						}
						else
						{
							ScrollWheel(dragDelta.X * DragToScrollWheelFactor / scrollbox->GetWheelScrollMultiplier());
						}
					}
				}
				break;
				
			case EInteractableWidgetType::ScrollBox2D:
				// Since we are using a custom scroll method in place of ScrollWheel(), there is no need to apply the
				// drag-to-wheel speed conversion factor.
				if (SScrollBox2D* scrollbox = static_cast<SScrollBox2D*>(HoveredScrollableSlateWidget.Pin().Get()))
				{
					if (scrollbox->IsDragScrollingEnabled())
					{
						scrollbox->DragScroll2D(dragDelta);
					}
				}
				break;

			case EInteractableWidgetType::ComboBox:
				// ComboBoxes do not have a scroll wheel multiplier.
				// Note: Drag-scrolling should probably not be needed for combo-boxes.
				ScrollWheel(dragDelta.Y * DragToScrollWheelFactor);
				break;
				
			default:
				ensureMsgf(false, TEXT("Unhandled scrollable widget type."));
				break;
			}
		}

		// Wait until the mouse position moves enough before starting scrolling.
		bIsDragging |= (GetMouseDeviation() > MinimumMouseDeviationForDragging);
		MouseDragPosition = dragPosition;
	}
	else
	{
		//-----------------------------------------------------------
		// Native drag scrolling (requires right mouse button key)
		//-----------------------------------------------------------
		// Native drag-scrolling does not work if an interactable widget (e.g. button) is selected. In such
		// situations we need to detect that scrolling is intended and force-release the left mouse button.
		if (bIsRightClicking && bIsLeftClicking && IsDragScrollPreferredOverSelection())
		{
			SetIsLeftClicking(false, true);
		}

		bIsDragging = bIsRightClicking && !bIsLeftClicking;
	}
}


void UDrsWidgetInteractionComponent::UpdateInputContext(bool enableInput)
{
	enableInput &= InteractorHand.IsSet();
	
	if (enableInput != bInputContextEnabled)
	{
		if (enableInput)
		{
			CoreInputUtils::AddInputContext(this, UDrsInputSettings::Get().UIInteractionControls, InteractorHand.GetValue(), GetFName());
		}
		else
		{
			CoreInputUtils::RemoveInputContext(this, UDrsInputSettings::Get().UIInteractionControls, InteractorHand.GetValue(), GetFName());
		}
		
		bInputContextEnabled = enableInput;
	}
}


void UDrsWidgetInteractionComponent::UpdateScrollInputContext(bool enableInput)
{
	enableInput &= InteractorHand.IsSet();
	
	if (enableInput != bScrollInputContextEnabled)
	{
		if (enableInput)
		{
			CoreInputUtils::AddInputContext(this, UDrsInputSettings::Get().UIScrollControls, InteractorHand.GetValue(), GetFName());
		}
		else
		{
			CoreInputUtils::RemoveInputContext(this, UDrsInputSettings::Get().UIScrollControls, InteractorHand.GetValue(), GetFName());
		}
		
		bScrollInputContextEnabled = enableInput;
	}
}


void UDrsWidgetInteractionComponent::UpdateHoveredState()
{
	TWeakPtr<SWidget> prevHoveredWidget = GetHoveredSlateWidget();
	const EInteractableWidgetType prevHoveredSelectableWidgetType = HoveredSelectableWidgetType;
	
	// Query information about the widget(s) being hovered.
	const bool isHovering       = IsActive() && IsOverHitTestVisibleWidget() && WeakHoveredWidgetComponent.IsValid();
	HoveredSelectableWidgetType = FindHoveredSelectableWidget(isHovering ? LastWidgetPath : FWeakWidgetPath(), HoveredSelectableSlateWidget, HoveredSelectableUserWidget);
	HoveredScrollableWidgetType = FindHoveredScrollableWidget(isHovering ? LastWidgetPath : FWeakWidgetPath(), HoveredScrollableSlateWidget, HoveredScrollableUserWidget);
	
	// Update hovered state and any associated input contexts.
	SetIsHovering(isHovering);
	SetIsHoveringScrollableWidget(HoveredScrollableWidgetType != EInteractableWidgetType::None);
	
	if (GetHoveredSlateWidget() != prevHoveredWidget)
	{
		UE_CLOG(HoveredSelectableWidgetType != EInteractableWidgetType::None, LogWidgetInteraction, Verbose, TEXT("%s hand hovering selectable widget of type '%s'."),  *GetInteractorHandString(), *RW1::EnumToString(HoveredSelectableWidgetType));
		UE_CLOG(HoveredScrollableWidgetType != EInteractableWidgetType::None, LogWidgetInteraction, Verbose, TEXT("%s hand hovering scrollable widget of type '%s'."),  *GetInteractorHandString(), *RW1::EnumToString(HoveredScrollableWidgetType));
		
		// Trigger optional haptic feedback for hovering (selectable) widgets. (But do not trigger haptics when moving
		// immediately from one selectable widget to the next, otherwise certain layouts may cause false positives.)
		if (bIsHovering && HoveredSelectableWidgetType != EInteractableWidgetType::None)
		{
			if (prevHoveredSelectableWidgetType == EInteractableWidgetType::None)
			{
				StartHoverHaptics();
			}
		}
		else
		{
			EndHoverHaptics();
		}

		// Notifications.
		if (OnWidgetChanged.IsBound())
		{
			OnWidgetChanged.Broadcast(this, GetHoveredWidgetObject(), GetHoveredWidgetType());
		}
	}
}


void UDrsWidgetInteractionComponent::SetIsHovering(bool isHovering)
{
	if (isHovering != bIsHovering)
	{
		bIsHovering = isHovering;
		
		if (isHovering)
		{
			OnWidgetHovered.Broadcast(this, true);
			UpdateInputContext(true);
		}
		else
		{
			OnWidgetHovered.Broadcast(this, false);
			UpdateInputContext(bIsSelecting);

			// Additional clean-up in case hovering is force-terminated.
			HoveredSelectableWidgetType = EInteractableWidgetType::None;
			HoveredScrollableWidgetType = EInteractableWidgetType::None;
			HoveredSelectableSlateWidget = nullptr;
			HoveredScrollableSlateWidget = nullptr;
			HoveredSelectableUserWidget = nullptr;
			HoveredScrollableUserWidget = nullptr;

			EndHoverHaptics();
		}
	}
}


void UDrsWidgetInteractionComponent::SetIsHoveringScrollableWidget(bool isHovering)
{
	if (isHovering != bIsHoveringScrollableWidget)
	{
		bIsHoveringScrollableWidget = isHovering;
		UpdateScrollInputContext(isHovering);

		// Additional clean-up in case hovering is force-terminated.
		if (!isHovering)
		{
			HoveredScrollableWidgetType = EInteractableWidgetType::None;
			HoveredScrollableSlateWidget = nullptr;
			HoveredScrollableUserWidget = nullptr;
		}
	}
}


void UDrsWidgetInteractionComponent::SetIsSelecting(bool isSelecting)
{
	if (isSelecting != bIsSelecting)
	{
		UE_LOG(LogWidgetInteraction, Verbose, TEXT("Set is selecting '%d' on %s hand."), isSelecting, *GetInteractorHandString());
		
		if (isSelecting)
		{
			bIsSelecting          = true;
			MouseDeviation        = 0.0f;
			InteractorDeviation   = 0.0f;
			MousePositionOnSelect = CurrentMousePosition;
			InteractorLocalPositionOnSelect = CurrentInteractorLocalPosition;
			
			SelectedSlateWidget   = GetHoveredSlateWidget();
			SelectedUserWidget    = GetHoveredUserWidget();
			SelectedWidgetType    = GetHoveredWidgetType();
			
			// The right virtual mouse button is needed for drag scrolling with inertia. We *could* trigger right-clicks
			// via separate input actions bound to different controls. However, this would not work for touch interaction,
			// and so we instead try to keep the behavior consistent across different interaction methods by clicking the
			// left and right virtual mouse buttons together.
			SetIsLeftClicking(true);
			SetIsRightClicking(AllowRightClicking());
			UE_CLOG(!bIsRightClicking, LogWidgetInteraction, Verbose, TEXT("Right-clicking disallowed on %s hand."), *GetInteractorHandString());
			
			OnWidgetSelected.Broadcast(this, true);
		}
		else
		{
			SetIsLeftClicking(false);
			SetIsRightClicking(false);
			UpdateInputContext(bIsHovering);
			
			bIsSelecting        = false;
			MouseDeviation      = 0.0f;
			InteractorDeviation = 0.0f;
			
			SelectedSlateWidget = nullptr;
			SelectedWidgetType  = EInteractableWidgetType::None;
			
			OnWidgetSelected.Broadcast(this, false);
		}
	}
}


void UDrsWidgetInteractionComponent::SetIsLeftClicking(bool isClicking, bool suppressClickEvent)
{
	if (isClicking != bIsLeftClicking)
	{
		UE_LOG(LogWidgetInteraction, Verbose, TEXT("Set is left-clicking '%d' on %s hand."), isClicking, *GetInteractorHandString());
		
		// In some special cases we want to release the left mouse button without triggering a click event.
		// To achieve this, we need to clear the 'is clicking' flag *before* the pointer key is processed, so that
		// AllowButtonClickEvents() will return false if queried by the custom Core button widget.
		if (suppressClickEvent)
		{
			bIsLeftClicking = isClicking;
		}
		
		if (isClicking)
		{
			PressPointerKey(EKeys::LeftMouseButton);
			StartSelectHaptics();
		}
		else
		{
			ReleasePointerKey(EKeys::LeftMouseButton);
			EndSelectHaptics();
		}

		// In all normal cases we want to allow click events to be triggered when the left mouse button is released.
		// To achieve this, we need to clear the 'is clicking' flag *after* the pointer key is processed, so that
		// AllowButtonClickEvents() will return true if queried by the custom Core button widget.
		if (!suppressClickEvent)
		{
			bIsLeftClicking = isClicking;
		}
	}
}


void UDrsWidgetInteractionComponent::SetIsRightClicking(bool isClicking)
{
	if (isClicking != bIsRightClicking)
	{
		UE_LOG(LogWidgetInteraction, Verbose, TEXT("Set is right-clicking '%d' on %s hand."), isClicking, *GetInteractorHandString());
		
		if (isClicking)
		{
			PressPointerKey(EKeys::RightMouseButton);
		}
		else
		{
			ReleasePointerKey(EKeys::RightMouseButton);
		}

		bIsRightClicking = isClicking;
	}
}


bool UDrsWidgetInteractionComponent::AllowRightClicking() const
{
	// Right mouse button events are needed for drag scrolling with inertia (but not for custom, non-inertial drag-scrolling).
	// Note: Right mouse button events can prevent combo-boxes from opening. Therefore, we skip inertial drag-scrolling
	// of any scroll-boxes while a combo-box is hovered.
	return IsInertialScrollEnabled() && IsHoveredWidgetDraggable() && !UseCustomDragScrolling() && !IsHoveringScrollableWidgetType(EInteractableWidgetType::ComboBox);
}


void UDrsWidgetInteractionComponent::StartHoverHaptics()
{
	if (bIsHovering && !ActiveHoverHapticEventId.IsValid() && IsValid(HoveringHapticEvent) && InteractorHand.IsSet())
	{
		ActiveHoverHapticEventId = HapticsUtils::StartHapticEvent(this, InteractorHand.GetValue(), HoveringHapticEvent);
	}
}


void UDrsWidgetInteractionComponent::EndHoverHaptics()
{
	if (ActiveHoverHapticEventId.IsValid())
	{
		HapticsUtils::EndHapticEvent(this, ActiveHoverHapticEventId);
		ActiveHoverHapticEventId.Reset();
	}
}


void UDrsWidgetInteractionComponent::StartSelectHaptics()
{
	if (!ActiveSelectHapticEventId.IsValid() && IsValid(SelectionHapticEvent) && InteractorHand.IsSet())
	{
		ActiveSelectHapticEventId = HapticsUtils::StartHapticEvent(this, InteractorHand.GetValue(), SelectionHapticEvent);
	}
}


void UDrsWidgetInteractionComponent::EndSelectHaptics()
{
	if (ActiveSelectHapticEventId.IsValid())
	{
		HapticsUtils::EndHapticEvent(this, ActiveSelectHapticEventId);
		ActiveSelectHapticEventId.Reset();
	}
}


TWeakPtr<SWidget> UDrsWidgetInteractionComponent::GetHoveredSlateWidget() const
{
	// Only include scrollable widgets (e.g. scroll-box) if we are not hovering an interactable widget (e.g. button, scrollbar).
	const bool isHoveringScrollableWidget = (bIsHoveringScrollableWidget && HoveredSelectableWidgetType == EInteractableWidgetType::None);
	return isHoveringScrollableWidget ? HoveredScrollableSlateWidget : HoveredSelectableSlateWidget;
}


const UUserWidget* UDrsWidgetInteractionComponent::GetHoveredUserWidget() const
{
	// Only include scrollable widgets (e.g. scroll-box) if we are not hovering an interactable widget (e.g. button, scrollbar).
	const bool isHoveringScrollableWidget = (bIsHoveringScrollableWidget && HoveredSelectableWidgetType == EInteractableWidgetType::None);
	return isHoveringScrollableWidget ? HoveredScrollableUserWidget : HoveredSelectableUserWidget;
}


EInteractableWidgetType UDrsWidgetInteractionComponent::GetHoveredWidgetType() const
{
	// Only include scrollable widgets (e.g. scroll-box) if we are not hovering an interactable widget (e.g. button, scrollbar).
	const bool isHoveringScrollableWidget = (bIsHoveringScrollableWidget && HoveredSelectableWidgetType == EInteractableWidgetType::None);
	return isHoveringScrollableWidget ? HoveredScrollableWidgetType : HoveredSelectableWidgetType;
}


UWidget* UDrsWidgetInteractionComponent::GetHoveredWidgetObject() const
{
	// Only include scrollable widgets (e.g. scroll-box) if we are not hovering an interactable widget (e.g. button, scrollbar).
	const bool isHoveringScrollableWidget = (bIsHoveringScrollableWidget && HoveredSelectableWidgetType == EInteractableWidgetType::None);
	return isHoveringScrollableWidget ? GetWidgetObject(HoveredScrollableSlateWidget, HoveredScrollableUserWidget) : GetWidgetObject(HoveredSelectableSlateWidget, HoveredSelectableUserWidget);
}


bool UDrsWidgetInteractionComponent::IsHoveringInteractableWidget() const
{
	if (HoveredSelectableWidgetType != EInteractableWidgetType::None)
	{
		return true;
	}

	if (IsHoveredWidgetDraggable())
	{
		return true;
	}
	
	return false;
}


void UDrsWidgetInteractionComponent::ClearLastHitResult()
{
	// Adapted from UWidgetInteractionComponent::SimulatePointerMovement().
	SetCustomHitResult(NoHit);
	LastHitResult = NoHit;
	LastWidgetPath = FWeakWidgetPath();
	
	if (VirtualUser.IsValid())
	{
		FPointerEvent PointerEvent(
			VirtualUser->GetUserIndex(),
			static_cast<uint32>(PointerIndex),
			LocalHitLocation,
			LastLocalHitLocation,
			PressedKeys,
			FKey(),
			0.0f,
			ModifierKeys);
		
		FSlateApplication::Get().RoutePointerMoveEvent(FWidgetPath(), PointerEvent, false);
	}
}


bool UDrsWidgetInteractionComponent::IsInteractionConflict(const FWidgetPath& hitWidgetPath) const
{
	// This function prevents a second interactor hovering a widget that another interactor is already interacting with.
	
	// TODO: Fix the following edge-cases:
	// #1. While selecting a button inside a scrollbox with one interactor, prevent additional interactors from scrolling
	// that scrollbox (either via scrollbar or via dragging).
	// #2. While scrolling a scrollbox with one interactor, allow additional interactors to select buttons inside that
	// scrollbox (but do not allow them to start dragging the scrollbox).
	
	if (!hitWidgetPath.IsValid())
	{
		return false;
	}
	
	if (const UUserInterfaceSubsystem* subsystem = UUserInterfaceSubsystem::Get(this))
	{
		for (const UDrsWidgetInteractionComponent* widgetInteraction : subsystem->GetRegisteredWidgetInteractors())
		{
			if (widgetInteraction
				&& widgetInteraction != this
				&& widgetInteraction->IsSelecting()
				&& widgetInteraction->GetSelectedSlateWidget().IsValid())
			{
				if (hitWidgetPath.ContainsWidget(widgetInteraction->GetSelectedSlateWidget().Pin().Get()))
				{
					return true;
				}
			}
		}
	}
	
	return false;
}


EInteractableWidgetType UDrsWidgetInteractionComponent::FindHoveredSelectableWidget(const FWeakWidgetPath& widgetPath, TWeakPtr<SWidget>& outSlateWidget, const UUserWidget*& outUserWidget)
{
	return FindHoveredWidget(SelectableWidgetLookup, widgetPath, outSlateWidget, outUserWidget);
}


EInteractableWidgetType UDrsWidgetInteractionComponent::FindHoveredScrollableWidget(const FWeakWidgetPath& widgetPath, TWeakPtr<SWidget>& outSlateWidget, const UUserWidget*& outUserWidget)
{
	return FindHoveredWidget(ScrollableWidgetLookup, widgetPath, outSlateWidget, outUserWidget);
}


EInteractableWidgetType UDrsWidgetInteractionComponent::FindHoveredWidget(const TMap<FName, EInteractableWidgetType>& lookup, const FWeakWidgetPath& widgetPath, TWeakPtr<SWidget>& outSlateWidget, const UUserWidget*& outUserWidget)
{
	outUserWidget = nullptr;
	
	if (widgetPath.IsValid())
	{
		for (int32 forwardIdx = 0; forwardIdx < widgetPath.Widgets.Num(); ++forwardIdx)
		{
			const TWeakPtr<SWidget>& widget = widgetPath.Widgets[forwardIdx];
			if (const SWidget* slateWidget = widget.IsValid() ? widget.Pin().Get() : nullptr)
			{
				if (slateWidget->GetVisibility() == EVisibility::Visible)
				{
					if (const EInteractableWidgetType* widgetType = lookup.Find(slateWidget->GetType()))
					{
						for (int32 backwardIdx = forwardIdx; backwardIdx >= 0; --backwardIdx)
						{
							const TWeakPtr<SWidget>& precedingWidget = widgetPath.Widgets[backwardIdx];
							if (const SWidget* precedingSlateWidget = precedingWidget.IsValid() ? precedingWidget.Pin().Get() : nullptr)
							{
								if (precedingSlateWidget->GetType() == SObjectWidgetType)
								{
									outUserWidget = static_cast<const SObjectWidget*>(precedingSlateWidget)->GetWidgetObject();
									break;
								}
							}
						}
						
						outSlateWidget = widget;
						return *widgetType;
					}
				}
			}
		}
	}
	
	outSlateWidget = TWeakPtr<SWidget>();
	return EInteractableWidgetType::None;
}


bool UDrsWidgetInteractionComponent::IsHoveringWidgetType(const FWeakWidgetPath& widgetPath, EInteractableWidgetType widgetType, const TMap<FName, EInteractableWidgetType>& widgetTypeLookup)
{
	if (widgetPath.IsValid())
	{
		for (int32 i = widgetPath.Widgets.Num() - 1; i >= 0; --i)
		{
			const TWeakPtr<SWidget>& widget = widgetPath.Widgets[i];
			if (const SWidget* slateWidget = widget.IsValid() ? widget.Pin().Get() : nullptr)
			{
				if (const EInteractableWidgetType* widgetTypePtr = widgetTypeLookup.Find(slateWidget->GetType()))
				{
					if (*widgetTypePtr == widgetType)
					{
						return true;
					}
				}
			}
		}
	}
	
	return false;
}


bool UDrsWidgetInteractionComponent::IsHoveringSelectableWidgetType(EInteractableWidgetType widgetType) const
{
	return IsHoveringWidgetType(LastWidgetPath, widgetType, SelectableWidgetLookup);
}


bool UDrsWidgetInteractionComponent::IsHoveringScrollableWidgetType(EInteractableWidgetType widgetType) const
{
	return IsHoveringWidgetType(LastWidgetPath, widgetType, ScrollableWidgetLookup);
}


UWidget* FindWidgetInTree(const UWidgetTree* widgetTree, bool skipCollapsedBranches, const TFunctionRef<bool(UWidget*)>& predicate)
{
	// Brute force method for finding a widget object in a widget tree based on some predicate.
	// Adapted from UWidgetTree::ForEachWidgetAndDescendants() with added early-out behaviour and skipping of
	// collapsed branches.
	UWidget* foundWidgetObject = nullptr;

	if (widgetTree)
	{
		widgetTree->ForEachWidget([&](UWidget* widget)
			{
				if (foundWidgetObject)
				{
					return;
				}
			
				if (skipCollapsedBranches && widget->GetVisibility() == ESlateVisibility::Collapsed)
				{
					return;
				}
				
				if (const UUserWidget* userWidgetChild = (widget != widgetTree->RootWidget) ? Cast<UUserWidget>(widget) : nullptr)
				{
					if (userWidgetChild->WidgetTree)
					{
						foundWidgetObject = FindWidgetInTree(userWidgetChild->WidgetTree, skipCollapsedBranches, predicate);
						return;
					}
				}
			
				if (predicate(widget))
				{
					foundWidgetObject = widget;
				}
			});
	}

	return foundWidgetObject;
}


UWidget* UDrsWidgetInteractionComponent::GetSelectedWidgetObject() const
{
	return GetWidgetObject(SelectedSlateWidget, SelectedUserWidget);
}


UWidget* UDrsWidgetInteractionComponent::GetSelectableWidgetObject() const
{
	return GetWidgetObject(HoveredSelectableSlateWidget, HoveredSelectableUserWidget);
}


UWidget* UDrsWidgetInteractionComponent::GetScrollableWidgetObject() const
{
	return GetWidgetObject(HoveredScrollableSlateWidget, HoveredScrollableUserWidget);
}


UWidget* UDrsWidgetInteractionComponent::GetWidgetObject(const TWeakPtr<SWidget>& slateWidget, const UUserWidget* userWidget) const
{
	if (slateWidget.IsValid())
	{
		// Fast lookup of UWidget object corresponding to a specific Slate widget from cache.
		if (UWidget** widgetObjectHandle = WidgetObjectLookup.Find(slateWidget))
		{
			return *widgetObjectHandle;
		}

		// Choose the widget-tree to search.
		if (userWidget == nullptr)
		{
			userWidget = WeakHoveredWidgetComponent.IsValid() ? WeakHoveredWidgetComponent->GetUserWidgetObject() : nullptr;
		}
		
		// Brute force method for finding the UWidget object corresponding to a specific Slate widget.
		// Results are added to a cache for fast lookup next time.
		if (userWidget)
		{
			UWidget* widgetObject = FindWidgetInTree(userWidget->WidgetTree, true, [&](const UWidget* widget)
				{
					return (widget && widget->GetCachedWidget() == slateWidget);
				});

			if (widgetObject)
			{
				WidgetObjectLookup.Add(slateWidget, widgetObject);
				return widgetObject;
			}
		}
	}
	
	return nullptr;
}


void UDrsWidgetInteractionComponent::ConsolidateWidgetCaches()
{
	for (auto it = WidgetObjectLookup.CreateIterator(); it; ++it)
	{
		if (!it.Key().IsValid())
		{
			it.RemoveCurrent();
		}
	}
}


bool UDrsWidgetInteractionComponent::HasFocusedWidget(const UWidget* widget) const
{
	if (widget)
	{
		// This implementation is needed for IsHovering()/IsSelecting() to work (e.g. for UCoreButton).
		if (GetWidgetObject(HoveredSelectableSlateWidget, HoveredSelectableUserWidget) == widget)
		{
			return true;
		}
		
		// This implementation is needed for WidgetUtils::FindFocusedWidgetInteractionFromWidget() to work (e.g. for
		// text boxes).
		if (VirtualUser.IsValid())
		{
			const TSharedPtr<SWidget> sWidget = widget->GetCachedWidget();
			return sWidget.IsValid() && (sWidget->HasUserFocus(VirtualUser->GetUserIndex()).IsSet() || sWidget->HasUserFocusedDescendants(VirtualUser->GetUserIndex()));
		}
	}
	return false;
}


FString UDrsWidgetInteractionComponent::GetInteractorHandString() const
{
	return *RW1::EnumToString(InteractorHand.Get(EControllerHand::AnyHand));
}


//-----------------------------------------------------------
// Debug rendering
//-----------------------------------------------------------

void UDrsWidgetInteractionComponent::SetShowInteractionDebug(bool showDebug)
{
	if (bShowInteractionDebug != showDebug)
	{
		bShowInteractionDebug = showDebug;
		UpdateInteractionDebug();
	}
}


void UDrsWidgetInteractionComponent::UpdateInteractionDebug()
{
#if ENABLE_DRAW_DEBUG
	// Debug render is driven from a world tick delegate rather than from TickComponent(), because suspending a widget
	// interaction component disables its tick - and suspension is itself one of the states that is worth displaying.
	if (bShowInteractionDebug)
	{
		if (!DebugDrawHandle.IsValid())
		{
			DebugDrawHandle = FWorldDelegates::OnWorldPostActorTick.AddWeakLambda(this, [this](const UWorld* world, ELevelTick, float deltaTime)
		   {
			   DrawInteractionDebug(world, deltaTime);
		   });
		}
	}
	else
	{
		if (DebugDrawHandle.IsValid())
		{
			FWorldDelegates::OnWorldPostActorTick.Remove(DebugDrawHandle);
			DebugDrawHandle.Reset();
		}
	}
#endif
}


void UDrsWidgetInteractionComponent::DrawInteractionDebug(const UWorld* world, const float deltaTime) const
{
#if ENABLE_DRAW_DEBUG
	using namespace WidgetInteractionDebug;

	if (!bShowInteractionDebug || world == nullptr)
	{
		return;
	}
	
	//--------------------------------------------
	// Cache shared debug info
	//--------------------------------------------
	FWidgetInteractionDebugInfo debugInfo;
	debugInfo.WidgetComponent	= GetHoveredWidgetComponent();
	debugInfo.InteractorPosition= GetComponentLocation();
	debugInfo.ContactPosition	= GetMousePosition();

	// Choose a draw color based on the interactor's state.
	debugInfo.StateColor	= IsSuspended()	? SuspendedColor
							: bIsDragging	? DragColor
							: bIsSelecting	? SelectColor
							: bIsHovering	? HoverColor
							:				  IdleColor;
	
	// Sample the surface normal at the contact point, so that geometry drawn on the surface stays tangential to
	// curved widgets as well as planar ones.
	if (IsValid(debugInfo.WidgetComponent))
	{
		debugInfo.SurfaceNormal = WidgetUtils::GetWidgetNormalAtPosition(debugInfo.WidgetComponent, debugInfo.ContactPosition);
		if (debugInfo.SurfaceNormal.IsNearlyZero())
		{
			debugInfo.SurfaceNormal = debugInfo.WidgetComponent->GetForwardVector();
		}
		debugInfo.SurfaceNormal.FindBestAxisVectors(debugInfo.SurfaceTangentZ, debugInfo.SurfaceTangentY);
	}

	//--------------------------------------------
	// Debug Geometry
	//--------------------------------------------
	DrawDebugGeometry(world, debugInfo);

	//--------------------------------------------
	// Debug Text
	//--------------------------------------------
	// Create an identifier string for this component based on its class. (e.g. "FingerWidgetInteractionComponent" -> "FINGER".) 
	FString label = GetClass()->GetName();
	label.RemoveFromEnd(TEXT("_C"));
	label.RemoveFromEnd(TEXT("WidgetInteractionComponent"));
	label = label.IsEmpty() ? TEXT("WIDGET") : label.ToUpper();
	
	// Header text
	FString text = FString::Printf(TEXT("%s %s"), *GetInteractorHandString(), *label);
	
	if (IsSuspended())
	{
		// If suspended, display the suspension reason(s). Nothing else is useful.
		text += FString::Printf(TEXT(" SUSPENDED (%s)"), *SuspendedState.GetVotesString());
	}
	else
	{
		// Append component-specific debug text.
		AppendDebugText(text);
	}
	
	// Anchor the readout at the contact point if there is one.
	const FVector textAnchor = IsValid(debugInfo.WidgetComponent) ? debugInfo.ContactPosition : debugInfo.InteractorPosition;
	DrawDebugString(world, textAnchor + TextOffset, text, nullptr, TextColor, FMath::Max(deltaTime, UE_KINDA_SMALL_NUMBER));
	UE_LOG(LogWidgetInteraction, Verbose, TEXT("%s"), *text);
#endif
}


void UDrsWidgetInteractionComponent::DrawDebugGeometry(const UWorld* world, const FWidgetInteractionDebugInfo& debugInfo) const
{
#if ENABLE_DRAW_DEBUG
	using namespace WidgetInteractionDebug;

	// Interactor position and direction.
	DrawDebugSphere(world, debugInfo.InteractorPosition, InteractorRadius, SphereSegments, debugInfo.StateColor);
	DrawDebugDirectionalArrow(world, debugInfo.InteractorPosition, debugInfo.InteractorPosition + GetForwardVector() * ForwardArrowLength, 1.0f, debugInfo.StateColor);

	if (IsValid(debugInfo.WidgetComponent))
	{
		// The line to the contact point visualizes the hover distance.
		DrawDebugLine(world, debugInfo.InteractorPosition, debugInfo.ContactPosition, debugInfo.StateColor);
		DrawDebugPoint(world, debugInfo.ContactPosition, PointSize, debugInfo.StateColor);

		// Whether click events would currently be allowed is shown as a ring around the contact point.
		const EAllowClickEvents allowClicks = AllowButtonClickEvents();
		const FColor clickColor = (allowClicks == EAllowClickEvents::Yes)						? AllowClickColor
								: (allowClicks == EAllowClickEvents::WithMouseDeviationCheck)	? CheckDeviationColor
								: DenyClickColor;
		DrawDebugCircle(world, debugInfo.ContactPosition, ContactRingRadius, CircleSegments, clickColor, false, -1.f, 0, 0.f, debugInfo.SurfaceTangentY, debugInfo.SurfaceTangentZ, false);
	}

	// Mouse deviation since selection started. (The reference position is only meaningful while selecting.)
	if (bIsSelecting)
	{
		const FVector referencePosition = MousePositionOnSelect;
		DrawDebugPoint(world, referencePosition, PointSize, DeviationColor);
		DrawDebugLine(world, referencePosition, debugInfo.ContactPosition, DeviationColor, false, -1.f, 0, 0.1f);
		
		if (const UCoreButton* selectedButton = Cast<UCoreButton>(GetSelectedWidgetObject()))
		{
			const float deviationLimit = UCoreButton::GetMaxMouseDeviation(selectedButton);
			if (deviationLimit > 0.0f && !debugInfo.SurfaceNormal.IsNearlyZero())
			{
				DrawDebugCircle(world, referencePosition, deviationLimit, CircleSegments, DeviationColor, false, -1.f, 0, 0.f, debugInfo.SurfaceTangentY, debugInfo.SurfaceTangentZ, false);
			}
		}
	}
#endif
}


void UDrsWidgetInteractionComponent::AppendDebugText(FString& outText) const
{
	using namespace WidgetInteractionDebug;

	// Interaction state info
	outText += FString::Printf(TEXT("\nhover:%d  select:%d  drag:%d  LMB:%d  RMB:%d  allow click:%s")
		, bIsHovering
		, bIsSelecting
		, bIsDragging
		, bIsLeftClicking
		, bIsRightClicking
		, *RW1::EnumToString(AllowButtonClickEvents()));

	// Hover distance - Distances are left at FLT_MAX while nothing is hovered, which is not worth printing.
	// Subclasses may insert additional text at this point, as identified by DistanceLabel.
	if (HoverDistance < TNumericLimits<float>::Max() * 0.5f)
	{
		outText += FString::Printf(TEXT("\n%s%.2f"), DistanceLabel, HoverDistance);
	}

	// Widget component info.
	if (const UWidgetComponent* widgetComponent = GetHoveredWidgetComponent())
	{
		outText += FString::Printf(TEXT("\nwidget:%s  geometry:%s  arc:%.1f")
			, *widgetComponent->GetName()
			, *RW1::EnumToString(widgetComponent->GetGeometryMode())
			, widgetComponent->GetCylinderArcAngle());
	}

	// Selection info / mouse deviation.
	if (bIsSelecting)
	{
		const UCoreButton* selectedButton = Cast<UCoreButton>(GetSelectedWidgetObject());
		const float clickDeviationLimit =  selectedButton ? UCoreButton::GetMaxMouseDeviation(selectedButton) : -1.0f;
		outText += FString::Printf(TEXT("\ndeviation:%.2f  peak:%.2f  (%sdrag>%.2f)")
			, FVector::Distance(MousePositionOnSelect, GetMousePosition())
			, GetMouseDeviation()
			, (clickDeviationLimit >= 0.0f) ? *FString::Printf(TEXT("click<=%.2f  "), clickDeviationLimit) : TEXT("")
			, MinimumMouseDeviationForDragging);
	}
}
