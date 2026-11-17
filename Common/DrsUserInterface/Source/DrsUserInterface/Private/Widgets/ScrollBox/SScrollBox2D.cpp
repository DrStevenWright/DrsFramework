// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#include "Widgets/ScrollBox/SScrollBox2D.h"

#include "Framework/Application/SlateApplication.h"
#include "Layout/LayoutUtils.h"
#include "Rendering/DrawElements.h"
#include "Types/SlateConstants.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SOverlay.h"

// Constants
namespace
{
	constexpr float ShadowFadeDistance = 32.0f;
}


template<typename SlotType>
static void ArrangeChildrenInStack2D(EOrientation arrangementDirection, EFlowDirection inLayoutFlow, const TPanelChildren<SlotType>& children, const FGeometry& allottedGeometry, FArrangedChildren& arrangedChildren, const FVector2D& inOffset, bool inAllowShrink)
{
	// Allotted space will be given to fixed-size children first.
	// Remaining space will be proportionately divided between stretch children (SizeRule_Stretch)
	// based on their stretch coefficient

	if (children.Num() > 0)
	{
		FVector2f fixedTotal = FVector2f::ZeroVector;
		FVector2f stretchSizeTotal = FVector2f::ZeroVector;
		float stretchCoefficientTotal = 0.0f;
		bool anyChildVisible = false;
		
		// Compute the sum of stretch coefficients (SizeRule_Stretch) and space required by fixed-size widgets (SizeRule_Auto),
		// as well as the total desired size.
		for (int32 childIndex = 0; childIndex < children.Num(); ++childIndex)
		{
			const SlotType& curChild = children[childIndex];
			if (curChild.GetWidget()->GetVisibility() != EVisibility::Collapsed)
			{
				// All widgets contribute their margin to the fixed space requirement
				fixedTotal += FVector2f(curChild.GetPadding().template GetTotalSpaceAlong<Orient_Horizontal>(),
										curChild.GetPadding().template GetTotalSpaceAlong<Orient_Vertical>());
				anyChildVisible = true;

				// Auto-sized children contribute their desired size to the fixed space requirement
				const FVector2f childSize = curChild.GetWidget()->GetDesiredSize();

				if (curChild.GetSizeRule() == FSizeParam::SizeRule_Stretch)
				{
					// For stretch children we save sum up the stretch coefficients
					stretchCoefficientTotal += curChild.GetSizeValue();
					stretchSizeTotal += childSize;
				}
				else
				{
					// Clamp to the max size if it was specified
					fixedTotal += (curChild.GetMaxSize() > 0.0f) ? FVector2f::Min(curChild.GetMaxSize2D(), childSize) : childSize;
				}
			}
		}

		if (!anyChildVisible)
		{
			return;
		}

		// When shrink is not allowed, we'll ensure to use all the space desired by the stretchable widgets.
		const FVector2f minSize = inAllowShrink ? FVector2f::ZeroVector : stretchSizeTotal;

		// The space available for SizeRule_Stretch widgets is any space that wasn't taken up by fixed-sized widgets.
		const FVector2f nonFixedSpace = FVector2f::Max(allottedGeometry.GetLocalSize() - fixedTotal, minSize);
		
		// Now that we have the total fixed-space requirement and the total stretch coefficients we can
		// arrange widgets top-to-bottom or left-to-right (depending on the orientation).
		FVector2f positionSoFar = FVector2f::ZeroVector;
		for (TPanelChildrenConstIterator<SlotType> it(children, inLayoutFlow); it; ++it)
		{
			const SlotType& curChild = *it;
			const EVisibility childVisibility = curChild.GetWidget()->GetVisibility();

			// Figure out the area allocated to the child in the direction of BoxPanel
			// The area allocated to the slot is ChildSize + the associated margin.
			FVector2f childSize = FVector2f::ZeroVector;
			if (childVisibility != EVisibility::Collapsed)
			{
				// The size of the widget depends on its size type
				if (curChild.GetSizeRule() == FSizeParam::SizeRule_Stretch)
				{
					// Stretch widgets get a fraction of the space remaining after all the fixed-space requirements are met
					if (stretchCoefficientTotal > 0.f)
					{
						childSize = nonFixedSpace * curChild.GetSizeValue() / stretchCoefficientTotal;
					}
				}
				else
				{
					// Auto-sized widgets get their desired-size value
					childSize = curChild.GetWidget()->GetDesiredSize();
				}

				// Clamp to the max size if it was specified
				if (curChild.GetMaxSize() > 0.0f)
				{
					childSize = FVector2f::Min(curChild.GetMaxSize2D(), childSize);
				}
			}

			const FMargin slotPadding(LayoutPaddingWithFlow(inLayoutFlow, curChild.GetPadding()));
			const FVector2f slotSize(childSize.X + slotPadding.GetTotalSpaceAlong<Orient_Horizontal>(),
									 childSize.Y + slotPadding.GetTotalSpaceAlong<Orient_Vertical>());
									 
			// Figure out the size and local position of the child within the slot
			const AlignmentArrangeResult alignmentResultX = AlignChild<Orient_Horizontal>(inLayoutFlow, slotSize.X, curChild, slotPadding);
			const AlignmentArrangeResult alignmentResultY = AlignChild<Orient_Vertical>(slotSize.Y, curChild, slotPadding);
			
			const FVector2f localPosition = FVector2f(positionSoFar.X + alignmentResultX.Offset + inOffset.X, positionSoFar.Y + alignmentResultY.Offset + inOffset.Y);
			const FVector2f localSize = FVector2f(alignmentResultX.Size, alignmentResultY.Size);

			// Add the information about this child to the output list (ArrangedChildren)
			arrangedChildren.AddWidget(childVisibility, allottedGeometry.MakeChild(
				// The child widget being arranged
				curChild.GetWidget(),
				// Child's local position (i.e. position within parent)
				localPosition,
				// Child's size
				localSize
			));

			if (childVisibility != EVisibility::Collapsed)
			{
				// Offset the next child by the size of the current child and any post-child (bottom/right) margin
				if (arrangementDirection == Orient_Horizontal)
				{
					positionSoFar.X += slotSize.X;
				}
				else
				{
					positionSoFar.Y += slotSize.Y;
				}
			}
		}
	}
}


//-----------------------------------------------------
// SScrollBox2D::FSlot
//-----------------------------------------------------
SScrollBox2D::FSlot::FSlot()
	: TBasicLayoutWidgetSlot<SScrollBox2D::FSlot>(HAlign_Fill, VAlign_Fill)
	, SizeRule(FSizeParam::SizeRule_Auto)
	, SizeValue(*this, 1.0f)
	, MaxSize(*this, 0.0f)
{
}

void SScrollBox2D::FSlot::Construct(const FChildren& slotOwner, FSlotArguments&& inArgs)
{
	TBasicLayoutWidgetSlot<FSlot>::Construct(slotOwner, MoveTemp(inArgs));
	if (inArgs._MaxSize.IsSet())
	{
		SetMaxSize(MoveTemp(inArgs._MaxSize));
	}
	if (inArgs._SizeParam.IsSet())
	{
		SetSizeParam(MoveTemp(inArgs._SizeParam.GetValue()));
	}
}

