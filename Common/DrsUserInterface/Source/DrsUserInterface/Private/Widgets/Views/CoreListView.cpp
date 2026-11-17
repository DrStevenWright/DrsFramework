// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#include "Widgets/Views/CoreListView.h"


namespace
{
	// Utility to find Horizontal/Vertical ScrollBar slot which should be always the last one based on current construction.
	template <typename TScrollSlotClass>
	typename TScrollSlotClass::FSlot* FindScrollbarSlot(FChildren* children)
	{
		if (children && children->Num() > 0)
		{
			auto sBox = StaticCastSharedPtr<TScrollSlotClass>(children->GetChildAt(children->Num() - 1).ToSharedPtr());
			if (sBox.IsValid() && sBox->IsValidSlotIndex(sBox->NumSlots() - 1))
			{
				return &sBox->GetSlot(sBox->NumSlots() - 1);
			}
		}
		
		return nullptr;
	}
}


template<typename ItemType>
void SCoreListView<ItemType>::SetWidgetStyle(const FTableViewStyle* newStyle)
{
	if (this->GetStyle() != newStyle)
	{
		this->SetStyle(newStyle);
	}
}

template<typename ItemType>
void SCoreListView<ItemType>::SetWidgetBarStyle(const FScrollBarStyle* newScrollBarStyle)
{
	if (this->ScrollBar.IsValid() && ScrollBarStyle != newScrollBarStyle)
	{
		ScrollBarStyle = newScrollBarStyle;
		this->ScrollBar->SetStyle(ScrollBarStyle);
	}
}


TSharedRef<STableViewBase> UCoreListView::RebuildListWidget()
{
	MyCoreListView = ConstructListView<SCoreListView>();

#if WITH_EDITOR
	// Inject Orientation property with 'DesignerRebuild' meta to rebuild the widget designer preview when the property changes.
	// SynchronizationProperties or Rebuilding doesn't seem to work in this case.
	if (FProperty* propOrientation = GetClass()->FindPropertyByName(FName("Orientation")))
	{
		static const FName DesignerRebuildName = FName("DesignerRebuild");
		if (!propOrientation->HasMetaData(DesignerRebuildName))
		{
			propOrientation->SetMetaData(DesignerRebuildName, "true");
		}
	}
#endif
	
	// Cache and initialize scrollbar slot settings, so it can be controlled similar to ScrollBox 
	if (MyCoreListView->Orientation == Orient_Vertical)
	{
		MyCoreListView->VerticalScrollBarSlot = FindScrollbarSlot<SHorizontalBox>(MyCoreListView->GetChildren());
		if (auto slot = MyCoreListView->VerticalScrollBarSlot)
		{
			if (auto sBox = StaticCastSharedPtr<SBox>(slot->GetWidget().ToSharedPtr()))
			{
				sBox->SetWidthOverride(-1); // Disable width override, so we can control it through thickness param
			}
		}
	}
	else // Orient_Horizontal
	{
		MyCoreListView->HorizontalScrollBarSlot = FindScrollbarSlot<SVerticalBox>(MyCoreListView->GetChildren());
		if (auto slot = MyCoreListView->VerticalScrollBarSlot)
		{
			if (auto sBox = StaticCastSharedPtr<SBox>(slot->GetWidget().ToSharedPtr()))
			{
				sBox->SetHeightOverride(-1); // Disable height override, so we can control it through thickness param
			}
		}
	}

	Orientation = MyCoreListView->Orientation;
	
	SynchronizePropertiesInternal();
	
	return MyCoreListView.ToSharedRef();
}


void UCoreListView::ReleaseSlateResources(bool releaseChildren)
{
	MyCoreListView.Reset();
	
	Super::ReleaseSlateResources(releaseChildren);
}


void UCoreListView::SynchronizePropertiesInternal()
{
	SetScrollBarPadding(GetScrollBarPadding());
	SetScrollbarThickness(ScrollbarThickness);
	SetAlwaysShowScrollbar(bAlwaysShowScrollbar);
	SetAlwaysShowScrollbarTrack(bAlwaysShowScrollbarTrack);
	SetWidgetStyle(WidgetStyle);
	SetWidgetBarStyle(ScrollBarStyle);
	SetConsumeMouseWheel(ConsumeMouseWheel);
	SetOrientation(Orientation);
}


void UCoreListView::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	SynchronizePropertiesInternal();
}


void UCoreListView::SetWidgetStyle(const FTableViewStyle& newWidgetStyle)
{
	WidgetStyle = newWidgetStyle;
	
	if (MyCoreListView.IsValid())
	{
		MyCoreListView->SetWidgetStyle(&WidgetStyle);
	}
}


void UCoreListView::SetWidgetBarStyle(const FScrollBarStyle& newScrollBarStyle)
{
	ScrollBarStyle = newScrollBarStyle;
	
	if (MyCoreListView.IsValid())
	{
		MyCoreListView->SetWidgetBarStyle(&ScrollBarStyle);
	}
}


void UCoreListView::SetScrollbarPaddingValue(EScrollbarPadDirection paddingDirection, float paddingValue)
{
	FMargin padding;
	
	switch (paddingDirection)
	{
		case EScrollbarPadDirection::Left:
			padding.Left = paddingValue;
			break;
		case EScrollbarPadDirection::Top:
			padding.Top = paddingValue;
			break;
		case EScrollbarPadDirection::Right:
			padding.Right = paddingValue;
			break;
		case EScrollbarPadDirection::Bottom:
			padding.Bottom = paddingValue;
			break;
		default:
			break;
	}
	
	SetScrollBarPadding(padding);
}


