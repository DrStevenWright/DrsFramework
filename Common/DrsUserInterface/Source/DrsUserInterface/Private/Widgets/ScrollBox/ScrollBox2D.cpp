// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#include "Widgets/ScrollBox/ScrollBox2D.h"

#include "Containers/Ticker.h"
#include "Styling/DefaultStyleCache.h"
#include "UObject/EditorObjectVersion.h"
#include "Widgets/ScrollBox/ScrollBoxSlot2D.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ScrollBox2D)


UScrollBox2D::UScrollBox2D(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
	, ScrollbarPadding(2.0f)
{
	bIsVariable = false;

	SetVisibilityInternal(ESlateVisibility::Visible);
	SetClipping(EWidgetClipping::ClipToBounds);
	
	WidgetStyle = UE::Slate::Private::FDefaultStyleCache::GetRuntime().GetScrollBoxStyle();
	WidgetBarStyle = UE::Slate::Private::FDefaultStyleCache::GetRuntime().GetScrollBarStyle();

#if WITH_EDITOR 
	if (IsEditorWidget())
	{
		WidgetStyle = UE::Slate::Private::FDefaultStyleCache::GetEditor().GetScrollBoxStyle();
		WidgetBarStyle = UE::Slate::Private::FDefaultStyleCache::GetEditor().GetScrollBarStyle();

		// The CDO isn't an editor widget and thus won't use the editor style, call post edit change to mark difference from CDO
		PostEditChange();
	}
#endif // WITH_EDITOR

	bAllowRightClickDragScrolling = true;
}

void UScrollBox2D::ReleaseSlateResources(bool releaseChildren)
{
	NotifySlateResourcesReleased(this);
	
	Super::ReleaseSlateResources(releaseChildren);

	MyScrollBox.Reset();
}

UClass* UScrollBox2D::GetSlotClass() const
{
	return UScrollBoxSlot2D::StaticClass();
}

void UScrollBox2D::OnSlotAdded(UPanelSlot* inSlot)
{
	// Add the child to the live canvas if it already exists
	if (MyScrollBox.IsValid())
	{
		CastChecked<UScrollBoxSlot2D>(inSlot)->BuildSlot(MyScrollBox.ToSharedRef());
	}
}

void UScrollBox2D::OnSlotRemoved(UPanelSlot* inSlot)
{
	// Remove the widget from the live slot if it exists.
	if (MyScrollBox.IsValid() && inSlot->Content)
	{
		const TSharedPtr<SWidget> widget = inSlot->Content->GetCachedWidget();
		if (widget.IsValid())
		{
			MyScrollBox->RemoveSlot(widget.ToSharedRef());
		}
	}
}

TSharedRef<SWidget> UScrollBox2D::RebuildWidget()
{
	MyScrollBox = SNew(SScrollBox2D)
		.Style(&WidgetStyle)
		.ScrollBarStyle(&WidgetBarStyle)
		.ConsumeMouseWheel(ConsumeMouseWheel)
		.ArrangementDirection(ArrangementDirection)
		.NavigationDestination(NavigationDestination)
		.NavigationScrollPadding(NavigationScrollPadding)
		.ScrollWhenFocusChanges(ScrollWhenFocusChanges)
		.BackPadScrolling(BackPadScrolling)
		.FrontPadScrolling(FrontPadScrolling)
		.AnimateWheelScrolling(bAnimateWheelScrolling)
		.WheelScrollMultiplier(WheelScrollMultiplier)
		.OnUserScrolled(BIND_UOBJECT_DELEGATE(FOnUserScrolled2D, SlateHandleUserScrolled));
	
	for (UPanelSlot* panelSlot : Slots)
	{
		if (UScrollBoxSlot2D* typedSlot = Cast<UScrollBoxSlot2D>(panelSlot))
		{
			typedSlot->Parent = this;
			typedSlot->BuildSlot(MyScrollBox.ToSharedRef());
		}
	}
	
	return MyScrollBox.ToSharedRef();
}

void UScrollBox2D::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (!MyScrollBox.IsValid())
	{
		return;
	}
	
	MyScrollBox->SetScrollOffset(DesiredScrollOffset);
	MyScrollBox->SetScrollBarVisibility(UWidget::ConvertSerializedVisibilityToRuntime(ScrollBarVisibility));
	MyScrollBox->SetScrollBarThickness(ScrollbarThickness);
	MyScrollBox->SetScrollBarPadding(ScrollbarPadding);
	MyScrollBox->SetScrollBarAlwaysVisible(AlwaysShowScrollbar);
	MyScrollBox->SetScrollBarTrackAlwaysVisible(AlwaysShowScrollbarTrack);
	MyScrollBox->SetAllowOverscroll(AllowOverscroll ? EAllowOverscroll::Yes : EAllowOverscroll::No);
	MyScrollBox->SetDragScrollingEnabled(bAllowRightClickDragScrolling);
	MyScrollBox->SetConsumeMouseWheel(ConsumeMouseWheel);
	MyScrollBox->SetAnimateWheelScrolling(bAnimateWheelScrolling);
	MyScrollBox->SetWheelScrollMultiplier(WheelScrollMultiplier);
	MyScrollBox->SetStyle(&WidgetStyle);
	MyScrollBox->InvalidateStyle();
	MyScrollBox->SetScrollBarStyle(&WidgetBarStyle);
	MyScrollBox->InvalidateScrollBarStyle();
}