void SScrollBox2D::FSlot::RegisterAttributes(FSlateWidgetSlotAttributeInitializer& attributeInitializer)
{
	TBasicLayoutWidgetSlot<FSlot>::RegisterAttributes(attributeInitializer);
	SLATE_ADD_SLOT_ATTRIBUTE_DEFINITION_WITH_NAME(FSlot, attributeInitializer, "Slot.MaxSize", MaxSize, EInvalidateWidgetReason::Layout);
	SLATE_ADD_SLOT_ATTRIBUTE_DEFINITION_WITH_NAME(FSlot, attributeInitializer, "Slot.SizeValue", SizeValue, EInvalidateWidgetReason::Layout)
		.UpdatePrerequisite("Slot.MaxSize");
}

SScrollBox2D::FSlot::FSlotArguments SScrollBox2D::Slot()
{
	return FSlot::FSlotArguments(MakeUnique<FSlot>());
}


//-----------------------------------------------------
// SScrollPanel2D
//-----------------------------------------------------
void SScrollPanel2D::Construct(const FArguments& inArgs, TArray<SScrollBox2D::FSlot::FSlotArguments> inSlots)
{
	PhysicalOffset = FVector2D::ZeroVector;
	Children.AddSlots(MoveTemp(inSlots));
	BackPadScrolling = inArgs._BackPadScrolling;
	FrontPadScrolling = inArgs._FrontPadScrolling;
	ArrangementDirection = inArgs._ArrangementDirection;
}

void SScrollPanel2D::OnArrangeChildren(const FGeometry& allottedGeometry, FArrangedChildren& arrangedChildren) const
{
	const FVector2D scrollPadding = allottedGeometry.GetLocalSize();
	const FVector2D childrenOffset =  (BackPadScrolling ? scrollPadding : FVector2D::ZeroVector) - PhysicalOffset;
	constexpr bool allowShrink = false;
	
	ArrangeChildrenInStack2D(ArrangementDirection, GSlateFlowDirection, this->Children, allottedGeometry, arrangedChildren, childrenOffset, allowShrink);
}

FVector2D SScrollPanel2D::ComputeDesiredSize(float) const
{
	FVector2D thisDesiredSize = FVector2D::ZeroVector;
	for (int32 slotIndex = 0; slotIndex < Children.Num(); ++slotIndex)
	{
		const SScrollBox2D::FSlot& thisSlot = Children[slotIndex];
		if (thisSlot.GetWidget()->GetVisibility() != EVisibility::Collapsed)
		{
			const FVector2D childDesiredSize = thisSlot.GetWidget()->GetDesiredSize();
			if (ArrangementDirection == Orient_Vertical)
			{
				thisDesiredSize.X = FMath::Max(childDesiredSize.X + thisSlot.GetPadding().GetTotalSpaceAlong<Orient_Horizontal>(), thisDesiredSize.X);
				thisDesiredSize.Y += childDesiredSize.Y + thisSlot.GetPadding().GetTotalSpaceAlong<Orient_Vertical>();
			}
			else
			{
				thisDesiredSize.X += childDesiredSize.X + thisSlot.GetPadding().GetTotalSpaceAlong<Orient_Horizontal>();
				thisDesiredSize.Y = FMath::Max(childDesiredSize.Y + thisSlot.GetPadding().GetTotalSpaceAlong<Orient_Vertical>(), thisDesiredSize.Y);
			}
		}
	}
	
	const FVector2D scrollPadding = GetTickSpaceGeometry().GetLocalSize();
	if (BackPadScrolling)
	{
		thisDesiredSize += scrollPadding;
	}
	if (FrontPadScrolling)
	{
		thisDesiredSize += scrollPadding;
	}
	
	return thisDesiredSize;
}


//-----------------------------------------------------
// SScrollBox2D
//-----------------------------------------------------
SScrollBox2D::SScrollBox2D()
{
	bClippingProxy = true;
}

void SScrollBox2D::Construct(const FArguments& inArgs)
{
	check(inArgs._Style);

	Style = inArgs._Style;
	ScrollBarStyle = inArgs._ScrollBarStyle;
	OnUserScrolled = inArgs._OnUserScrolled;
	ConsumeMouseWheel = inArgs._ConsumeMouseWheel;
	AllowOverscroll = inArgs._AllowOverscroll;
	bBackPadScrolling = inArgs._BackPadScrolling;
	bFrontPadScrolling = inArgs._FrontPadScrolling;
	bAnimateWheelScrolling = inArgs._AnimateWheelScrolling;
	WheelScrollMultiplier = inArgs._WheelScrollMultiplier;
	ArrangementDirection = inArgs._ArrangementDirection;
	NavigationScrollPadding = inArgs._NavigationScrollPadding;
	NavigationDestination = inArgs._NavigationDestination;
	ScrollWhenFocusChanges = inArgs._ScrollWhenFocusChanges;
	ScrollBarSlotPadding = inArgs._ScrollBarPadding;
	bVolatilityAlwaysInvalidatesPrepass = true;
	
	// Make a scroll bars
	HorizontalScrollBar = ConstructScrollBar(EOrientation::Orient_Horizontal);
	HorizontalScrollBar->SetDragFocusCause(inArgs._ScrollBarDragFocusCause);
	HorizontalScrollBar->SetThickness(inArgs._ScrollBarThickness);
	HorizontalScrollBar->SetUserVisibility(inArgs._ScrollBarVisibility);
	HorizontalScrollBar->SetScrollBarAlwaysVisible(inArgs._ScrollBarAlwaysVisible);
	HorizontalScrollBar->SetOnUserScrolled(FOnUserScrolled::CreateSP(this, &SScrollBox2D::ScrollBar_OnUserScrolledHorizontally));
	
	VerticalScrollBar = ConstructScrollBar(EOrientation::Orient_Vertical);
	VerticalScrollBar->SetDragFocusCause(inArgs._ScrollBarDragFocusCause);
	VerticalScrollBar->SetThickness(inArgs._ScrollBarThickness);
	VerticalScrollBar->SetUserVisibility(inArgs._ScrollBarVisibility);
	VerticalScrollBar->SetScrollBarAlwaysVisible(inArgs._ScrollBarAlwaysVisible);
	VerticalScrollBar->SetOnUserScrolled(FOnUserScrolled::CreateSP(this, &SScrollBox2D::ScrollBar_OnUserScrolledVertically));
	
	SAssignNew(ScrollPanel, SScrollPanel2D, MoveTemp(const_cast<TArray<FSlot::FSlotArguments>&>(inArgs._Slots)))
		.Clipping(inArgs._Clipping)
		.BackPadScrolling(bBackPadScrolling)
		.FrontPadScrolling(bFrontPadScrolling)
		.ArrangementDirection(ArrangementDirection);

	ConstructLayout();

	HorizontalScrollBar->SetState( 0.0f, 1.0f );
	VerticalScrollBar->SetState( 0.0f, 1.0f );
}