void UCoreListView::SetScrollbarThickness(const FVector2D& newScrollbarThickness)
{
	ScrollbarThickness = newScrollbarThickness;

	if (MyCoreListView.IsValid() && MyCoreListView->ScrollBar.IsValid())
	{
		MyCoreListView->ScrollBar->SetThickness(ScrollbarThickness);
	}
}


void UCoreListView::SetAlwaysShowScrollbar(bool newAlwaysShowScrollbar)
{
	bAlwaysShowScrollbar = newAlwaysShowScrollbar;

	if (MyCoreListView.IsValid() && MyCoreListView->ScrollBar.IsValid())
	{
		MyCoreListView->ScrollBar->SetScrollBarAlwaysVisible(bAlwaysShowScrollbar);
	}
}


void UCoreListView::SetAlwaysShowScrollbarTrack(bool newAlwaysShowScrollbarTrack)
{
	bAlwaysShowScrollbarTrack = newAlwaysShowScrollbarTrack;

	if (MyCoreListView.IsValid() && MyCoreListView->ScrollBar.IsValid())
	{
		MyCoreListView->ScrollBar->SetScrollBarTrackAlwaysVisible(bAlwaysShowScrollbarTrack);
	}
}


void UCoreListView::SetOrientation(EOrientation newOrientation)
{
	Orientation = newOrientation;
	
	if (MyCoreListView.IsValid() && MyCoreListView->Orientation != newOrientation)
	{
		MyCoreListView->Orientation = newOrientation;
		RequestRefresh();
		InvalidateLayoutAndVolatility();
	}
}


void UCoreListView::SetConsumeMouseWheel(EConsumeMouseWheel newConsumeMouseWheel)
{
	ConsumeMouseWheel  = newConsumeMouseWheel;
	
	if (MyCoreListView.IsValid())
	{
		MyCoreListView->ConsumeMouseWheel = newConsumeMouseWheel;
	}
}


void UCoreListView::SetClearSelectionOnClick(bool newClearSelectionOnClick)
{
	bClearSelectionOnClick = newClearSelectionOnClick;

	if (MyCoreListView.IsValid())
	{
		MyCoreListView->bClearSelectionOnClick = bClearSelectionOnClick;
	}
}


void UCoreListView::SetIsFocusable(bool newIsFocusable)
{
	if (bIsFocusable != newIsFocusable)
	{
		bIsFocusable = newIsFocusable;
		
		if (TSharedPtr<SWidget> CachedWidget = GetCachedWidget())
		{
			CachedWidget->Invalidate(EInvalidateWidgetReason::Paint);
		}
	}
}


void UCoreListView::SetReturnFocusToSelection_CoreListView(bool newReturnFocusToSelection)
{
	bReturnFocusToSelection = newReturnFocusToSelection;
	if (MyCoreListView.IsValid() && MyCoreListView->bReturnFocusToSelection != newReturnFocusToSelection)
	{
		MyCoreListView->bReturnFocusToSelection = newReturnFocusToSelection;
		MyCoreListView->Invalidate(EInvalidateWidgetReason::Paint);
	}
}


void UCoreListView::SetFixedLineScrollOffset(float newFixedLineScrollOffset)
{
	FixedLineScrollOffset = newFixedLineScrollOffset;
	if (MyCoreListView.IsValid())
	{
		MyCoreListView->SetFixedLineScrollOffset(newFixedLineScrollOffset);
	}
}


void UCoreListView::SetIsScrollAnimationEnabled(bool newEnableScrollAnimation)
{
	bEnableScrollAnimation = newEnableScrollAnimation;
	if (MyCoreListView.IsValid() && MyCoreListView->bEnableAnimatedScrolling != newEnableScrollAnimation)
	{
		MyCoreListView->SetIsScrollAnimationEnabled(newEnableScrollAnimation);
	}
}


void UCoreListView::SetIsRightClickScrollingEnabled(bool newEnableRightClickScrolling)
{
	bEnableRightClickScrolling = newEnableRightClickScrolling;
	if (MyCoreListView.IsValid())
	{
		MyCoreListView->SetIsRightClickScrollingEnabled(bEnableRightClickScrolling);
	}
}


void UCoreListView::SetAllowDragging(bool newAllowDragging)
{
	if (bAllowDragging != newAllowDragging)
	{
		bAllowDragging = newAllowDragging;
		if (!IsDesignTime() && MyCoreListView.IsValid())
		{
			MyCoreListView->RequestListRefresh();
		}
	}
}


float UCoreListView::GetTargetScrollOffset() const
{
	if (MyCoreListView.IsValid())
	{
		return MyCoreListView->GetTargetScrollOffset();
	}

	return 0.0f;
}


float UCoreListView::GetScrollDistance() const
{
	if (MyCoreListView.IsValid())
	{
		return MyCoreListView->GetScrollDistance().Y;
	}

	return 0.0f;
}


float UCoreListView::GetScrollDistanceRemaining() const
{
	if (MyCoreListView.IsValid())
	{
		return MyCoreListView->GetScrollDistanceRemaining().Y;
	}

	return 0.0f;
}


float UCoreListView::GetScrollOffsetOfEnd() const
{
	if (MyCoreListView.IsValid())
	{
		return MyCoreListView->GetNumItemsBeingObserved();
	}
	
	return 0.0f;
}


bool UCoreListView::IsUserScrolling() const
{
	if (MyCoreListView.IsValid())
	{
		return MyCoreListView->IsUserScrolling();
	}

	return false;
}
