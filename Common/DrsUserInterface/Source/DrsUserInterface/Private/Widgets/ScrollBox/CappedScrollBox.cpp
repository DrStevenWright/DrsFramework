// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#include "Widgets/ScrollBox/CappedScrollBox.h"
#include "Components/ScrollBoxSlot.h"

DEFINE_LOG_CATEGORY_STATIC(LogCappedScrollBox, Log, All);

// Custom wrapper for MyScrollBox to allow accessing and modifying
// protected params like ScrollPanel's children order.
#pragma region SCappedScrollBox

class SCappedScrollBox : public SScrollBox
{
	friend class UCappedScrollBox;
	
	TWeakObjectPtr<UCappedScrollBox> Owner;
	
public:
	
	void MoveLastSlot(int32 toIndex) const
	{
		ScrollPanel->Children.Move(ScrollPanel->Children.Num() - 1, toIndex);
	}
	
	float GetPhysicalOffset() const
	{
		return ScrollPanel->PhysicalOffset;
	}
	
	int32 GetPhysicalScrollDir(float scrollOffset) const
	{
		const float scrollDelta = scrollOffset - GetPhysicalOffset();
		return FMath::Sign(scrollDelta);
	}

	/**
	 * Helper function to get the active scroll delta which will be applied to scroll panel.
	 * Required for determining actual location of widgets during scrolling.
	 */
	float GetActiveScrollDelta() const
	{
		return GetScrollOffset() - GetPhysicalOffset();
	}
	
	virtual int32 OnPaint(const FPaintArgs& args, const FGeometry& allottedGeometry, const FSlateRect& myCullingRect, FSlateWindowElementList& outDrawElements, int32 layerId, const FWidgetStyle& inWidgetStyle, bool parentEnabled) const override
	{
		layerId = SScrollBox::OnPaint(args, allottedGeometry, myCullingRect, outDrawElements, layerId, inWidgetStyle, parentEnabled);
		if (Owner.IsValid() && Owner->bIsFillListPending)
		{
			Owner->bIsFillListPending = false;
			Owner->ProcessFillScrollList();
		}
		return layerId;
	}
};

#pragma endregion // SCappedScrollBox


#pragma region UScrollBox

TSharedRef<SWidget> UCappedScrollBox::RebuildWidget()
{
	// Copy/paste from UScrollBox with different function binding to OnUserScrolled.
	MyScrollBox = SNew(SCappedScrollBox)
		.Style(&GetWidgetStyle())
		.ScrollBarStyle(&GetWidgetBarStyle())
		.Orientation(GetOrientation())
		.ConsumeMouseWheel(GetConsumeMouseWheel())
		.NavigationDestination(GetNavigationDestination())
		.NavigationScrollPadding(GetNavigationScrollPadding())
		.ScrollWhenFocusChanges(GetScrollWhenFocusChanges())
		.AnimateWheelScrolling(IsAnimateWheelScrolling())
		.WheelScrollMultiplier(GetWheelScrollMultiplier())
		 // Bind custom function to handle spawning new items in OnUserScrolled
		.OnUserScrolled(BIND_UOBJECT_DELEGATE(FOnUserScrolled, HandleOnUserScrolled));
	
	for (UPanelSlot* PanelSlot : Slots)
	{
		if (UScrollBoxSlot* TypedSlot = Cast<UScrollBoxSlot>(PanelSlot))
		{
			TypedSlot->Parent = this;
			TypedSlot->BuildSlot(MyScrollBox.ToSharedRef());
		}
	}

	MyCappedScrollBox = StaticCastSharedPtr<SCappedScrollBox, SScrollBox>(MyScrollBox);
	MyCappedScrollBox.Pin()->Owner = this;
	return MyScrollBox.ToSharedRef();
}