void SScrollBox2D::OnClippingChanged()
{
	ScrollPanel->SetClipping(Clipping);
}

TSharedPtr<SScrollBar> SScrollBox2D::ConstructScrollBar(EOrientation orientation)
{
	return TSharedPtr<SScrollBar>(SNew(SScrollBar)
		.Style(ScrollBarStyle)
		.Orientation(orientation) 
		.Padding(0.0f));
}

void SScrollBox2D::ConstructLayout()
{
	TSharedPtr<SHorizontalBox> innerHorizontalBoxAndVerticalScrollbar;
	TSharedPtr<SVerticalBox> outerVerticalBoxAndHorizontalScrollbar;
	
	this->ChildSlot
	[
		SAssignNew(outerVerticalBoxAndHorizontalScrollbar, SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			.Padding(Style->HorizontalScrolledContentPadding)
			[
				SAssignNew(innerHorizontalBoxAndVerticalScrollbar, SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					.Padding(Style->VerticalScrolledContentPadding)
					[
						// Scroll panel that presents the scrolled content
						ScrollPanel.ToSharedRef()
					]

					+ SOverlay::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Top)
					[
						// Shadow: Hint to scroll up
						SNew(SImage)
						.Visibility(EVisibility::HitTestInvisible)
						.ColorAndOpacity(this, &SScrollBox2D::GetStartShadowOpacity_Vertical)
						.Image(&Style->TopShadowBrush)
					]

					+ SOverlay::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Bottom)
					[
						// Shadow: Hint to scroll down
						SNew(SImage)
						.Visibility(EVisibility::HitTestInvisible)
						.ColorAndOpacity(this, &SScrollBox2D::GetEndShadowOpacity_Vertical)
						.Image(&Style->BottomShadowBrush)
					]
				]
			]

			+ SOverlay::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Fill)
			[
				// Shadow: Hint to scroll left
				SNew(SImage)
				.Visibility(EVisibility::HitTestInvisible)
				.ColorAndOpacity(this, &SScrollBox2D::GetStartShadowOpacity_Horizontal)
				.Image(&Style->LeftShadowBrush)
			]

			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Fill)
			[
				// Shadow: Hint to scroll right
				SNew(SImage)
				.Visibility(EVisibility::HitTestInvisible)
				.ColorAndOpacity(this, &SScrollBox2D::GetEndShadowOpacity_Horizontal)
				.Image(&Style->RightShadowBrush)
			]
		]
	];

	VerticalScrollBarSlot = nullptr;
	HorizontalScrollBarSlot = nullptr;

	innerHorizontalBoxAndVerticalScrollbar->AddSlot()
	.Padding(ScrollBarSlotPadding)
	.AutoWidth()
	.Expose(VerticalScrollBarSlot)
	[
		VerticalScrollBar.ToSharedRef()
	];
	
	outerVerticalBoxAndHorizontalScrollbar->AddSlot()
	.Padding(ScrollBarSlotPadding)
	.AutoHeight()
	.Expose(HorizontalScrollBarSlot)
	[
		HorizontalScrollBar.ToSharedRef()
	];
}

SScrollBox2D::FScopedWidgetSlotArguments SScrollBox2D::AddSlot()
{
	return FScopedWidgetSlotArguments{MakeUnique<FSlot>(), ScrollPanel->Children, INDEX_NONE};
}

void SScrollBox2D::RemoveSlot(const TSharedRef<SWidget>& widgetToRemove)
{
	ScrollPanel->Children.Remove(widgetToRemove);
}

void SScrollBox2D::ClearChildren()
{
	ScrollPanel->Children.Empty();
}

bool SScrollBox2D::IsRightClickDragScrolling() const
{
	if (FSlateApplication::IsInitialized())
	{
		const float dragTriggerDistance = FSlateApplication::Get().GetDragTriggerDistance();
		if (AmountScrolledWhileRightMouseDown.X >= dragTriggerDistance && HorizontalScrollBar->IsNeeded())
		{
			return true;
		}
		else if (AmountScrolledWhileRightMouseDown.Y > dragTriggerDistance && VerticalScrollBar->IsNeeded())
		{
			return true;
		}
	}
	
	return false;
}

void SScrollBox2D::SetScrollOffset(const FVector2D& newScrollOffset)
{
	DesiredScrollOffset = newScrollOffset;
	bScrollToEnd = false;
	Invalidate(EInvalidateWidget::Layout);
}

FVector2D SScrollBox2D::GetScrollOffsetOfEnd() const
{
	const FGeometry scrollPanelGeometry = FindChildGeometry(CachedGeometry, ScrollPanel.ToSharedRef());
	const FVector2D contentSize = ScrollPanel->GetDesiredSize();
	const FVector2D offset = contentSize - scrollPanelGeometry.GetLocalSize();
	return FVector2D::Max(offset, FVector2D::ZeroVector);
}

FVector2D SScrollBox2D::GetViewFraction() const
{
	const FGeometry scrollPanelGeometry = FindChildGeometry(CachedGeometry, ScrollPanel.ToSharedRef());
	const FVector2D contentSize = ScrollPanel->GetDesiredSize();
	const FVector2D localSize = CachedGeometry.GetLocalSize();
	const FVector2D scaledPanelSize = scrollPanelGeometry.Size / contentSize;

	// If this is a safety check then it seems strange to be checking localSize here, rather then the value we are actually using in the divide.
	FVector2D viewFraction;
	viewFraction.X = (localSize.X > 0) ? FMath::Clamp(scaledPanelSize.X, 0, 1) : 1;
	viewFraction.Y = (localSize.Y > 0) ? FMath::Clamp(scaledPanelSize.Y, 0, 1) : 1;
	
	return viewFraction;
}

FVector2D SScrollBox2D::GetViewOffsetFraction() const
{
	const FVector2D viewFraction = GetViewFraction();
	const FVector2D offsetFraction = DesiredScrollOffset / ScrollPanel->GetDesiredSize();

	return FVector2D(FMath::Clamp(offsetFraction.X, 0.0f, 1.0f - viewFraction.X),
					FMath::Clamp(offsetFraction.Y, 0.0f, 1.0f - viewFraction.Y));
}

void SScrollBox2D::ScrollToStart()
{
	SetScrollOffset(FVector2D::ZeroVector);
}

void SScrollBox2D::ScrollToEnd()
{
	bScrollToEnd = true;
	Invalidate(EInvalidateWidget::Layout);
}

void SScrollBox2D::ScrollDescendantIntoView(const TSharedPtr<SWidget>& widgetToScrollIntoView, bool inAnimateScroll, EDescendantScrollDestination inDestination, float inScrollPadding)
{
	ScrollIntoViewRequest = [this, widgetToScrollIntoView, inAnimateScroll, inDestination, inScrollPadding] (const FGeometry& allottedGeometry)
	{
		InternalScrollDescendantIntoView(allottedGeometry, widgetToScrollIntoView, inAnimateScroll, inDestination, inScrollPadding);
	};

	BeginInertialScrolling();
}