FVector2D UScrollBox2D::GetScrollOffset() const
{
	if (MyScrollBox.IsValid())
	{
		return MyScrollBox->GetScrollOffset();
	}

	return FVector2D::ZeroVector;
}

FVector2D UScrollBox2D::GetScrollOffsetOfEnd() const
{
	if (MyScrollBox.IsValid())
	{
		return MyScrollBox->GetScrollOffsetOfEnd();
	}

	return FVector2D::ZeroVector;
}

FVector2D UScrollBox2D::GetViewFraction() const
{
	if (MyScrollBox.IsValid())
	{
		return MyScrollBox->GetViewFraction();
	}

	return FVector2D::ZeroVector;
}

FVector2D UScrollBox2D::GetViewOffsetFraction() const
{
	if (MyScrollBox.IsValid())
	{
		return MyScrollBox->GetViewOffsetFraction();
	}

	return FVector2D::ZeroVector;
}

void UScrollBox2D::SetScrollOffset(const FVector2D& newScrollOffset)
{
	DesiredScrollOffset = newScrollOffset;

	if (MyScrollBox.IsValid())
	{
		MyScrollBox->SetScrollOffset(newScrollOffset);
	}
}

void UScrollBox2D::ScrollToStart()
{
	if (MyScrollBox.IsValid())
	{
		MyScrollBox->ScrollToStart();
	}
}

void UScrollBox2D::ScrollToEnd()
{
	if (MyScrollBox.IsValid())
	{
		MyScrollBox->ScrollToEnd();
	}
}

void UScrollBox2D::ScrollWidgetIntoView(UWidget* widgetToFind, bool animateScroll, EDescendantScrollDestination InScrollDestination, float padding)
{
	TSharedPtr<SWidget> slateWidgetToFind;
	if (widgetToFind)
	{
		slateWidgetToFind = widgetToFind->GetCachedWidget();
	}

	if (MyScrollBox.IsValid())
	{
		// NOTE: Pass even if null! This, in effect, cancels a request to scroll which is necessary to avoid warnings/ensures 
		//       when we request to scroll to a widget and later remove that widget!
		MyScrollBox->ScrollDescendantIntoView(slateWidgetToFind, animateScroll, InScrollDestination, padding);
	}
}

void UScrollBox2D::SetWidgetStyle(const FScrollBoxStyle& newWidgetStyle)
{
	WidgetStyle = newWidgetStyle;

	if (MyScrollBox.IsValid())
	{
		MyScrollBox->InvalidateStyle();
	}
}

void UScrollBox2D::SetWidgetBarStyle(const FScrollBarStyle& newWidgetBarStyle)
{
	WidgetBarStyle = newWidgetBarStyle;

	if (MyScrollBox.IsValid())
	{
		MyScrollBox->InvalidateScrollBarStyle();
	}
}

const FScrollBarStyle& UScrollBox2D::GetWidgetBarStyle() const
{
	return WidgetBarStyle;
}

void UScrollBox2D::SetNavigationDestination(const EDescendantScrollDestination newNavigationDestination)
{
	NavigationDestination = newNavigationDestination;

	if (MyScrollBox.IsValid())
	{
		MyScrollBox->SetNavigationDestination(newNavigationDestination);
	}
}

void UScrollBox2D::SetConsumeMouseWheel(EConsumeMouseWheel newConsumeMouseWheel)
{
	ConsumeMouseWheel = newConsumeMouseWheel;

	if (MyScrollBox.IsValid())
	{
		MyScrollBox->SetConsumeMouseWheel(newConsumeMouseWheel);
	}
}

void UScrollBox2D::SetScrollBarVisibility(ESlateVisibility newScrollBarVisibility)
{
	ScrollBarVisibility = newScrollBarVisibility;

	if (MyScrollBox.IsValid())
	{
		switch (ScrollBarVisibility)
		{
		case ESlateVisibility::Collapsed:
			MyScrollBox->SetScrollBarVisibility(EVisibility::Collapsed);
			break;
		case ESlateVisibility::Hidden:
			MyScrollBox->SetScrollBarVisibility(EVisibility::Hidden);
			break;
		case ESlateVisibility::HitTestInvisible:
			MyScrollBox->SetScrollBarVisibility(EVisibility::HitTestInvisible);
			break;
		case ESlateVisibility::SelfHitTestInvisible:
			MyScrollBox->SetScrollBarVisibility(EVisibility::SelfHitTestInvisible);
			break;
		case ESlateVisibility::Visible:
			MyScrollBox->SetScrollBarVisibility(EVisibility::Visible);
			break;
		}
	}
}