void UCappedScrollBox::ClearChildren()
{
	const int32 removedChildrenCount = GetChildrenCount();
	Super::ClearChildren();

	// Make sure if all tracked data is also cleared and layout size recalculated.
	UpdateTracker.Reset();
	if (removedChildrenCount > 0)
	{
		ForceLayoutPrepass();
		ClampScrollOffsetToEnd();
	}
}

#pragma endregion // UScrollBox


#pragma region Accessors

int32 UCappedScrollBox::GetCurrentDataCount() const
{
	if (CurrentDataCountDelegate.IsBound())
	{
		return CurrentDataCountDelegate.Execute();
	}
	return CurrentDataCount;
}


void UCappedScrollBox::SetCurrentDataCount(int32 dataCount)
{
	if (CurrentDataCountDelegate.IsBound())
	{
		UE_LOG(LogCappedScrollBox, Warning, TEXT("%s: Calling SetCurrentDataCount will clear delegate bound to the value."), *GetName())
		CurrentDataCountDelegate.Clear();
	}
	CurrentDataCount = dataCount;
}


int32 UCappedScrollBox::GetDesiredDataCount() const
{
	if (DesiredDataCountDelegate.IsBound())
	{
		return DesiredDataCountDelegate.Execute();
	}
	return DesiredDataCount;
}


void UCappedScrollBox::SetDesiredDataCount(int32 desiredDataCount)
{
	if (DesiredDataCountDelegate.IsBound())
	{
		UE_LOG(LogCappedScrollBox, Warning, TEXT("%s: Calling SetDesiredDataCount will clear delegate bound to the value."), *GetName())
		DesiredDataCountDelegate.Clear();
	}
	DesiredDataCount = desiredDataCount;
}


int32 UCappedScrollBox::GetItemsCountToSpawn() const
{
	const int32 currentDataCount = GetCurrentDataCount();
	const int32 desiredDataCount = ArePlaceholderItemsAllowed() ? GetDesiredDataCount() : 0;
	return FMath::Max(currentDataCount, desiredDataCount);
}


TSubclassOf<UUserWidget> UCappedScrollBox::GetItemClassForIndex(int32 index) const
{
	if (ItemClassDelegate.IsBound())
	{
		return ItemClassDelegate.Execute(index);
	}
	return ItemClass;
}


TSubclassOf<UUserWidget> UCappedScrollBox::GetPlaceholderClassForIndex(int32 index) const
{
	if (PlaceholderClassDelegate.IsBound())
	{
		return PlaceholderClassDelegate.Execute(index);
	}
	return PlaceholderClass;
}

#pragma endregion // Accessors


#pragma region Handlers

void UCappedScrollBox::HandleOnUserScrolled(float currentOffset)
{
	// During scrolling check if we need to update or create new visible items.
	const int32 maxItemsToSpawn = GetItemsCountToSpawn();
	const int32 childrenCount = GetChildrenCount();
	const int32 scrollDir = MyCappedScrollBox.Pin()->GetPhysicalScrollDir(currentOffset);

	// Check if we have any items spawned which are possible to update.
	if (childrenCount > 0 && !UpdateTracker.IsEmpty())
	{
		// Check if there was any movement and visibility of upper/lower item bound changed.
		if (scrollDir != 0)
		{
			const int32 upperBoundIndex = FindItemIndexAtVisibleBoundary(true);
			const int32 lowerBoundIndex = FindItemIndexAtVisibleBoundary(false);
			if (UpdateTracker.UpdateVisibleBounds(upperBoundIndex, lowerBoundIndex))
			{
				FillVisibleArea(upperBoundIndex, maxItemsToSpawn);
			}
		}
	}

	// Notify if we scrolled to the bottom of the list.
	// Might be useful for custom fetching/spawning logic.
	if (scrollDir > -1 && OnUserScrolledToEnd.IsBound())
	{
		if (currentOffset >= GetScrollOffsetOfEnd())
		{
			OnUserScrolledToEnd.Broadcast();
		}
	}
	
	// Call default parent logic (broadcast OnUserScrolled).
	SlateHandleUserScrolled(currentOffset);
}