bool SScrollBox2D::InternalScrollDescendantIntoView(const FGeometry& myGeometry, const TSharedPtr<SWidget>& widgetToFind, bool inAnimateScroll, EDescendantScrollDestination inDestination, float inScrollPadding)
{
	// We need to safely find the one widgetToFind among our descendants.
	TSet<TSharedRef<SWidget>> widgetsToFind;
	{
		if (widgetToFind.IsValid())
		{
			widgetsToFind.Add(widgetToFind.ToSharedRef());
		}
	}
	TMap<TSharedRef<SWidget>, FArrangedWidget> result;

	FindChildGeometries( myGeometry, widgetsToFind, result );

	if (widgetToFind.IsValid())
	{
		FArrangedWidget* widgetGeometry = result.Find(widgetToFind.ToSharedRef());
		if (!widgetGeometry)
		{
			UE_LOG(LogSlate, Warning, TEXT("Unable to scroll to descendant as it's not a child of the scrollbox"));
		}
		else
		{
			FVector2D scrollOffset = FVector2D::ZeroVector;
			const FVector2D padding(inScrollPadding);
			
			if (inDestination == EDescendantScrollDestination::TopOrLeft)
			{
				// Calculate how much we would need to scroll to bring this to the top/left of the scroll box
				const FVector2D widgetPosition = myGeometry.AbsoluteToLocal(widgetGeometry->Geometry.GetAbsolutePosition());
				const FVector2D myPosition = padding;
				scrollOffset = widgetPosition - myPosition;
			}
			else if (inDestination == EDescendantScrollDestination::BottomOrRight)
			{
				// Calculate how much we would need to scroll to bring this to the bottom/right of the scroll box
				const FVector2D widgetPosition = myGeometry.AbsoluteToLocal(widgetGeometry->Geometry.GetAbsolutePosition() + widgetGeometry->Geometry.GetAbsoluteSize()) - myGeometry.GetLocalSize();
				const FVector2D myPosition = padding;
				scrollOffset = widgetPosition - myPosition;
			}
			else if (inDestination == EDescendantScrollDestination::Center)
			{
				// Calculate how much we would need to scroll to bring this to the top/left of the scroll box
				const FVector2D widgetPosition = myGeometry.AbsoluteToLocal(widgetGeometry->Geometry.GetAbsolutePosition()) + (widgetGeometry->Geometry.GetLocalSize() / 2);
				const FVector2D myPosition = myGeometry.GetLocalSize() / 2;
				scrollOffset = widgetPosition - myPosition;
			}
			else
			{
				const FVector2D widgetStartPosition = myGeometry.AbsoluteToLocal(widgetGeometry->Geometry.GetAbsolutePosition());
				const FVector2D widgetEndPosition = widgetStartPosition + widgetGeometry->Geometry.GetLocalSize();
				const FVector2D viewStartPosition = padding;
				const FVector2D viewEndPosition = myGeometry.GetLocalSize() - padding;
				const FVector2D deltaFromStart = widgetStartPosition - viewStartPosition;
				const FVector2D deltaFromEnd = widgetEndPosition - viewEndPosition;
				
				scrollOffset.X = deltaFromStart.X < 0 ? deltaFromStart.X : (deltaFromEnd.X > 0 ? deltaFromEnd.X : scrollOffset.X);
				scrollOffset.Y = deltaFromStart.Y < 0 ? deltaFromStart.Y : (deltaFromEnd.Y > 0 ? deltaFromEnd.Y : scrollOffset.Y);
			}

			if (!scrollOffset.IsZero())
			{
				DesiredScrollOffset = ScrollPanel->PhysicalOffset;
				ScrollBy(myGeometry, scrollOffset, EAllowOverscroll::No, inAnimateScroll);
			}

			return true;
		}
	}

	return false;
}

void SScrollBox2D::SetStyle(const FScrollBoxStyle* inStyle)
{
	if (Style != inStyle)
	{
		Style = inStyle;
		InvalidateStyle();
	}
}

void SScrollBox2D::SetScrollBarStyle(const FScrollBarStyle* inBarStyle)
{
	if (inBarStyle != ScrollBarStyle)
	{
		ScrollBarStyle = inBarStyle;
		
		if (VerticalScrollBar.IsValid())
		{
			VerticalScrollBar->SetStyle(ScrollBarStyle);
		}
		
		if (HorizontalScrollBar.IsValid())
		{
			HorizontalScrollBar->SetStyle(ScrollBarStyle);
		}
	}
}

void SScrollBox2D::InvalidateStyle()
{
	Invalidate(EInvalidateWidgetReason::Layout);
}

void SScrollBox2D::InvalidateScrollBarStyle()
{
	if (HorizontalScrollBar.IsValid())
	{
		HorizontalScrollBar->InvalidateStyle();
	}
	
	if (VerticalScrollBar.IsValid())
	{
		VerticalScrollBar->InvalidateStyle();
	}
}

void SScrollBox2D::SetScrollBarVisibility(EVisibility inVisibility)
{
	HorizontalScrollBar->SetUserVisibility(inVisibility);
	VerticalScrollBar->SetUserVisibility(inVisibility);
}

void SScrollBox2D::SetScrollBarAlwaysVisible(bool inAlwaysVisible)
{
	HorizontalScrollBar->SetScrollBarAlwaysVisible(inAlwaysVisible);
	VerticalScrollBar->SetScrollBarAlwaysVisible(inAlwaysVisible);
}

void SScrollBox2D::SetScrollBarTrackAlwaysVisible(bool inAlwaysVisible)
{
	HorizontalScrollBar->SetScrollBarTrackAlwaysVisible(inAlwaysVisible);
	VerticalScrollBar->SetScrollBarTrackAlwaysVisible(inAlwaysVisible);
}

void SScrollBox2D::SetScrollBarThickness(UE::Slate::FDeprecateVector2DParameter inThickness)
{
	HorizontalScrollBar->SetThickness(inThickness);
	VerticalScrollBar->SetThickness(inThickness);
}

void SScrollBox2D::SetScrollBarPadding(const FMargin& inPadding)
{
	ScrollBarSlotPadding = inPadding;
	
	if (VerticalScrollBarSlot)
	{
		VerticalScrollBarSlot->SetPadding(ScrollBarSlotPadding);
	}

	if (HorizontalScrollBarSlot)
	{
		HorizontalScrollBarSlot->SetPadding(ScrollBarSlotPadding);
	}
}

