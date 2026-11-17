// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "Components/WidgetInteractionComponent.h"
#include "HapticId.h"
#include "Interfaces/DrsWidgetInteractionInterface.h"
#include "Interfaces/ValidateComponentInterface.h"
#include "Utils/VotableState.h"
#include "DrsWidgetInteractionComponent.generated.h"

// Forward declares
class UDrsEnhancedInputComponent;
class UHapticEvent;
class SWidget;
class UWidget;
struct FInputActionValue;
struct FWidgetInteractionDebugInfo;

// Types of interactable widget.
UENUM(BlueprintType)
enum class EInteractableWidgetType : uint8
{
	None,
	Button,
	Slider,
	ScrollBar,
	ScrollBox,
	ScrollBox2D,
	TextBox,
	RadialSlider,
	ListView,
	ComboBox,
};

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWidgetHoverEvent, UDrsWidgetInteractionComponent*, widgetInteractionComponent, bool, onOff);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWidgetSelectEvent, UDrsWidgetInteractionComponent*, widgetInteractionComponent, bool, onOff);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FWidgetChangedEvent, UDrsWidgetInteractionComponent*, widgetInteractionComponent, const UWidget*, widget, EInteractableWidgetType, widgetType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInteractorBindEvent, TScriptInterface<class IDrsWidgetInteractorInterface>, interactor);

// Logging
#if UE_BUILD_SHIPPING && !WITH_EDITOR
DECLARE_LOG_CATEGORY_EXTERN(LogWidgetInteraction, Log, Log);
#else
DECLARE_LOG_CATEGORY_EXTERN(LogWidgetInteraction, Log, All);
#endif


/**
 * Base class for Core's custom widget interaction components.
 */