UWidget* UCappedScrollBox::HandleCreateOrUpdateItemForIndex(UWidget* spawnedItem, int32 index, int32 dataCount)
{
	// Check if we need to use placeholder item.
	ensure(dataCount == GetCurrentDataCount());
	const bool canUseDefaultItem = dataCount > index;
	const TSubclassOf<UUserWidget> itemClassForIndex = canUseDefaultItem ? GetItemClassForIndex(index) : GetPlaceholderClassForIndex(index);

	// Check if we need to create a new item.
	if (!spawnedItem || !spawnedItem->IsA(itemClassForIndex))
	{
		// Item at this index was null or desired class changed - spawn new item (or placeholder if provided).
		if (canUseDefaultItem && OnCreateOrUpdateItem.IsBound())
		{
			if (UWidget* newItem = OnCreateOrUpdateItem.Execute(nullptr, itemClassForIndex, index))
			{
				UE_LOG(LogCappedScrollBox, Verbose, TEXT("%s: OnCreateOrUpdateItem: Spawning at [%i], Old item: '%s', New item: '%s' "), *GetName(), index, spawnedItem ? *spawnedItem->GetName() : TEXT("null"), *newItem->GetName());
				return newItem;
			}
			UE_LOG(LogCappedScrollBox, Warning, TEXT("%s: Failed to create widget at index [%i]. Make sure OnCreateOrUpdateItem always retunrs valid item."), *GetName(), index)
		}

		// Doesn't seem necessary to expose spawning placeholder items to blueprints.
		UWidget* newItem = CreateWidget(this, itemClassForIndex);
		UE_LOG(LogCappedScrollBox, Verbose, TEXT("%s: Spawning Item at [%i], Old item: '%s', New item: '%s' "), *GetName(), index, spawnedItem ? *spawnedItem->GetName() : TEXT("null"), *newItem->GetName());
		return newItem;
	}

	// Check if we need to update item at this index. Skip update logic for placeholders.
	if (canUseDefaultItem)
	{
		const TWeakObjectPtr<UWidget>* updatedWidget = UpdateTracker.UpdatedItemsMap.Find(index);
		if (!updatedWidget || *updatedWidget != spawnedItem)
		{
			// Item with desired class exists at this index but needs to be updated with new data.
			if (OnCreateOrUpdateItem.IsBound())
			{
				if (UWidget* newItem = OnCreateOrUpdateItem.Execute(spawnedItem, itemClassForIndex, index))
				{
					UE_LOG(LogCappedScrollBox, Verbose, TEXT("%s: OnCreateOrUpdateItem: Updating at [%i], Item: '%s' "), *GetName(), index, *newItem->GetName())
					return newItem;
				}
			}
		}
	}
	
	return spawnedItem;
}

#pragma endregion // Handlers


#pragma region Fill Logic

void UCappedScrollBox::FillScrollList(const FName dataId)
{
	if (!dataId.IsNone() && UpdateTracker.DataId != dataId)
	{
		UpdateTracker.Reset();
		UpdateTracker.DataId = dataId;
	}
	
	// Postpone initial update if was called too early until ScrollBox geometry is valid.
	// Otherwise, it won't be possible to check bounds of scroll area.
	if (!MyCappedScrollBox.IsValid())
	{
		bIsFillListPending = true;
		return;
	}
	if (bIsFillListPending)
	{
		return;
	}

	ProcessFillScrollList();
}


void UCappedScrollBox::ProcessFillScrollList()
{
	const int32 maxItemsToSpawn = GetItemsCountToSpawn();
	const int32 startIndex = INDEX_NONE;
	
	FillVisibleArea(startIndex, maxItemsToSpawn);
	const int32 upperBoundIndex = FindItemIndexAtVisibleBoundary(true);
	const int32 lowerBoundIndex = FindItemIndexAtVisibleBoundary(false);
	UpdateTracker.UpdateVisibleBounds(upperBoundIndex, lowerBoundIndex);
}