void SScrollBox2D::Tick(const FGeometry& allottedGeometry, const double inCurrentTime, const float inDeltaTime)
{
	CachedGeometry = allottedGeometry;

	if (bTouchPanningCapture && (FSlateApplication::Get().GetCurrentTime() - LastScrollTime) > 0.10)
	{
		InertialScrollManagerHorizontal.ClearScrollVelocity();
		InertialScrollManagerVertical.ClearScrollVelocity();
	}
	
	// If we needed a widget to be scrolled into view, make that happen.
	if (ScrollIntoViewRequest)
	{
		ScrollIntoViewRequest(allottedGeometry);
		ScrollIntoViewRequest = nullptr;
	}

	const FGeometry scrollPanelGeometry = FindChildGeometry(allottedGeometry, ScrollPanel.ToSharedRef());

	if (bScrollToEnd)
	{
		const FVector2D offset = ScrollPanel->GetDesiredSize() - scrollPanelGeometry.GetLocalSize();
		DesiredScrollOffset =  FVector2D::Max(offset, FVector2D::ZeroVector);
		bScrollToEnd = false;
	}
	
	// If this scroll box has no size, do not compute a view fraction because it will be wrong and causes pop in when the size is available
	const FVector2D viewFraction = GetViewFraction();
	const FVector2D targetViewOffset = GetViewOffsetFraction();
	const FVector2D scrollbarState(HorizontalScrollBar->DistanceFromTop(), VerticalScrollBar->DistanceFromTop());
	const FVector2D currentViewOffset = bAnimateScroll ? FMath::Vector2DInterpTo(scrollbarState, targetViewOffset, inDeltaTime, 15.f) : targetViewOffset;
	
	// Update the scrollbar with the clamped version of the offset
	FVector2D newPhysicalOffset = currentViewOffset * ScrollPanel->GetDesiredSize();
	if (AllowOverscroll == EAllowOverscroll::Yes)
	{
		newPhysicalOffset.X += OverscrollHorizontal.GetOverscroll(allottedGeometry);
		newPhysicalOffset.Y += OverscrollVertical.GetOverscroll(allottedGeometry);
	}

	const bool wasScrolling = bIsScrolling;
	bIsScrolling = !newPhysicalOffset.Equals(ScrollPanel->PhysicalOffset, 0.001f);

	ScrollPanel->PhysicalOffset = newPhysicalOffset;
	
	if (wasScrolling && !bIsScrolling)
	{
		Invalidate(EInvalidateWidget::Layout);
	}
	
	HorizontalScrollBar->SetState(currentViewOffset.X, viewFraction.X);
	VerticalScrollBar->SetState(currentViewOffset.Y, viewFraction.Y);
	
	// We cannot scroll, so ensure that there is no offset.
	if (!HorizontalScrollBar->IsNeeded())
	{
		ScrollPanel->PhysicalOffset.X = 0.0f;
	}
	
	if (!VerticalScrollBar->IsNeeded())
	{
		ScrollPanel->PhysicalOffset.Y = 0.0f;
	}
}

bool SScrollBox2D::ComputeVolatility() const
{
	return bIsScrolling || IsRightClickDragScrolling();
}

FReply SScrollBox2D::OnPreviewMouseButtonDown(const FGeometry& myGeometry, const FPointerEvent& mouseEvent)
{
	if (mouseEvent.IsTouchEvent() && !bFingerOwningTouchInteraction.IsSet())
	{
		// Clear any inertia 
		InertialScrollManagerHorizontal.ClearScrollVelocity();
		InertialScrollManagerVertical.ClearScrollVelocity();
		
		// We have started a new interaction; track how far the user has moved since they put their finger down.
		AmountScrolledWhileRightMouseDown = FVector2D::ZeroVector;
		PendingScrollTriggerAmount = FVector2D::ZeroVector;
		
		// Someone put their finger down in this list, so they probably want to drag the list.
		bFingerOwningTouchInteraction = mouseEvent.GetPointerIndex();

		Invalidate(EInvalidateWidget::Layout);
	}
	return FReply::Unhandled();
}

FReply SScrollBox2D::OnMouseButtonDown(const FGeometry& myGeometry, const FPointerEvent& mouseEvent)
{
	if (!bFingerOwningTouchInteraction.IsSet())
	{
		EndInertialScrolling();
	}
	
	if (mouseEvent.IsTouchEvent())
	{
		return FReply::Handled();
	}
	else
	{
		if (mouseEvent.GetEffectingButton() == EKeys::RightMouseButton && bAllowsRightClickDragScrolling
			&& (HorizontalScrollBar->IsNeeded() || VerticalScrollBar->IsNeeded()))
		{
			AmountScrolledWhileRightMouseDown = FVector2D::ZeroVector;

			Invalidate(EInvalidateWidget::Layout);

			return FReply::Handled();
		}
	}

	return FReply::Unhandled();	
}

FReply SScrollBox2D::OnMouseButtonUp(const FGeometry& myGeometry, const FPointerEvent& mouseEvent)
{
	if (mouseEvent.GetEffectingButton() == EKeys::RightMouseButton && bAllowsRightClickDragScrolling)
	{
		if (!bIsScrollingActiveTimerRegistered && IsRightClickDragScrolling())
		{
			// Register the active timer to handle the inertial scrolling
			CachedGeometry = myGeometry;
			BeginInertialScrolling();
		}

		AmountScrolledWhileRightMouseDown = FVector2D::ZeroVector;

		Invalidate(EInvalidateWidget::Layout);

		FReply reply = FReply::Handled().ReleaseMouseCapture();
		bShowSoftwareCursor = false;

		// If we have mouse capture, snap the mouse back to the closest location that is within the panel's bounds
		if (HasMouseCapture())
		{
			FSlateRect panelScreenSpaceRect = myGeometry.GetLayoutBoundingRect();
			FVector2f cursorPosition = myGeometry.LocalToAbsolute( SoftwareCursorPosition );

			FIntPoint bestPositionInPanel(
				FMath::RoundToInt( FMath::Clamp( cursorPosition.X, panelScreenSpaceRect.Left, panelScreenSpaceRect.Right ) ),
				FMath::RoundToInt( FMath::Clamp( cursorPosition.Y, panelScreenSpaceRect.Top, panelScreenSpaceRect.Bottom ) )
			);

			reply.SetMousePos(bestPositionInPanel);
		}

		return reply;
	}

	return FReply::Unhandled();
}

