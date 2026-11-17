// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "Components/PanelWidget.h"
#include "Containers/Ticker.h"
#include "Interfaces/SlateLifecycleAware.h"
#include "Styling/SlateTypes.h"
#include "Utils/WidgetUtils.h"
#include "Widgets/ScrollBox/SScrollBox2D.h"
#include "Widgets/SWidget.h"
#include "ScrollBox2D.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUserScrolled2DEvent, const FVector2D&, currentOffset);


/**
 * Version of Scrollbox widget that scrolls in two-dimensions.
 */
UCLASS()
class DRSUSERINTERFACE_API UScrollBox2D : public UPanelWidget, public ISlateLifecycleAware
{
	GENERATED_UCLASS_BODY()

public:
	
	FORCEINLINE virtual UWorld* GetWorld() const override
	{
		return WidgetUtils::GetWorld(GetOuter());
	}

	/** Called when the scroll has changed */
	UPROPERTY(BlueprintAssignable, Category = "Button|Event")
	FOnUserScrolled2DEvent OnUserScrolled;
	
	void SetWidgetStyle(const FScrollBoxStyle& newWidgetStyle);
	const FScrollBoxStyle& GetWidgetStyle() const { return WidgetStyle; }
	
	void SetWidgetBarStyle(const FScrollBarStyle& newWidgetBarStyle);
	const FScrollBarStyle& GetWidgetBarStyle() const;

	UFUNCTION(BlueprintCallable, Category = "Scroll")
	void SetConsumeMouseWheel(EConsumeMouseWheel newConsumeMouseWheel);
	EConsumeMouseWheel GetConsumeMouseWheel() const { return ConsumeMouseWheel; }

	UFUNCTION(BlueprintCallable, Category = "Scroll")
	void SetScrollBarVisibility(ESlateVisibility newScrollBarVisibility);
	ESlateVisibility GetScrollBarVisibility() const { return ScrollBarVisibility; }

	UFUNCTION(BlueprintCallable, Category = "Scroll")
	void SetScrollbarThickness(const FVector2D& newScrollbarThickness);
	FVector2D GetScrollbarThickness() const { return ScrollbarThickness; }

	UFUNCTION(BlueprintCallable, Category = "Scroll")
	void SetScrollbarPadding(const FMargin& newScrollbarPadding);
	FMargin GetScrollbarPadding() const { return ScrollbarPadding; }

	UFUNCTION(BlueprintCallable, Category = "Scroll")
	void SetAlwaysShowScrollbar(bool newAlwaysShowScrollbar);
	bool IsAlwaysShowScrollbar() const { return AlwaysShowScrollbar; }

	UFUNCTION(BlueprintCallable, Category = "Scroll")
	void SetAllowOverscroll(bool newAllowOverscroll);
	bool IsAllowOverscroll() const { return AllowOverscroll; }

	UFUNCTION(BlueprintCallable, Category = "Scroll")
	void SetAnimateWheelScrolling(bool shouldAnimateWheelScrolling);
	bool IsAnimateWheelScrolling() const { return bAnimateWheelScrolling; }

	UFUNCTION(BlueprintCallable, Category = "Scroll")
	void SetWheelScrollMultiplier(const FVector2D& newWheelScrollMultiplier);
	FVector2D GetWheelScrollMultiplier() const { return WheelScrollMultiplier; }

	UFUNCTION(BlueprintCallable, Category = "Scroll")
	void SetScrollWhenFocusChanges(EScrollWhenFocusChanges newScrollWhenFocusChanges);
	EScrollWhenFocusChanges GetScrollWhenFocusChanges() const { return ScrollWhenFocusChanges; }

	UFUNCTION(BlueprintCallable, Category = "Scroll")
	void SetNavigationDestination(const EDescendantScrollDestination newNavigationDestination);
	EDescendantScrollDestination GetNavigationDestination() const { return NavigationDestination; }

	/** Instantly stops any inertial scrolling that is currently in progress */
	UFUNCTION(BlueprintCallable, Category = "Scroll")
	void EndInertialScrolling();
	
	/**
	 * Updates the scroll offset of the scrollbox.
	 * @param newScrollOffset is in Slate Units.
	 */
	UFUNCTION(BlueprintCallable, Category="Widget")
	void SetScrollOffset(const FVector2D& newScrollOffset);
	
	/**
	 * Gets the scroll offset of the scrollbox in Slate Units.
	 */
	UFUNCTION(BlueprintCallable, Category="Widget")
	FVector2D GetScrollOffset() const;

	/** Gets the scroll offset of the bottom of the ScrollBox in Slate Units. */
	UFUNCTION(BlueprintCallable, Category = "Widget")
	FVector2D GetScrollOffsetOfEnd() const;

	/** Gets the fraction currently visible in the scrollbox */
	UFUNCTION(BlueprintCallable, Category="Widget")
	FVector2D GetViewFraction() const;

	UFUNCTION(BlueprintCallable, Category="Widget")
	FVector2D GetViewOffsetFraction() const;

	/** Scrolls the ScrollBox to the top instantly */
	UFUNCTION(BlueprintCallable, Category="Widget")
	void ScrollToStart();

	/** Scrolls the ScrollBox to the bottom instantly during the next layout pass. */
	UFUNCTION(BlueprintCallable, Category="Widget")
	void ScrollToEnd();

	/** Scrolls the ScrollBox to the widget during the next layout pass. */
	UFUNCTION(BlueprintCallable, Category="Widget")
	void ScrollWidgetIntoView(UWidget* widgetToFind, bool animateScroll = true, EDescendantScrollDestination scrollDestination = EDescendantScrollDestination::IntoView, float padding = 0);
	