void UCappedScrollBox::FillVisibleArea(int32 startIndex, int32 itemsToSpawn)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UCappedScrollBox::FillVisibleArea)
	
	const int32 currentDataCount = GetCurrentDataCount();

	// Remove excess items and adjust the layout if needed
	if (RemoveExcessiveItems(itemsToSpawn))
	{
		ForceLayoutPrepass();
		SynchronizeProperties();
		ClampScrollOffsetToEnd();
	}

	// Try to start from top visible index if not provided.
	if (startIndex == INDEX_NONE)
	{
		startIndex = HasAnyChildren() ? FMath::Max(0, FindItemIndexAtVisibleBoundary(true)) : 0;
	}
	
	UE_LOG(LogCappedScrollBox, Verbose, TEXT("%s: FillVisibleArea START [%i]: ChildrenCount: %i, itemsToSpawn: %i"), *GetName(), startIndex, GetChildrenCount(), itemsToSpawn);
	for (int32 i = startIndex; i < itemsToSpawn; ++i)
	{
		UWidget* spawnedItem = GetChildAt(i);

		// Stop updating if the current item is outside the visible area.
		if (spawnedItem && !IsItemInViewArea(spawnedItem, ExtraFillOffset))
		{
			UE_LOG(LogCappedScrollBox, Verbose, TEXT("%s: FillVisibleArea [%i]: Skipped Updating more widgets. spawnedItem '%s' is out of visible area."), *GetName(), i, *spawnedItem->GetName());
			break;
		}

		// Create or update the item at the current index
		UWidget* newItem = HandleCreateOrUpdateItemForIndex(spawnedItem, i, currentDataCount);
		if (!newItem)
		{
			UE_LOG(LogCappedScrollBox, Error, TEXT("%s: FillVisibleArea [%i]: Failed. Couldn't create new item widget! Make sure that OnCreateOrUpdateItem always returns valid item"), *GetName(), i);
			break;
		}

		// Mark the item as updated (so it can be skipped next time) and replace or add it to the scroll box.
		UpdateTracker.MarkItemUpdated(i, newItem);
		if (newItem != spawnedItem)
		{
			if (spawnedItem)
			{
				ReplaceItemAt(newItem, i);
			}
			else
			{
				AddChild(newItem);
			}
			ForceLayoutPrepass();

			// Stop the process if the new item is not in the visible area
			if (!IsItemInViewArea(newItem, ExtraFillOffset))
			{
				UE_LOG(LogCappedScrollBox, Verbose, TEXT("%s: FillVisibleArea [%i]: Skipped checking more items. '%s' is out of visible area."), *GetName(), i, *newItem->GetName());
				break;
			}
		}
	}
}


bool UCappedScrollBox::ReplaceItemAt(UWidget* newItem, int32 index)
{
	if (!Slots.IsValidIndex(index))
	{
		return false;
	}
	
	UPanelSlot* slot = Slots[index];
	UWidget* oldWidget = slot->Content;
	
	// Make sure two widgets exist and are different.
	if (!ensure(newItem) || !oldWidget || !ensure(oldWidget != newItem))
	{
		return false;
	}
	
	// Handle removing old widget from slot.
	oldWidget->Slot = nullptr;
	
	// Remove the widget from the live slot if it exists.
	const TSharedPtr<SWidget> oldsSWidget = oldWidget->GetCachedWidget();
	const TSharedPtr<SCappedScrollBox> myCappedScrollBox = MyCappedScrollBox.Pin();
	if (oldsSWidget.IsValid() && myCappedScrollBox.IsValid())
	{
		myCappedScrollBox->RemoveSlot(oldsSWidget.ToSharedRef());
	}

	slot->ReleaseSlateResources(true);
	slot->Parent = nullptr;
	slot->Content = nullptr;

	// Handle filling slot with new widget.
	if (newItem)
	{
		newItem->RemoveFromParent();
		newItem->Slot = slot;
	}

	slot->Content = newItem;
	if (myCappedScrollBox.IsValid())
	{
		// New slot is always built and added at the end so we need to move it back to desired index.
		CastChecked<UScrollBoxSlot>(slot)->BuildSlot(myCappedScrollBox.ToSharedRef());
		myCappedScrollBox->MoveLastSlot(index);
	}
	
	slot->SynchronizeProperties();
	InvalidateLayoutAndVolatility();
	
	return true;
}