FReply SScrollBox2D::OnMouseMove(const FGeometry& myGeometry, const FPointerEvent& mouseEvent)
{
	const FVector2D scrollByAmountScreen = mouseEvent.GetCursorDelta();
	const FVector2D scrollByAmountLocal = scrollByAmountScreen / myGeometry.Scale;

	if (mouseEvent.IsTouchEvent())
	{
		FReply reply = FReply::Unhandled();

		if (!bTouchPanningCapture)
		{
			if (bFingerOwningTouchInteraction.IsSet() && mouseEvent.IsTouchEvent() && !HasMouseCapture())
			{
				PendingScrollTriggerAmount += scrollByAmountScreen;

				const float dragTriggerDistance = FSlateApplication::Get().GetDragTriggerDistance();
				if (FMath::Abs(PendingScrollTriggerAmount.X) > dragTriggerDistance)
				{
					bTouchPanningCapture = true;
					HorizontalScrollBar->BeginScrolling();

					// The user has moved the list some amount; they are probably trying to scroll. From now on, the list
					// assumes the user is scrolling until they lift their finger.
					reply = FReply::Handled().CaptureMouse(AsShared());
				}
				else if (FMath::Abs(PendingScrollTriggerAmount.Y) > dragTriggerDistance)
				{
					bTouchPanningCapture = true;
					VerticalScrollBar->BeginScrolling();

					// The user has moved the list some amount; they are probably trying to scroll. From now on, the list
					// assumes the user is scrolling until they lift their finger.
					reply = FReply::Handled().CaptureMouse(AsShared());
				}
				else
				{
					reply = FReply::Handled();
				}
			}
		}
		else
		{
			if (bFingerOwningTouchInteraction.IsSet() && HasMouseCaptureByUser(mouseEvent.GetUserIndex(), mouseEvent.GetPointerIndex()))
			{
				LastScrollTime = FSlateApplication::Get().GetCurrentTime();
				
				InertialScrollManagerHorizontal.AddScrollSample(-scrollByAmountScreen.X, LastScrollTime);
				InertialScrollManagerVertical.AddScrollSample(-scrollByAmountScreen.Y, LastScrollTime);
				
				ScrollBy(myGeometry, -scrollByAmountLocal, EAllowOverscroll::Yes, false);

				reply = FReply::Handled();
			}
		}

		return reply;
	}
	else
	{
		if (mouseEvent.IsMouseButtonDown(EKeys::RightMouseButton) && bAllowsRightClickDragScrolling)
		{
			// If scrolling with the right mouse button, we need to remember how much we scrolled.
			// If we did not scroll at all, we will bring up the context menu when the mouse is released.
			AmountScrolledWhileRightMouseDown += scrollByAmountScreen.GetAbs();

			// Has the mouse moved far enough with the right mouse button held down to start capturing
			// the mouse and dragging the view?
			if (IsRightClickDragScrolling())
			{
				InertialScrollManagerHorizontal.AddScrollSample(-scrollByAmountScreen.X, FPlatformTime::Seconds());
				InertialScrollManagerVertical.AddScrollSample(-scrollByAmountScreen.Y, FPlatformTime::Seconds());
				
				const bool didScroll = ScrollBy(myGeometry, -scrollByAmountLocal, AllowOverscroll, false);
				FReply reply = FReply::Handled();

				// Capture the mouse if we need to
				if (HasMouseCapture() == false)
				{
					reply.CaptureMouse(AsShared()).UseHighPrecisionMouseMovement(AsShared());
					SoftwareCursorPosition = myGeometry.AbsoluteToLocal(mouseEvent.GetScreenSpacePosition());
					bShowSoftwareCursor = true;
				}

				// Check if the mouse has moved.
				if (didScroll)
				{
					const FVector2f scrollAmount(scrollByAmountLocal);
					SoftwareCursorPosition += scrollAmount;
				}

				return reply;
			}
		}
	}

	return FReply::Unhandled();
}

void SScrollBox2D::OnMouseEnter(const FGeometry& myGeometry, const FPointerEvent& mouseEvent)
{
	if (mouseEvent.IsTouchEvent())
	{
		if (!bFingerOwningTouchInteraction.IsSet())
		{
			// If we currently do not have touch capture, allow this widget to begin scrolling on pointer enter events
			// if it comes from a child widget
			if (myGeometry.IsUnderLocation(mouseEvent.GetLastScreenSpacePosition()))
			{
				bFingerOwningTouchInteraction = mouseEvent.GetPointerIndex();
			}
		}
	}
}

void SScrollBox2D::OnMouseLeave(const FPointerEvent& mouseEvent)
{
	if (HasMouseCapture() == false)
	{
		// No longer scrolling (unless we have mouse capture)
		if (!AmountScrolledWhileRightMouseDown.IsZero())
		{
			AmountScrolledWhileRightMouseDown = FVector2D::ZeroVector;;
			Invalidate(EInvalidateWidget::Layout);
		}

		if (mouseEvent.IsTouchEvent())
		{
			bFingerOwningTouchInteraction.Reset();
		}
	}
}

FReply SScrollBox2D::OnMouseWheel(const FGeometry& myGeometry, const FPointerEvent& mouseEvent)
{
	// Make sure scroll velocity is cleared so that it doesn't fight with the mouse wheel input
	InertialScrollManagerHorizontal.ClearScrollVelocity();
	InertialScrollManagerVertical.ClearScrollVelocity();
	
	if (ConsumeMouseWheel == EConsumeMouseWheel::Always
		|| (ConsumeMouseWheel == EConsumeMouseWheel::WhenScrollingPossible && (HorizontalScrollBar->IsNeeded() || VerticalScrollBar->IsNeeded())))
	{
		const FVector2D scrollAmount(0.0f, -mouseEvent.GetWheelDelta() * GetGlobalScrollAmount() * WheelScrollMultiplier.Y);
		const bool scrollWasHandled = ScrollBy(myGeometry, scrollAmount, EAllowOverscroll::No, bAnimateWheelScrolling);

		if (scrollWasHandled && !bIsScrollingActiveTimerRegistered)
		{
			// Register the active timer to handle the inertial scrolling
			CachedGeometry = myGeometry;
			BeginInertialScrolling();
		}

		return scrollWasHandled ? FReply::Handled() : FReply::Unhandled();
	}
	else
	{
		return FReply::Unhandled();
	}
}

void SScrollBox2D::ScrollVertical(float delta)
{
	Scroll2D(FVector2D(0.f, delta));
}

void SScrollBox2D::ScrollHorizontal(float delta)
{
	Scroll2D(FVector2D(delta, 0.f));
}

void SScrollBox2D::Scroll2D(const FVector2D& delta)
{
	ScrollBy(CachedGeometry, -delta * WheelScrollSpeed * WheelScrollMultiplier / CachedGeometry.Scale, EAllowOverscroll::No, bAnimateWheelScrolling);
}

void SScrollBox2D::DragScroll2D(const FVector2D& delta)
{
	ensure(bAllowsRightClickDragScrolling);
	ScrollBy(CachedGeometry, -delta / CachedGeometry.Scale, AllowOverscroll, false);
}

bool SScrollBox2D::ScrollBy(const FGeometry& allottedGeometry, const FVector2D& localScrollAmount, EAllowOverscroll overscrolling, bool inAnimateScroll)
{
	Invalidate(EInvalidateWidget::LayoutAndVolatility);

	bAnimateScroll = inAnimateScroll;
	
	const FVector2D contentSize = ScrollPanel->GetDesiredSize();
	const FGeometry scrollPanelGeometry = FindChildGeometry(allottedGeometry, ScrollPanel.ToSharedRef());
	const FVector2D previousScrollOffset = DesiredScrollOffset;

	if (!localScrollAmount.IsZero())
	{
		const FVector2D scrollMin = FVector2D::ZeroVector;
		const FVector2D scrollMax = FVector2D::Max(contentSize - scrollPanelGeometry.GetLocalSize(), FVector2D::ZeroVector);
		
		if (AllowOverscroll == EAllowOverscroll::Yes && overscrolling == EAllowOverscroll::Yes && OverscrollHorizontal.ShouldApplyOverscroll(DesiredScrollOffset.X == scrollMin.X, DesiredScrollOffset.X == scrollMax.X, localScrollAmount.X))
		{
			OverscrollHorizontal.ScrollBy(allottedGeometry, localScrollAmount.X);
		}
		else
		{
			DesiredScrollOffset.X = FMath::Clamp(DesiredScrollOffset.X + localScrollAmount.X, scrollMin.X, scrollMax.X);
		}

		if (AllowOverscroll == EAllowOverscroll::Yes && overscrolling == EAllowOverscroll::Yes && OverscrollVertical.ShouldApplyOverscroll(DesiredScrollOffset.Y == scrollMin.Y, DesiredScrollOffset.Y == scrollMax.Y, localScrollAmount.Y))
		{
			OverscrollVertical.ScrollBy(allottedGeometry, localScrollAmount.Y);
		}
		else
		{
			DesiredScrollOffset.Y = FMath::Clamp(DesiredScrollOffset.Y + localScrollAmount.Y, scrollMin.X, scrollMax.Y);
		}
	}

	OnUserScrolled.ExecuteIfBound(DesiredScrollOffset);

	return ConsumeMouseWheel == EConsumeMouseWheel::Always || DesiredScrollOffset != previousScrollOffset;
}

