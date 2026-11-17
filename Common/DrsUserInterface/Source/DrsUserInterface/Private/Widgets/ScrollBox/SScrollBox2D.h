// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "Framework/Layout/Overscroll.h"
#include "Input/CursorReply.h"
#include "Input/NavigationReply.h"
#include "Input/Reply.h"
#include "Layout/Children.h"
#include "Layout/Geometry.h"
#include "Layout/Visibility.h"
#include "Layout/WidgetSlotWithAttributeSupport.h"
#include "SlotBase.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateColor.h"
#include "Styling/SlateTypes.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Layout/SScrollBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SPanel.h"
#include "Widgets/SWidget.h"

class FPaintArgs;
class FSlateWindowElementList;
class SScrollPanel2D;

DECLARE_DELEGATE_OneParam(FOnUserScrolled2D, const FVector2D&);	/** ScrollOffset as a fraction between 0 and 1 */


/**
 * Scrollbox widget that scrolls in two-dimensions.
 */
class DRSUSERINTERFACE_API SScrollBox2D : public SCompoundWidget
{
public:

#pragma region Slot
	// Slot that provides layout options for the contents of a scrollable box.
	class FSlot : public TBasicLayoutWidgetSlot<FSlot>
	{
	public:
		
		SLATE_SLOT_BEGIN_ARGS(FSlot, TBasicLayoutWidgetSlot<FSlot>)
			SLATE_ARGUMENT(TOptional<FSizeParam>, SizeParam)
			TAttribute<float> _MaxSize;
			FSlotArguments& AutoSize() {_SizeParam = FAuto(); return Me(); }
			FSlotArguments& FillSize(TAttribute<float> inStretchCoefficient) {_SizeParam = FStretch(MoveTemp(inStretchCoefficient)); return Me();}
			FSlotArguments& MaxSize(TAttribute<float> inMaxHeight) {_MaxSize = MoveTemp(inMaxHeight); return Me();}
		SLATE_SLOT_END_ARGS()
		
		FSlot();
		void Construct(const FChildren& slotOwner, FSlotArguments&& inArgs);
		static void RegisterAttributes(FSlateWidgetSlotAttributeInitializer& attributeInitializer);
		
		FSizeParam::ESizeRule GetSizeRule() const { return SizeRule; }
		float GetSizeValue() const { return SizeValue.Get(); }
		float GetMaxSize() const { return MaxSize.Get(); }
		FVector2f GetSizeValue2D() const { return FVector2f(SizeValue.Get(), SizeValue.Get()); }
		FVector2f GetMaxSize2D() const { return FVector2f(MaxSize.Get()); }
		void SetSizeParam(FSizeParam inSizeParam) { SizeRule = inSizeParam.SizeRule; SizeValue.Assign(*this, MoveTemp(inSizeParam.Value)); }
		void SetSizeToAuto() { SetSizeParam(FAuto()); }
		void SetSizeToStretch(TAttribute<float> stretchCoefficient) { SetSizeParam(FStretch(MoveTemp(stretchCoefficient))); }
		void SetMaxSize(TAttribute<float> inMaxSize) { MaxSize.Assign(*this, MoveTemp(inMaxSize)); }

	private:

		FSizeParam::ESizeRule SizeRule;
		typename TBasicLayoutWidgetSlot<FSlot>::template TSlateSlotAttribute<float> SizeValue;
		typename TBasicLayoutWidgetSlot<FSlot>::template TSlateSlotAttribute<float> MaxSize;
	};
#pragma endregion Slot