bool UCappedScrollBox::RemoveExcessiveItems(int32 dataCount)
{
	bool anyChildRemoved = false;
	if (dataCount < 0)
	{
		dataCount = FindItemIndexAtVisibleBoundary(false) + 1;
	}
	dataCount = FMath::Max(0, dataCount);
	while (GetChildrenCount() > dataCount)
	{
		const int32 removeIndex = GetChildrenCount() - 1;
		UpdateTracker.OnEndItemRemoved(removeIndex);
		anyChildRemoved |= RemoveChildAt(removeIndex);
	}

	if (anyChildRemoved)
	{
		UpdateTracker.ClampVisibleBounds(GetChildrenCount() - 1);
	}
	
	return anyChildRemoved;
}


void UCappedScrollBox::MakeItemsDirty(const TArray<int32> items, bool refreshScrollList)
{
	bool anyItemRemovedFromTracker = false;
	for (const int32 itemIndex : items)
	{
		if (UpdateTracker.UpdatedItemsMap.Remove(itemIndex))
		{
			anyItemRemovedFromTracker = true;
		}
	}

	if (refreshScrollList && anyItemRemovedFromTracker)
	{
		FillScrollList(UpdateTracker.DataId);
	}
}


void UCappedScrollBox::MakeDataDirty(bool refreshScrollList)
{
	const bool dataRemovedFromTracker = !UpdateTracker.IsEmpty();
	UpdateTracker.Reset();
	if (dataRemovedFromTracker && refreshScrollList)
	{
		FillScrollList(UpdateTracker.DataId);
	}
}


void UCappedScrollBox::SetScrollListDataId(const FName dataId)
{
	if (UpdateTracker.DataId != dataId)
	{
		UpdateTracker.DataId = dataId;
		MakeDataDirty(true);
	}
}


bool UCappedScrollBox::ArePlaceholderItemsAllowed() const
{
	return (PlaceholderClassDelegate.IsBound() || PlaceholderClass);
}

#pragma endregion // FillLogic


#pragma region Search ScrollBox

int32 UCappedScrollBox::FindItemIndexAtVisibleBoundary(bool topBoundary) const
{
	if (!MyCappedScrollBox.IsValid())
	{
		return INDEX_NONE;
	}

	const TSharedPtr<SCappedScrollBox> myCappedScrollBox = MyCappedScrollBox.Pin();
	const FGeometry& myGeometry = myCappedScrollBox->GetCachedGeometry();
	const FGeometry& scrollPanelGeom = myCappedScrollBox->ScrollPanel->GetCachedGeometry();
	
	FArrangedChildren arrangedChildren(EVisibility::Visible);
	myCappedScrollBox->ScrollPanel->ArrangeChildren(scrollPanelGeom, arrangedChildren);

	const float activeScrollDelta = myCappedScrollBox->GetActiveScrollDelta();
	const FVector2D absBoundaryPos = myGeometry.GetAbsolutePositionAtCoordinates(FVector2D(0.5f, topBoundary ? 0.0f : 1.0f)) + FVector2D(0.0f, activeScrollDelta);
	const int32 itemIndex = myCappedScrollBox->FindChildUnderPosition(arrangedChildren, absBoundaryPos);
	if (!topBoundary && itemIndex == INDEX_NONE && HasAnyChildren())
	{
		return GetChildrenCount() - 1;
	}
	return itemIndex;
}