void UScrollBox2D::SetScrollbarThickness(const FVector2D& newScrollbarThickness)
{
	ScrollbarThickness = newScrollbarThickness;

	if (MyScrollBox.IsValid())
	{
		MyScrollBox->SetScrollBarThickness(ScrollbarThickness);
	}
}

void UScrollBox2D::SetScrollbarPadding(const FMargin& newScrollbarPadding)
{
	ScrollbarPadding = newScrollbarPadding;

	if (MyScrollBox.IsValid())
	{
		MyScrollBox->SetScrollBarPadding(ScrollbarPadding);
	}
}

void UScrollBox2D::SetAlwaysShowScrollbar(bool newAlwaysShowScrollbar)
{
	AlwaysShowScrollbar = newAlwaysShowScrollbar;

	if (MyScrollBox.IsValid())
	{
		MyScrollBox->SetScrollBarAlwaysVisible(AlwaysShowScrollbar);
	}
}

void UScrollBox2D::SetAllowOverscroll(bool newAllowOverscroll)
{
	AllowOverscroll = newAllowOverscroll;

	if (MyScrollBox.IsValid())
	{
		MyScrollBox->SetAllowOverscroll(AllowOverscroll ? EAllowOverscroll::Yes : EAllowOverscroll::No);
	}
}

void UScrollBox2D::SetAnimateWheelScrolling(bool shouldAnimateWheelScrolling)
{
	bAnimateWheelScrolling = shouldAnimateWheelScrolling;
	if (MyScrollBox)
	{
		MyScrollBox->SetAnimateWheelScrolling(shouldAnimateWheelScrolling);
	}
}

void UScrollBox2D::SetWheelScrollMultiplier(const FVector2D& newWheelScrollMultiplier)
{
	WheelScrollMultiplier = newWheelScrollMultiplier;
	if (MyScrollBox)
	{
		MyScrollBox->SetWheelScrollMultiplier(newWheelScrollMultiplier);
	}
}

void UScrollBox2D::SetScrollWhenFocusChanges(EScrollWhenFocusChanges newScrollWhenFocusChanges)
{
	ScrollWhenFocusChanges = newScrollWhenFocusChanges;
	if (MyScrollBox)
	{
		MyScrollBox->SetScrollWhenFocusChanges(newScrollWhenFocusChanges);
	}
}

void UScrollBox2D::EndInertialScrolling()
{
	if (MyScrollBox.IsValid())
	{
		MyScrollBox->EndInertialScrolling();
	}
}

void UScrollBox2D::SetAlwaysShowScrollbarTrack(bool newAlwaysShowScrollbarTrack)
{
	AlwaysShowScrollbarTrack = newAlwaysShowScrollbarTrack;
	if (MyScrollBox)
	{
		MyScrollBox->SetScrollBarTrackAlwaysVisible(AlwaysShowScrollbarTrack);
	}
}

void UScrollBox2D::SetAllowsRightClickDragScrolling(bool shouldAllowRightClickDragScrolling)
{
	AlwaysShowScrollbarTrack = shouldAllowRightClickDragScrolling;
	if (MyScrollBox)
	{
		MyScrollBox->SetDragScrollingEnabled(AlwaysShowScrollbarTrack);
	}
}

void UScrollBox2D::InitBackPadScrolling(bool inBackPadScrolling)
{
	ensureMsgf(!MyScrollBox.IsValid(), TEXT("The widget is already created."));
	BackPadScrolling = inBackPadScrolling;
}

void UScrollBox2D::InitFrontPadScrolling(bool inFrontPadScrolling)
{
	ensureMsgf(!MyScrollBox.IsValid(), TEXT("The widget is already created."));
	FrontPadScrolling = inFrontPadScrolling;
}

void UScrollBox2D::InitNavigationScrollPadding(float inNavigationScrollPadding)
{
	ensureMsgf(!MyScrollBox.IsValid(), TEXT("The widget is already created."));
	NavigationScrollPadding = inNavigationScrollPadding;
}

void UScrollBox2D::SlateHandleUserScrolled(const FVector2D& currentOffset)
{
	OnUserScrolled.Broadcast(currentOffset);
}

#if WITH_EDITOR

const FText UScrollBox2D::GetPaletteCategory()
{
	return INVTEXT("Panel");
}

void UScrollBox2D::OnDescendantSelectedByDesigner( UWidget* descendantWidget )
{
	UWidget* selectedChild = UWidget::FindChildContainingDescendant( this, descendantWidget );
	if (selectedChild)
	{
		ScrollWidgetIntoView( selectedChild, true );

		if (TickHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker( TickHandle );
			TickHandle.Reset();
		}
	}
}

void UScrollBox2D::OnDescendantDeselectedByDesigner( UWidget* descendantWidget )
{
	if (TickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker( TickHandle );
		TickHandle.Reset();
	}

	// because we get a de-select before we get a select, we need to delay this call until we're sure we didn't scroll to another widget.
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float) -> bool
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_UScrollBox2D_ScrollToStart_LambdaTick);
		this->ScrollToStart();
		return false;
	}));
}

#endif