	void SetAlwaysShowScrollbarTrack(bool newAlwaysShowScrollbarTrack);
	bool IsAlwaysShowScrollbarTrack() const { return AlwaysShowScrollbarTrack; }

	void SetAllowsRightClickDragScrolling(bool shouldAllowRightClickDragScrolling);
	bool AllowsRightClickDragScrolling() const { return bAllowRightClickDragScrolling; }
	float GetNavigationScrollPadding() const { return NavigationScrollPadding; }
	bool IsFrontPadScrolling() const { return FrontPadScrolling; }
	bool IsBackPadScrolling() const { return BackPadScrolling; }
	
protected:
	
	/** The style */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter, Setter, Category="Style", meta=( DisplayName="Style" ))
	FScrollBoxStyle WidgetStyle;
	
	/** The bar style */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter, Setter, Category="Style", meta=( DisplayName="Bar Style" ))
	FScrollBarStyle WidgetBarStyle;

	/** Arrangement direction for scroll-box slots. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scroll")
	TEnumAsByte<EOrientation> ArrangementDirection = Orient_Vertical;
	
	/** Visibility */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter, Setter, BlueprintSetter = "SetScrollBarVisibility", Category="Scroll")
	ESlateVisibility ScrollBarVisibility = ESlateVisibility::Visible;
	
	/** When mouse wheel events should be consumed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter, Setter, BlueprintSetter = "SetConsumeMouseWheel", Category = "Scroll")
	EConsumeMouseWheel ConsumeMouseWheel = EConsumeMouseWheel::WhenScrollingPossible;
	
	/** The thickness of the scrollbar thumb */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter, Setter, BlueprintSetter = "SetScrollbarThickness", Category="Scroll")
	FVector2D ScrollbarThickness = FVector2D(9.0f, 9.0f);
	
	/** The margin around the scrollbar */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter, Setter, BlueprintSetter = "SetScrollbarPadding", Category = "Scroll")
	FMargin ScrollbarPadding;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter = "IsAlwaysShowScrollbar", Setter, BlueprintSetter = "SetAlwaysShowScrollbar", Category = "Scroll")
	bool AlwaysShowScrollbar = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter = "IsAlwaysShowScrollbarTrack", Setter, Category = "Scroll")
	bool AlwaysShowScrollbarTrack = false;
	
	/**  Disable to stop scrollbars from activating inertial overscrolling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter = "IsAllowOverscroll", Setter, BlueprintSetter = "SetAllowOverscroll", Category = "Scroll")
	bool AllowOverscroll = true;

	/** Whether to back pad this scroll box, allowing user to scroll backward until child contents are no longer visible */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Getter = "IsBackPadScrolling", Category = "Scroll")
	bool BackPadScrolling = false;

	/** Whether to front pad this scroll box, allowing user to scroll forward until child contents are no longer visible */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Getter = "IsFrontPadScrolling", Category = "Scroll")
	bool FrontPadScrolling = false;
	
	/** True to lerp smoothly when wheel scrolling along the scroll box */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter = "IsAnimateWheelScrolling", Setter = "SetAnimateWheelScrolling", BlueprintSetter = "SetAnimateWheelScrolling", Category = "Scroll")
	bool bAnimateWheelScrolling = false;

	/** Sets where to scroll a widget to when using explicit navigation or if ScrollWhenFocusChanges is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter, Setter, BlueprintSetter = "SetNavigationDestination", Getter, Setter, Category = "Scroll")
	EDescendantScrollDestination NavigationDestination = EDescendantScrollDestination::IntoView;

	/**
	 * The amount of padding to ensure exists between the item being navigated to, at the edge of the
	 * scrollbox.  Use this if you want to ensure there's a preview of the next item the user could scroll to.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Getter, Category="Scroll")
	float NavigationScrollPadding = 0.0f;

	/** Scroll behavior when user focus is given to a child widget */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter, Setter, BlueprintSetter = "SetScrollWhenFocusChanges", Category="Scroll", meta=(DisplayName="Scroll When Focus Changes"))
	EScrollWhenFocusChanges ScrollWhenFocusChanges = EScrollWhenFocusChanges::NoScroll;
	
	/** Option to disable right-click-drag scrolling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter = "AllowsRightClickDragScrolling", Setter = "SetAllowsRightClickDragScrolling", Category = "Scroll")
	bool bAllowRightClickDragScrolling = false;

	/** The multiplier to apply when wheel scrolling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter, Setter, BlueprintSetter = "SetWheelScrollMultiplier", Category = "Scroll")
	FVector2D WheelScrollMultiplier = FVector2D::One();

protected:
	
	//~ Begin UWidget Interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;
#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
	virtual void OnDescendantSelectedByDesigner(UWidget* descendantWidget) override;
	virtual void OnDescendantDeselectedByDesigner(UWidget* descendantWidget) override;
#endif
	//~ End UWidget Interface

	//~ Begin UVisual Interface
	virtual void ReleaseSlateResources(bool releaseChildren) override;
	//~ End UVisual Interface
	
	// UPanelWidget Interface
	virtual UClass* GetSlotClass() const override;
	virtual void OnSlotAdded(UPanelSlot* slot) override;
	virtual void OnSlotRemoved(UPanelSlot* slot) override;
	// End UPanelWidget

	void SlateHandleUserScrolled(const FVector2D& currentOffset);
	void InitBackPadScrolling(bool inBackPadScrolling);
	void InitFrontPadScrolling(bool inFrontPadScrolling);
	void InitNavigationScrollPadding(float inNavigationScrollPadding);
	
	FVector2D DesiredScrollOffset = FVector2D::ZeroVector;
	TSharedPtr<SScrollBox2D> MyScrollBox;
#if WITH_EDITOR
	FTSTicker::FDelegateHandle TickHandle;
#endif //WITH_EDITOR
};