FCursorReply SScrollBox2D::OnCursorQuery(const FGeometry& myGeometry, const FPointerEvent& cursorEvent) const
{
	if (IsRightClickDragScrolling())
	{
		// We hide the native cursor as we'll be drawing the software EMouseCursor::GrabHandClosed cursor
		return FCursorReply::Cursor( EMouseCursor::None );
	}
	else
	{
		return FCursorReply::Unhandled();
	}
}

FReply SScrollBox2D::OnTouchEnded(const FGeometry& myGeometry, const FPointerEvent& inTouchEvent)
{
	CachedGeometry = myGeometry;

	if (HasMouseCaptureByUser(inTouchEvent.GetUserIndex(), inTouchEvent.GetPointerIndex()))
	{
		HorizontalScrollBar->EndScrolling();
		VerticalScrollBar->EndScrolling();
		
		Invalidate(EInvalidateWidget::Layout);

		BeginInertialScrolling();

		return FReply::Handled().ReleaseMouseCapture();
	}

	return FReply::Unhandled();
}

void SScrollBox2D::OnMouseCaptureLost(const FCaptureLostEvent& captureLostEvent)
{
	SCompoundWidget::OnMouseCaptureLost(captureLostEvent);
	AmountScrolledWhileRightMouseDown = FVector2D::ZeroVector;
	PendingScrollTriggerAmount = FVector2D::ZeroVector;
	bFingerOwningTouchInteraction.Reset();
	bTouchPanningCapture = false;
}

FNavigationReply SScrollBox2D::OnNavigation(const FGeometry& myGeometry, const FNavigationEvent& inNavigationEvent)
{
	TSharedPtr<SWidget> focusedChild;
	int32 focusedChildIndex = -1;
	int32 focusedChildDirection = 0;

	// Find the child with focus currently so that we can find the next logical child we're going to move to.
	TPanelChildren<SScrollBox2D::FSlot>& children = ScrollPanel->Children;
	for (int32 slotIndex = 0; slotIndex < children.Num(); ++slotIndex)
	{
		if (children[slotIndex].GetWidget()->HasUserFocus(inNavigationEvent.GetUserIndex()).IsSet() ||
			children[slotIndex].GetWidget()->HasUserFocusedDescendants(inNavigationEvent.GetUserIndex()))
		{
			focusedChild = children[slotIndex].GetWidget();
			focusedChildIndex = slotIndex;
			break;
		}
	}

	if (focusedChild.IsValid())
	{
		switch (inNavigationEvent.GetNavigationType())
		{
		case EUINavigation::Up:
			focusedChildDirection = -1;
			break;
		case EUINavigation::Down:
			focusedChildDirection = 1;
			break;
		case EUINavigation::Left:
			focusedChildDirection = -1;
			break;
		case EUINavigation::Right:
			focusedChildDirection = 1;
			break;
		default:
			// If we don't handle this direction in our current orientation we can 
			// just allow the behavior of the boundary rule take over.
			return SCompoundWidget::OnNavigation(myGeometry, inNavigationEvent);
		}

		// If the focused child index is in a valid range we know we can successfully focus
		// the new child we're moving focus to.
		if (focusedChildDirection != 0)
		{
			TSharedPtr<SWidget> nextFocusableChild;

			// Search in the direction we need to move for the next focusable child of the scrollbox.
			for (int32 childIndex = focusedChildIndex + focusedChildDirection; childIndex >= 0 && childIndex < children.Num(); childIndex += focusedChildDirection)
			{
				TSharedPtr<SWidget> possiblyFocusableChild = GetKeyboardFocusableWidget(children[childIndex].GetWidget());
				if (possiblyFocusableChild.IsValid())
				{
					nextFocusableChild = possiblyFocusableChild;
					break;
				}
			}

			// If we found a focusable child, scroll to it, and shift focus.
			if (nextFocusableChild.IsValid())
			{
				InternalScrollDescendantIntoView(myGeometry, nextFocusableChild, false, NavigationDestination, NavigationScrollPadding);
				return FNavigationReply::Explicit(nextFocusableChild);
			}
		}
	}

	return SCompoundWidget::OnNavigation(myGeometry, inNavigationEvent);
}

void SScrollBox2D::OnFocusChanging(const FWeakWidgetPath& previousFocusPath, const FWidgetPath& newWidgetPath, const FFocusEvent& inFocusEvent)
{
	if (ScrollWhenFocusChanges != EScrollWhenFocusChanges::NoScroll)
	{
		if (newWidgetPath.IsValid() && newWidgetPath.ContainsWidget(this))
		{
			ScrollDescendantIntoView(newWidgetPath.GetLastWidget(), ScrollWhenFocusChanges == EScrollWhenFocusChanges::AnimatedScroll ? true : false, NavigationDestination, NavigationScrollPadding);
		}
	}
}

TSharedPtr<SWidget> SScrollBox2D::GetKeyboardFocusableWidget(TSharedPtr<SWidget> inWidget)
{
	if (EVisibility::DoesVisibilityPassFilter(inWidget->GetVisibility(), EVisibility::Visible))
	{
		if (inWidget->SupportsKeyboardFocus())
		{
			return inWidget;
		}
		else
		{
			FChildren* children = inWidget->GetChildren();
			for (int32 i = 0; i < children->Num(); ++i)
			{
				TSharedPtr<SWidget> childWidget = children->GetChildAt(i);
				TSharedPtr<SWidget> focusableWidget = GetKeyboardFocusableWidget(childWidget);
				if (focusableWidget.IsValid() && EVisibility::DoesVisibilityPassFilter(focusableWidget->GetVisibility(), EVisibility::Visible))
				{
					return focusableWidget;
				}
			}
		}
	}
	return nullptr;
}