UCLASS(Abstract, Blueprintable, ClassGroup = (Interaction), hideCategories = (Activation, AssetUserData, Collision, Cooking, ComponentReplication, HLOD, Lighting, LOD, Mobile, Navigation, Physics, Rendering, Tags))
class DRSUSERINTERFACE_API UDrsWidgetInteractionComponent :
	public UWidgetInteractionComponent,
	public IDrsWidgetInteractionInterface,
	public IValidateComponentInterface
{
	GENERATED_BODY()

public:
		
	UDrsWidgetInteractionComponent(const FObjectInitializer& objectInitializer);
	
protected:

	// Editor callbacks
	virtual void PreSave(FObjectPreSaveContext saveContext) override;
	
	// Engine callbacks
	virtual void InitializeComponent() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void TickComponent(float deltaTime, enum ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;
	
	// Input callbacks
	void OnSelect();
	void OnRelease();
	void OnScrollVertical(const FInputActionValue& value);
	void OnScrollHorizontal(const FInputActionValue& value);

	// IValidateComponentInterface
	virtual void OnComponentValidate() override {}
	
	// IDrsWidgetInteractionInterface
	virtual bool IsHovering(const UWidget* widget) const override final { return bIsHovering && HasFocusedWidget(widget); }
	virtual bool IsSelecting(const UWidget* widget) const override final { return bIsSelecting && HasFocusedWidget(widget); }
	virtual float GetMouseDeviation() const override final { return MouseDeviation; }
	virtual EAllowClickEvents AllowButtonClickEvents() const override final;
	
	// UWidgetInteractionComponent
	virtual FWidgetTraceResult PerformTrace() const override final;
	virtual FWidgetTraceResult PerformTraceInternal();
	
	// UCoreWidgetInteractionComponent interface
	virtual void AddInputActions(UDrsEnhancedInputComponent* inputComponent) {}
	virtual void RemoveInputActions(UDrsEnhancedInputComponent* inputComponent);
	virtual bool CanBindTo(TScriptInterface<IDrsWidgetInteractorInterface> interactor) const;
	virtual void OnBindingChanged() {}
	virtual void UpdateSelectedState() {}
	virtual void OnResetState();
	virtual bool CanScroll() const;
	virtual bool CanDrag() const;
	virtual FHitResult CreateCustomHitResult() const;
	virtual void DrawDebugGeometry(const UWorld* world, const FWidgetInteractionDebugInfo& debugInfo) const;
	virtual void AppendDebugText(FString& outText) const;
	virtual float GetInteractionRadius() const { return 0.f; } // Optional buffer distance around the interactor position (e.g. to account for object geometry).
	
public:
	
	// Binds this WidgetInteractionComponent to a specific pawn/hand.
	UFUNCTION(BlueprintCallable)
	void BindTo(TScriptInterface<IDrsWidgetInteractorInterface> interactor);

	// Unbinds this widget interaction component from a specific pawn/hand.
	UFUNCTION(BlueprintCallable)
	void UnbindFrom(TScriptInterface<IDrsWidgetInteractorInterface> interactor);
	
	// Unbinds this widget interaction component from its current pawn/hand.
	UFUNCTION(BlueprintCallable)
	void Unbind();
	
	// Returns whether this component is currently bound to a user.
	UFUNCTION(BlueprintPure)
	bool IsBound() const { return InteractorComponent != nullptr; }
	
	// Returns whether this component is currently bound to a specific interactor component.
	UFUNCTION(BlueprintPure)
	bool IsBoundTo(TScriptInterface<IDrsWidgetInteractorInterface> interactor) const;
	
	// Returns whether this component is currently bound to a specific user.
	UFUNCTION(BlueprintPure)
	bool IsBoundToPawn(const APawn* pawn) const;
	
	// Returns whether this component is currently bound to the local user's pawn.
	UFUNCTION(BlueprintPure)
	bool IsLocallyControlled() const { return bIsLocallyControlled; }

	// Returns which hand this component is currently associated with.
	UFUNCTION(BlueprintPure)
	EControllerHand GetHand() const { return InteractorHand.Get(EControllerHand::AnyHand); }
	
	// Suspends/unsuspends widget interaction via this component. A unique reason must be provided for each suspension.
	// Widget interaction will remain suspended until all reasons have been withdrawn. NOTE: Suspended widget interaction
	// components will unregister their virtual user, unless otherwise instructed via the Preserve Virtual User property.
	UFUNCTION(BlueprintCallable)
	void SetSuspended(bool suspend, FName suspendReason);
	
	UFUNCTION(BlueprintPure)
	bool IsSuspended() const { return SuspendedState.IsVoted(); }
	
	UFUNCTION(BlueprintPure, Category = "Widget Interaction")
	bool IsHovering() const { return bIsHovering; }
	
	UFUNCTION(BlueprintPure, Category = "Widget Interaction")
	bool IsSelecting() const { return bIsSelecting; }

	UFUNCTION(BlueprintPure, Category = "Widget Interaction")
	bool IsDragging() const { return bIsDragging; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "Widget Interaction")
	UWidget* GetHoveredWidgetObject() const;

	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "Widget Interaction")
	UWidget* GetSelectedWidgetObject() const;
	
	UFUNCTION(BlueprintPure, Category = "Widget Interaction")
	EInteractableWidgetType GetHoveredWidgetType() const;
	
	UFUNCTION(BlueprintPure, Category = "Widget Interaction")
	EInteractableWidgetType GetSelectedWidgetType() const { return SelectedWidgetType; }
	
	UFUNCTION(BlueprintPure, Category = "Widget Interaction")
	bool IsHoveringInteractableWidget() const;
	
	UFUNCTION(BlueprintPure, Category = "Widget Interaction")
	float GetHoverDistance() const { return HoverDistance; }
	
	UFUNCTION(BlueprintPure, Category = "Widget Interaction")
	FVector GetMousePosition() const { return CurrentMousePosition; }
	
	UFUNCTION(BlueprintPure, Category = "Widget Interaction")
	bool HasFocusedWidget(const UWidget* widget) const;

	UFUNCTION(BlueprintPure, Category = "Widget Interaction")
	double GetTimeSinceLastSelect() const { return bIsSelecting ? 0.0f : FApp::GetGameTime() - LastSelectTime; }

	UFUNCTION(BlueprintPure, Category = "Widget Interaction")
	bool IsInertialScrollEnabled() const;

	UFUNCTION(BlueprintPure, Category = "Widget Interaction")
	bool UseCustomDragScrolling() const;
	
	UFUNCTION(BlueprintPure, Category = "Widget Interaction")
	bool IsShowingInteractionDebug() const { return bShowInteractionDebug; }
	
	UFUNCTION(BlueprintCallable, Category = "Widget Interaction")
	void SetShowInteractionDebug(bool showDebug);

protected:
	
	// Enables debug rendering of this component's widget interaction state.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, BlueprintSetter = SetShowInteractionDebug, Category = "Widget Interaction|General")
	bool bShowInteractionDebug = false;

	// Specifies that the virtual user should not be unregistered when the component is disabled.
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Widget Interaction|General")
	bool bPreserveVirtualUser = false;
	
	// SWTODO - Haptics system
	/*
	// Haptic effect to play when hovering UI buttons
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Widget Interaction|Haptics")
	const UHapticEvent* HoveringHapticEvent = nullptr;

	// Haptic effect to play when pressing UI buttons
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Widget Interaction|Haptics")
	const UHapticEvent* SelectionHapticEvent = nullptr;
	*/

	// Speed multiplier for scrolling by thumbstick/mouse-wheel.
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Widget Interaction|Scrolling")
	float ScrollSpeedMultiplier = 1.0f;
	
	// Enables scrolling by dragging the mouse pointer along a widget's scroll direction.
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Widget Interaction|Scrolling")
	bool bIsDragScrollingEnabled = true;

	// Enables additional inertial scrolling after drag scrolling is released. (This is implemented natively by the engine
	// and is currently not configurable.) Disabling this feature will switch to using a custom drag-scrolling implementation
	// in place of UE's native system.
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Widget Interaction|Scrolling", meta = (EditCondition = "bIsDragScrollingEnabled"))
	bool bIsInertialScrollingEnabled = true;
	
	// Amount the mouse pointer must move before drag-scrolling will start.
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Widget Interaction|Scrolling", meta = (EditCondition = "bIsDragScrollingEnabled"))
	float MinimumMouseDeviationForDragging = 0.5f;
	
	// Called when the user starts/stops hovering some UI.
	UPROPERTY(BlueprintAssignable, Category = "Widget Interaction")
	FWidgetHoverEvent OnWidgetHovered;
	
	// Called when the user starts/stops selecting a widget.
	UPROPERTY(BlueprintAssignable, Category = "Widget Interaction")
	FWidgetSelectEvent OnWidgetSelected;

	// Called when the hovered widget type changes.
	UPROPERTY(BlueprintAssignable, Category = "Widget Interaction")
	FWidgetChangedEvent OnWidgetChanged;

	// Called when this component is bound to a specific pawn/hand.
	UPROPERTY(BlueprintAssignable, Category = "Widget Interaction")
	FInteractorBindEvent OnBind;

	// Called when this component is unbound from a specific pawn/hand.
	UPROPERTY(BlueprintAssignable, Category = "Widget Interaction")
	FInteractorBindEvent OnUnbind;
	
	// This method can be overridden to restrict hovering of specific widget components in specific conditions.
	UFUNCTION(BlueprintNativeEvent)
	bool CanHover(const UWidgetComponent* widgetComponent) const;
	virtual bool CanHover_Implementation(const UWidgetComponent* widgetComponent) const { return !IsSuspended(); }

	// This method can be overridden to restrict selection of specific widget components in specific conditions.
	UFUNCTION(BlueprintNativeEvent)
	bool CanSelect(const UWidgetComponent* widgetComponent) const;
	virtual bool CanSelect_Implementation(const UWidgetComponent* widgetComponent) const { return true; }
	
protected:
	
	// Binding to some 'interactor' component (i.e. one that implements the IDrsWidgetInteractorInterface).
	UPROPERTY(Transient)
	UActorComponent* InteractorComponent = nullptr;

	// Handle to the interactor interface, for purpose of calling interface functions.
	UPROPERTY(Transient)
	TScriptInterface<IDrsWidgetInteractorInterface> InteractorBinding = nullptr;

	// Is the currently-bound interactor associated with a specific hand?
	TOptional<EControllerHand> InteractorHand;

	// Allow more generous  arbitration between selection and drag-scrolling in some cases.
	bool bMoreGenerousDragArbitration = false;
	
	// Interaction state
	bool bIsDragging = false;
	bool bIsSelecting = false;
	bool bIsHovering = false;
	bool bIsLeftClicking = false;
	bool bIsRightClicking = false;
	bool bIsHoveringScrollableWidget = false;
	float HoverDistance = TNumericLimits<float>::Max();
	
	// Input action IDs for UI interaction.
	static inline const FName InputActionID_Select = FName("UI_Select");
	static inline const FName InputActionID_RightClick = FName("UI_RightClick");
	static inline const FName InputActionID_Scroll_Vertical = FName("UI_Scroll_Vertical");
	static inline const FName InputActionID_Scroll_Horizontal = FName("UI_Scroll_Horizontal");
	
private:

	// Callbacks
	UFUNCTION()
	void OnHoveredWidgetComponentChanged(UWidgetComponent* prev, UWidgetComponent* next);
	void OnSetupInputComponent(const APawn* pawn, UInputComponent* inputComponent);
	
	// Internal functions.
	void SetIsHovering(bool isHovering);
	void SetIsHoveringScrollableWidget(bool isHovering);
	void SetIsSelecting(bool isSelecting);
	void SetIsLeftClicking(bool isClicking, bool suppressClickEvent = false);
	void SetIsRightClicking(bool isClicking);
	bool AllowRightClicking() const;
	void UpdateHoveredState();
	void UpdateInputContext(bool enableInput);
	void UpdateScrollInputContext(bool enableInput);
	void UpdateDragging();
	void OnActiveStateChanged();
	bool HasValidBinding() const;
	void ClaimUserIndex();
	void ReleaseUserIndex();
	void ClearLastHitResult();
	bool IsInteractionConflict(const FWidgetPath& hitWidgetPath) const;
	bool IsHoveredWidgetDraggable() const;
	bool IsDragScrollPreferredOverSelection() const;
	TOptional<EOrientation> GetHoveredWidgetScrollDirection() const;
	FString GetInteractorHandString() const;

	void StartHoverHaptics();
	void EndHoverHaptics();
	void StartSelectHaptics();
	void EndSelectHaptics();
	
	TWeakPtr<SWidget> GetSelectedSlateWidget() const { return SelectedSlateWidget; }
	TWeakPtr<SWidget> GetHoveredSlateWidget() const;
	const UUserWidget* GetHoveredUserWidget() const;
	UWidget* GetSelectableWidgetObject() const;
	UWidget* GetScrollableWidgetObject() const;
	UWidget* GetWidgetObject(const TWeakPtr<SWidget>& slateWidget, const UUserWidget* userWidget) const;
	
	static EInteractableWidgetType FindHoveredSelectableWidget(const FWeakWidgetPath& widgetPath, TWeakPtr<SWidget>& outSlateWidget, const UUserWidget*& outUserWidget);
	static EInteractableWidgetType FindHoveredScrollableWidget(const FWeakWidgetPath& widgetPath, TWeakPtr<SWidget>& outSlateWidget, const UUserWidget*& outUserWidget);
	static EInteractableWidgetType FindHoveredWidget(const TMap<FName, EInteractableWidgetType>& lookup, const FWeakWidgetPath& widgetPath, TWeakPtr<SWidget>& outSlateWidget, const UUserWidget*& outUserWidget);
	static bool IsHoveringWidgetType(const FWeakWidgetPath& widgetPath, EInteractableWidgetType widgetType, const TMap<FName, EInteractableWidgetType>& widgetTypeLookup);
	bool IsHoveringSelectableWidgetType(EInteractableWidgetType widgetType) const;
	bool IsHoveringScrollableWidgetType(EInteractableWidgetType widgetType) const;
	void ConsolidateWidgetCaches();
	
	void UpdateInteractionDebug();
	void DrawInteractionDebug(const UWorld* world, float deltaTime) const;
	
	// Internal flags
	bool bIsLocallyControlled = false;
	bool bVirtualUserClaimed = false;
	bool bInputContextEnabled = false;
	bool bScrollInputContextEnabled = false;

	// Enabled state
	TVotableStateName SuspendedState;

	// Hover state
	EInteractableWidgetType SelectedWidgetType = EInteractableWidgetType::None;
	EInteractableWidgetType HoveredSelectableWidgetType = EInteractableWidgetType::None;
	EInteractableWidgetType HoveredScrollableWidgetType = EInteractableWidgetType::None;
	TWeakPtr<SWidget> SelectedSlateWidget;
	TWeakPtr<SWidget> HoveredSelectableSlateWidget;
	TWeakPtr<SWidget> HoveredScrollableSlateWidget;
	UPROPERTY()
	const UUserWidget* SelectedUserWidget = nullptr;
	UPROPERTY()
	const UUserWidget* HoveredSelectableUserWidget = nullptr;
	UPROPERTY()
	const UUserWidget* HoveredScrollableUserWidget = nullptr;
	
	// Mouse state
	FVector CurrentMousePosition = FVector::ZeroVector;
	FVector MousePositionOnSelect = FVector::ZeroVector;
	FVector MouseDragPosition = FVector::ZeroVector;
	float MouseDeviation = 0.0f;
	FVector CurrentInteractorLocalPosition = FVector::ZeroVector;
	FVector InteractorLocalPositionOnSelect = FVector::ZeroVector;
	float InteractorDeviation = 0.0f;
	
	// Selection state
	double LastSelectTime = 0.0f;
	
	// Haptics state
	FHapticId ActiveHoverHapticEventId;
	FHapticId ActiveSelectHapticEventId;

	// Input state
	UPROPERTY(Transient)
	UDrsEnhancedInputComponent* CurrentInputComponent = nullptr;

	FDelegateHandle DebugDrawHandle;
	
	// Cached SWidget->UWidget lookup.
	// We can't add UPROPERTY to protect against garbage collection of the UWidgets here. But in theory, invalidation of
	// the UWidgets should always be accompanied by invalidation of the SWidget weak pointers. So it should never be
	// possible to retrieve a dangling pointer from this map.
	// ReSharper disable once CppUE4ProbableMemoryIssuesWithUObjectsInContainer
	mutable TMap<TWeakPtr<SWidget>, UWidget*> WidgetObjectLookup;
};