	SLATE_BEGIN_ARGS(SScrollBox2D)
		: _Style( &FAppStyle::Get().GetWidgetStyle<FScrollBoxStyle>("ScrollBox") )
		, _ScrollBarStyle( &FAppStyle::Get().GetWidgetStyle<FScrollBarStyle>("ScrollBar") )
		, _ScrollBarVisibility(EVisibility::Visible)
		, _ScrollBarAlwaysVisible(false)
		, _ScrollBarDragFocusCause(EFocusCause::Mouse)
		, _ScrollBarThickness(FVector2f(_Style->BarThickness, _Style->BarThickness))
		, _ScrollBarPadding(2.0f)
		, _AllowOverscroll(EAllowOverscroll::Yes)
		, _BackPadScrolling(false)
		, _FrontPadScrolling(false)
		, _AnimateWheelScrolling(false)
		, _WheelScrollMultiplier(1.f)
		, _NavigationDestination(EDescendantScrollDestination::IntoView)
		, _NavigationScrollPadding(0.0f)
		, _ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
		, _ConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible)
		{
			_Clipping = EWidgetClipping::ClipToBounds;
		}
		
		SLATE_SLOT_ARGUMENT(FSlot, Slots)
		SLATE_STYLE_ARGUMENT(FScrollBoxStyle, Style)
		SLATE_STYLE_ARGUMENT(FScrollBarStyle, ScrollBarStyle)
		SLATE_ARGUMENT(EVisibility, ScrollBarVisibility)
		SLATE_ARGUMENT(bool, ScrollBarAlwaysVisible)
		SLATE_ARGUMENT(EFocusCause, ScrollBarDragFocusCause)
		SLATE_ARGUMENT(UE::Slate::FDeprecateVector2DParameter, ScrollBarThickness)
		SLATE_ARGUMENT(FMargin, ScrollBarPadding)
		SLATE_ARGUMENT(EAllowOverscroll, AllowOverscroll);
		SLATE_ARGUMENT(bool, BackPadScrolling);
		SLATE_ARGUMENT(bool, FrontPadScrolling);
		SLATE_ARGUMENT(bool, AnimateWheelScrolling);
		SLATE_ARGUMENT(FVector2D, WheelScrollMultiplier);
		SLATE_ARGUMENT(EOrientation, ArrangementDirection);
		SLATE_ARGUMENT(EDescendantScrollDestination, NavigationDestination);
		SLATE_ARGUMENT(float, NavigationScrollPadding);
		SLATE_ARGUMENT(EScrollWhenFocusChanges, ScrollWhenFocusChanges);
		SLATE_EVENT(FOnUserScrolled2D, OnUserScrolled)
		SLATE_ARGUMENT(EConsumeMouseWheel, ConsumeMouseWheel);
	SLATE_END_ARGS()

	SScrollBox2D();

	void Construct( const FArguments& inArgs );
	static FSlot::FSlotArguments Slot();
	
	using FScopedWidgetSlotArguments = TPanelChildren<FSlot>::FScopedWidgetSlotArguments;
	FScopedWidgetSlotArguments AddSlot();
	void RemoveSlot(const TSharedRef<SWidget>& widgetToRemove);
	void ClearChildren();
	
	void SetDragScrollingEnabled(bool isAllowed) { bAllowsRightClickDragScrolling = isAllowed; }
	bool IsDragScrollingEnabled() const { return bAllowsRightClickDragScrolling; }
	bool IsRightClickDragScrolling() const;
	void SetWheelScrollMultiplier(const FVector2D& newWheelScrollMultiplier) { WheelScrollMultiplier = newWheelScrollMultiplier; }
	FVector2D GetWheelScrollMultiplier() const { return WheelScrollMultiplier; }
	void SetAnimateWheelScrolling(bool inAnimateWheelScrolling) { bAnimateWheelScrolling = inAnimateWheelScrolling; }
	void SetScrollWhenFocusChanges(EScrollWhenFocusChanges newScrollWhenFocusChanges) { ScrollWhenFocusChanges = newScrollWhenFocusChanges; }
	void SetAllowOverscroll(EAllowOverscroll newAllowOverscroll);
	void SetScrollOffset(const FVector2D& newScrollOffset);
	void SetNavigationDestination(const EDescendantScrollDestination newNavigationDestination) { NavigationDestination = newNavigationDestination; }
	void SetConsumeMouseWheel(EConsumeMouseWheel newConsumeMouseWheel) { ConsumeMouseWheel = newConsumeMouseWheel; }
	void SetScrollBarVisibility(EVisibility inVisibility);
	void SetScrollBarAlwaysVisible(bool inAlwaysVisible);
	void SetScrollBarTrackAlwaysVisible(bool inAlwaysVisible);
	void SetScrollBarThickness(UE::Slate::FDeprecateVector2DParameter inThickness);
	void SetScrollBarPadding(const FMargin& inPadding);
	void SetStyle(const FScrollBoxStyle* inStyle);
	void SetScrollBarStyle(const FScrollBarStyle* inBarStyle);
	void InvalidateStyle();
	void InvalidateScrollBarStyle();

	FVector2D GetScrollOffset() const { return DesiredScrollOffset; }
	FVector2D GetScrollOffsetOfEnd() const;
	FVector2D GetViewFraction() const;
	FVector2D GetViewOffsetFraction() const;
	
	void ScrollVertical(float delta);
	void ScrollHorizontal(float delta);
	void Scroll2D(const FVector2D& delta);
	void DragScroll2D(const FVector2D& delta);
	void ScrollToStart();
	void ScrollToEnd();
	void ScrollDescendantIntoView(const TSharedPtr<SWidget>& widgetToFind, bool inAnimateScroll = true, EDescendantScrollDestination inDestination = EDescendantScrollDestination::IntoView, float inScrollPadding = 0.f);
	void EndInertialScrolling();