void UCappedScrollBox::ClampScrollOffsetToEnd()
{
	const float scrollEndOffset = GetScrollOffsetOfEnd();
	if (GetScrollOffset() > scrollEndOffset)
	{
		SetScrollOffset(scrollEndOffset);
	}
}


bool UCappedScrollBox::IsItemInViewArea(const UWidget* item, float extraOffset) const
{
	if (!item)
	{
		return false;
	}

	if (extraOffset < 0.0f)
	{
		return true;
	}
	
	auto getScrollComponentFromVector = [this](const FVector2D& vector)
	{
		return GetOrientation() == Orient_Vertical ? vector.Y : vector.X;
	};

	const TSharedPtr<SCappedScrollBox> myCappedScrollBox = MyCappedScrollBox.Pin();
	const FGeometry& myGeometry = myCappedScrollBox->GetCachedGeometry();
	const TSharedPtr<SWidget> widgetToFind = item->GetCachedWidget();
	const FGeometry widgetGeometry = myCappedScrollBox->FindChildGeometry(myGeometry, widgetToFind.ToSharedRef());

	const float activeScrollDelta = myCappedScrollBox->GetActiveScrollDelta();
	const float viewStartPosition = -extraOffset;
	const float viewEndPosition = getScrollComponentFromVector(myGeometry.GetLocalSize() + extraOffset);
	
	const float widgetStartPosition = getScrollComponentFromVector(myGeometry.AbsoluteToLocal(widgetGeometry.GetAbsolutePosition())) - activeScrollDelta;
	if (widgetStartPosition > viewEndPosition)
	{
		return false;
	}
	
	const float widgetEndPosition = widgetStartPosition + getScrollComponentFromVector(widgetGeometry.GetLocalSize());
	if (widgetEndPosition < viewStartPosition)
	{
		return false;
	}
	
	return true;
}

#pragma endregion // Search ScrollBox


#pragma region UpdateTracker

void UCappedScrollBox::FUpdateTracker::Reset()
{
	UpdatedItemsMap.Reset();
	ResetBounds();
}


bool UCappedScrollBox::FUpdateTracker::IsEmpty() const
{
	return UpdatedItemsMap.Num() == 0;
}


void UCappedScrollBox::FUpdateTracker::ResetBounds()
{
	LastVisibleTopBoundaryItemIndex = TNumericLimits<int32>::Max();
	LastVisibleBottomBoundaryItemIndex = INDEX_NONE;
}


void UCappedScrollBox::FUpdateTracker::MarkItemUpdated(int32 index, UWidget* widget)
{
	UpdatedItemsMap.Add(index, widget);
}


void UCappedScrollBox::FUpdateTracker::OnEndItemRemoved(int32 index)
{
	UpdatedItemsMap.Remove(index);
}


bool UCappedScrollBox::FUpdateTracker::UpdateVisibleBounds(int32 topIndex, int32 bottomIndex)
{
	const bool anyBoundChanged = LastVisibleTopBoundaryItemIndex != topIndex || LastVisibleBottomBoundaryItemIndex != bottomIndex;
	LastVisibleTopBoundaryItemIndex = topIndex;
	LastVisibleBottomBoundaryItemIndex = bottomIndex;
	return anyBoundChanged;
}


void UCappedScrollBox::FUpdateTracker::ClampVisibleBounds(int32 maxIndex)
{
	LastVisibleBottomBoundaryItemIndex = FMath::Clamp(LastVisibleBottomBoundaryItemIndex, 0, maxIndex);
	LastVisibleTopBoundaryItemIndex = FMath::Min(LastVisibleTopBoundaryItemIndex, LastVisibleBottomBoundaryItemIndex);
}

#pragma endregion // UpdateTracker