int32 SScrollBox2D::OnPaint(const FPaintArgs& args, const FGeometry& allottedGeometry, const FSlateRect& myCullingRect, FSlateWindowElementList& outDrawElements, int32 layerId, const FWidgetStyle& inWidgetStyle, bool parentEnabled) const
{
	int32 newLayerId = SCompoundWidget::OnPaint( args, allottedGeometry, myCullingRect, outDrawElements, layerId, inWidgetStyle, parentEnabled );

	if (!bShowSoftwareCursor)
	{
		return newLayerId;
	}

	const FSlateBrush* brush = FCoreStyle::Get().GetBrush(TEXT("SoftwareCursor_Grab"));
	const FVector2f cursorSize = brush->ImageSize / allottedGeometry.Scale;

	FSlateDrawElement::MakeBox(
		outDrawElements,
		++newLayerId,
		allottedGeometry.ToPaintGeometry(cursorSize, FSlateLayoutTransform(SoftwareCursorPosition - (cursorSize * .5f))),
		brush
	);

	return newLayerId;
}

void SScrollBox2D::ScrollBar_OnUserScrolled(float inScrollOffsetFraction, int32 directionComponent)
{
	bAnimateScroll = false;

	const float contentSize = ScrollPanel->GetDesiredSize()[directionComponent];
	const FGeometry scrollPanelGeometry = FindChildGeometry(CachedGeometry, ScrollPanel.ToSharedRef());

	// Clamp to max scroll offset
	DesiredScrollOffset[directionComponent] = FMath::Min(inScrollOffsetFraction * contentSize, contentSize - scrollPanelGeometry.GetLocalSize()[directionComponent]);
	OnUserScrolled.ExecuteIfBound(DesiredScrollOffset);

	Invalidate(EInvalidateWidget::Layout);
}

void SScrollBox2D::ScrollBar_OnUserScrolledHorizontally(float inScrollOffsetFraction)
{
	ScrollBar_OnUserScrolled(inScrollOffsetFraction, 0);
}

void SScrollBox2D::ScrollBar_OnUserScrolledVertically(float inScrollOffsetFraction)
{
	ScrollBar_OnUserScrolled(inScrollOffsetFraction, 1);
}

FSlateColor SScrollBox2D::GetStartShadowOpacity(int32 directionComponent) const
{
	// The shadow should only be visible when the user needs a hint that they can scroll up.
	const float shadowOpacity = FMath::Clamp(ScrollPanel->PhysicalOffset[directionComponent] / ShadowFadeDistance, 0.0f, 1.0f);
	return FLinearColor(1.0f, 1.0f, 1.0f, shadowOpacity);
}

FSlateColor SScrollBox2D::GetStartShadowOpacity_Horizontal() const
{
	return GetStartShadowOpacity(0);
};

FSlateColor SScrollBox2D::GetStartShadowOpacity_Vertical() const
{
	return GetStartShadowOpacity(1);
}

FSlateColor SScrollBox2D::GetEndShadowOpacity(int32 directionComponent, float distanceFromBottom) const
{
	// The shadow should only be visible when the user needs a hint that they can scroll down.
	const float shadowOpacity = (distanceFromBottom * ScrollPanel->GetDesiredSize()[directionComponent] / ShadowFadeDistance);
	return FLinearColor(1.0f, 1.0f, 1.0f, shadowOpacity);
}

FSlateColor SScrollBox2D::GetEndShadowOpacity_Horizontal() const
{
	return GetEndShadowOpacity(0, HorizontalScrollBar->DistanceFromBottom());
};

FSlateColor SScrollBox2D::GetEndShadowOpacity_Vertical() const
{
	return GetEndShadowOpacity(1, VerticalScrollBar->DistanceFromBottom());
};

void SScrollBox2D::SetAllowOverscroll(EAllowOverscroll newAllowOverscroll)
{
	AllowOverscroll = newAllowOverscroll;

	if (AllowOverscroll == EAllowOverscroll::No)
	{
		OverscrollHorizontal.ResetOverscroll();
		OverscrollVertical.ResetOverscroll();
	}
}

bool SScrollBox2D::CanUseInertialScroll(const FVector2D& scrollAmount) const
{
	const FVector2D currentOverscroll = FVector2D(OverscrollHorizontal.GetOverscroll(CachedGeometry), OverscrollVertical.GetOverscroll(CachedGeometry));

	// We allow sampling for the inertial scroll if we are not in the overscroll region, or if we are scrolling outwards
	// of the overscroll region.
	return currentOverscroll.IsZero() || (currentOverscroll | scrollAmount) < 0.0f;
}

void SScrollBox2D::BeginInertialScrolling()
{
	if (!UpdateInertialScrollHandle.IsValid())
	{
		bIsScrolling = true;
		bIsScrollingActiveTimerRegistered = true;
		UpdateInertialScrollHandle = RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SScrollBox2D::UpdateInertialScroll));
		
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
	}
}

void SScrollBox2D::EndInertialScrolling()
{
	bIsScrolling = false;
	bIsScrollingActiveTimerRegistered = false;
	
	Invalidate(EInvalidateWidget::LayoutAndVolatility);
	
	if (UpdateInertialScrollHandle.IsValid())
	{
		UnRegisterActiveTimer(UpdateInertialScrollHandle.ToSharedRef());
		UpdateInertialScrollHandle.Reset();
	}

	// Zero the scroll velocity so the panel stops immediately on mouse down, even if the user does not drag
	InertialScrollManagerHorizontal.ClearScrollVelocity();
	InertialScrollManagerVertical.ClearScrollVelocity();
}

EActiveTimerReturnType SScrollBox2D::UpdateInertialScroll(double inCurrentTime, float inDeltaTime)
{
	bool keepTicking = bIsScrolling;
	
	if (bIsScrolling)
	{
		InertialScrollManagerHorizontal.UpdateScrollVelocity(inDeltaTime);
		InertialScrollManagerVertical.UpdateScrollVelocity(inDeltaTime);

		const FVector2D scrollVelocityLocal = FVector2D(InertialScrollManagerHorizontal.GetScrollVelocity(), InertialScrollManagerVertical.GetScrollVelocity()) / CachedGeometry.Scale;
		if (!scrollVelocityLocal.IsZero())
		{
			if (CanUseInertialScroll(scrollVelocityLocal))
			{
				keepTicking = true;
				ScrollBy(CachedGeometry, scrollVelocityLocal * inDeltaTime, AllowOverscroll, false);
			}
			else
			{
				InertialScrollManagerHorizontal.ClearScrollVelocity();
				InertialScrollManagerVertical.ClearScrollVelocity();
			}
		}
	}

	if (AllowOverscroll == EAllowOverscroll::Yes)
	{
		// If we are currently in overscroll, the list will need refreshing.
		// Do this before UpdateOverscroll, as that could cause GetOverscroll() to be 0
		if (OverscrollHorizontal.GetOverscroll(CachedGeometry) != 0.0f || OverscrollVertical.GetOverscroll(CachedGeometry) != 0.0f)
		{
			keepTicking = true;
		}

		OverscrollHorizontal.UpdateOverscroll(inDeltaTime);
		OverscrollVertical.UpdateOverscroll(inDeltaTime);
	}

	if (!keepTicking)
	{
		bIsScrolling = false;
		bIsScrollingActiveTimerRegistered = false;
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
		UpdateInertialScrollHandle.Reset();
	} 

	return keepTicking ? EActiveTimerReturnType::Continue : EActiveTimerReturnType::Stop;
}