protected:
	
	// Begin SWidget overrides.
	virtual void Tick( const FGeometry& allottedGeometry, const double inCurrentTime, const float inDeltaTime ) override;
	virtual bool ComputeVolatility() const override;
	virtual FReply OnPreviewMouseButtonDown(const FGeometry& myGeometry, const FPointerEvent& mouseEvent) override;
	virtual FReply OnMouseButtonDown( const FGeometry& myGeometry, const FPointerEvent& mouseEvent ) override;
	virtual FReply OnMouseButtonUp( const FGeometry& myGeometry, const FPointerEvent& mouseEvent ) override;
	virtual FReply OnMouseMove( const FGeometry& myGeometry, const FPointerEvent& mouseEvent ) override;
	virtual void OnMouseEnter( const FGeometry& myGeometry, const FPointerEvent& mouseEvent ) override;
	virtual void OnMouseLeave( const FPointerEvent& mouseEvent ) override;
	virtual FReply OnMouseWheel( const FGeometry& myGeometry, const FPointerEvent& mouseEvent ) override;
	virtual FCursorReply OnCursorQuery( const FGeometry& myGeometry, const FPointerEvent& cursorEvent ) const override;
	virtual int32 OnPaint( const FPaintArgs& args, const FGeometry& allottedGeometry, const FSlateRect& myCullingRect, FSlateWindowElementList& outDrawElements, int32 layerId, const FWidgetStyle& inWidgetStyle, bool parentEnabled ) const override;
	virtual FReply OnTouchEnded(const FGeometry& myGeometry, const FPointerEvent& inTouchEvent) override;
	virtual void OnMouseCaptureLost(const FCaptureLostEvent& captureLostEvent) override;
	virtual FNavigationReply OnNavigation(const FGeometry& myGeometry, const FNavigationEvent& inNavigationEvent) override;
	virtual void OnFocusChanging(const FWeakWidgetPath& previousFocusPath, const FWidgetPath& newWidgetPath, const FFocusEvent& inFocusEvent) override;
	virtual void OnClippingChanged() override;
	// End SWidget overrides.

private:
	
	void ConstructLayout();
	TSharedPtr<SScrollBar> ConstructScrollBar(EOrientation orientation);
	TSharedPtr<SWidget> GetKeyboardFocusableWidget(TSharedPtr<SWidget> inWidget);
	
	bool ScrollBy(const FGeometry& allottedGeometry, const FVector2D& localScrollAmount, EAllowOverscroll overscrolling, bool inAnimateScroll);
	void ScrollBar_OnUserScrolled(float inScrollOffsetFraction, int32 directionComponent);
	void ScrollBar_OnUserScrolledHorizontally(float inScrollOffsetFraction);
	void ScrollBar_OnUserScrolledVertically(float inScrollOffsetFraction);
	bool InternalScrollDescendantIntoView(const FGeometry& myGeometry, const TSharedPtr<SWidget>& widgetToFind, bool inAnimateScroll = true, EDescendantScrollDestination inDestination = EDescendantScrollDestination::IntoView, float inScrollPadding = 0.f);
	
	FSlateColor GetStartShadowOpacity(int32 directionComponent) const;
	FSlateColor GetEndShadowOpacity(int32 directionComponent, float distanceFromBottom) const;
	FSlateColor GetStartShadowOpacity_Horizontal() const;
	FSlateColor GetEndShadowOpacity_Horizontal() const;
	FSlateColor GetStartShadowOpacity_Vertical() const;
	FSlateColor GetEndShadowOpacity_Vertical() const;

	bool CanUseInertialScroll(const FVector2D& scrollAmount) const;
	void BeginInertialScrolling();
	EActiveTimerReturnType UpdateInertialScroll(double inCurrentTime, float inDeltaTime);
	
	// Fired when the user scrolls the scrollbox.
	FOnUserScrolled2D OnUserScrolled;
	
	SHorizontalBox::FSlot* VerticalScrollBarSlot = nullptr;
	SVerticalBox::FSlot* HorizontalScrollBarSlot = nullptr;
	TSharedPtr<SScrollPanel2D> ScrollPanel;
	TSharedPtr<SScrollBar> HorizontalScrollBar;
	TSharedPtr<SScrollBar> VerticalScrollBar;
	const FScrollBoxStyle* Style = nullptr;
	const FScrollBarStyle* ScrollBarStyle = nullptr;
	
	FVector2D AmountScrolledWhileRightMouseDown = FVector2D::ZeroVector;
	FVector2D PendingScrollTriggerAmount = FVector2D::ZeroVector;
	FVector2f SoftwareCursorPosition = FVector2f::ZeroVector;

	FMargin ScrollBarSlotPadding;
	FGeometry CachedGeometry;
	EOrientation ArrangementDirection = Orient_Vertical;
	EDescendantScrollDestination NavigationDestination = EDescendantScrollDestination::IntoView;
	EScrollWhenFocusChanges ScrollWhenFocusChanges = EScrollWhenFocusChanges::NoScroll;
	EConsumeMouseWheel ConsumeMouseWheel = EConsumeMouseWheel::WhenScrollingPossible;
	TOptional<int32> bFingerOwningTouchInteraction;
	TFunction<void(FGeometry)> ScrollIntoViewRequest;

	// TODO: Consider creating a custom 2D version of FInertialScrollManager.
	FInertialScrollManager InertialScrollManagerHorizontal;
	FInertialScrollManager InertialScrollManagerVertical;
	TSharedPtr<FActiveTimerHandle> UpdateInertialScrollHandle;

	// TODO: Consider creating a custom 2D version of FOverscroll.
	EAllowOverscroll AllowOverscroll = EAllowOverscroll::No;
	FOverscroll OverscrollHorizontal;
	FOverscroll OverscrollVertical;
	
	FVector2D DesiredScrollOffset = FVector2D::ZeroVector;
	FVector2D WheelScrollMultiplier = FVector2D::One();
	static constexpr float WheelScrollSpeed = 20.f;
	double LastScrollTime = 0;
	float NavigationScrollPadding = 0.0f;

	bool bBackPadScrolling = false;
	bool bFrontPadScrolling = false;
	bool bAnimateWheelScrolling = false;
	bool bShowSoftwareCursor = false;
	bool bIsScrolling = false;
	bool bAnimateScroll = false;
	bool bScrollToEnd = false;
	bool bAllowsRightClickDragScrolling = false;
	bool bTouchPanningCapture = false;
	bool bIsScrollingActiveTimerRegistered = false;
};


class DRSUSERINTERFACE_API SScrollPanel2D : public SPanel
{
public:

	SLATE_BEGIN_ARGS(SScrollPanel2D)
	{
		_Visibility = EVisibility::SelfHitTestInvisible;
	}
	SLATE_ARGUMENT(bool, BackPadScrolling)
	SLATE_ARGUMENT(bool, FrontPadScrolling)
	SLATE_ARGUMENT(EOrientation, ArrangementDirection)
	SLATE_END_ARGS()

	SScrollPanel2D() : Children(this) {}
	
	void Construct(const FArguments& inArgs, TArray<SScrollBox2D::FSlot::FSlotArguments> inSlots);

protected:
	
	// Begin SWidget overrides.
	virtual void OnArrangeChildren(const FGeometry& allottedGeometry, FArrangedChildren& arrangedChildren) const override;
	virtual FChildren* GetChildren() override { return &Children; }
	virtual FVector2D ComputeDesiredSize(float) const override;
	// End SWidget overrides.

private:

	friend class SScrollBox2D;
	
	bool BackPadScrolling = false;
	bool FrontPadScrolling = false;
	EOrientation ArrangementDirection = Orient_Vertical;
	FVector2D PhysicalOffset = FVector2D::ZeroVector;
	TPanelChildren<SScrollBox2D::FSlot> Children;
};
