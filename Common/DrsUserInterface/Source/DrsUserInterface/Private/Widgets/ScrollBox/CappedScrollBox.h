// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "CoreScrollBox.h"
#include "CappedScrollBox.generated.h"

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(TSubclassOf<UUserWidget>, FGetItemClassForIndex, int32, index);
DECLARE_DYNAMIC_DELEGATE_RetVal_ThreeParams(UWidget*, FCreateOrUpdateItem, UWidget*, updateItem, TSubclassOf<UUserWidget>, ItemClassForIndex, int32, index);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUserScrolledToEnd);


/**
 * ScrollBox which restricts number of items spawned at once, preventing performance spikes.
 * Only the minimum required items will be spawned initially to fill the scroll area. Additional elements will then be
 * spawned/updated upon the user's scrolling. Optional placeholders may be used for data that is not yet available.
 */
UCLASS()
class DRSUSERINTERFACE_API UCappedScrollBox : public UCoreScrollBox
{
	GENERATED_BODY()

	friend class SCappedScrollBox;
	
public:

	//------------------------------------------
	// Accessors
	//------------------------------------------
	
	/**
	 * Returns the data count which will be used for spawning and updating proper scroll items.
	 */
	UFUNCTION(BlueprintPure, Category = "CappedScrollBox")
	int32 GetCurrentDataCount() const;
	
	/**
	 * Sets value for data count used for spawning and updating proper scroll items.
	 * Caution! Calling it will reset CurrentDataCount binding delegate.
	 */
	UFUNCTION(BlueprintCallable, Category = "CappedScrollBox")
	void SetCurrentDataCount(int32 dataCount);
	
	/**
	 * Returns the desired count of data elements for the scroll box, which may exceed the current count.
	 * Used to determine number of placeholder items to spawn.
	 */
	UFUNCTION(BlueprintPure, Category = "CappedScrollBox")
	int32 GetDesiredDataCount() const;
	
	/**
	 * Sets value for desired count of data elements for the scroll box, which may exceed the current count.
	 * Calling it will reset DesiredDataCount binding delegate.
	 */
	UFUNCTION(BlueprintCallable, Category = "CappedScrollBox")
	void SetDesiredDataCount(int32 desiredDataCount);
	
	/**
	 * Returns number of items which will be used to fill scrollbox.
	 * Scroll list will be filled with placeholders if desired data is greater than currant data count. 
	 */
	UFUNCTION(BlueprintPure, Category = "CappedScrollBox")
	int32 GetItemsCountToSpawn() const;
	
	/**
	 * Get the widget class used for spawning items for a specific data index.
	 * @param index CurrentData index
	 */
	UFUNCTION(BlueprintPure, Category = "CappedScrollBox")
	TSubclassOf<UUserWidget> GetItemClassForIndex(int32 index) const;
	
	/**
	 * Get the widget class used for spawning temporary items to fill scrollbox if actual data is not available yet.
	 * @param index Index including DesiredDataCount range.
	 */
	UFUNCTION(BlueprintPure, Category = "CappedScrollBox")
	TSubclassOf<UUserWidget> GetPlaceholderClassForIndex(int32 index) const;
	
	//------------------------------------------
	// Fill ScrollList API
	//------------------------------------------
	
	/**
	 * Fills or updates the scroll list with new data.
	 * Call to notify the scrollbox whenever there was any change in provided data.
	 * Only items withing visible will be updated. Others will update during scrolling.
	 * @param dataId Identifier for the new data set. If it changes, all currently spawned items are updated.
	 * Pass 'None' if to keep previous data unchanged.
	 * Note: dataId is only relevant if scrollbox handles more than one data type for items.
	 */
	UFUNCTION(BlueprintCallable, Category = "CappedScrollBox")
	void FillScrollList(const FName dataId);

	/**
	 * Remove items from already updated list, so they can be updated again when FillScrollList is called.
	 * The items will be updated as soon as they became visible in scroll area.
	 * Can be used if only specific items needs to be updated with new data.
	 * @param items Array of item indices to mark as dirty.
	 * @param refreshScrollList If true, 'FillScrollList' will be called to refresh visible area after marking items.
	 */
	UFUNCTION(BlueprintCallable, Category = "CappedScrollBox")
	void MakeItemsDirty(const TArray<int32> items, bool refreshScrollList = true);

	/**
	 * Remove all items from already updated list, so they can be updated again when FillScrollList is called.
	 * Items will be updated as soon as they became visible in scroll area.
	 * @param refreshScrollList If true, 'FillScrollList' will be called to refresh visible area after marking items.
	 */
	UFUNCTION(BlueprintCallable, Category = "CappedScrollBox")
	void MakeDataDirty(bool refreshScrollList = true);

	/**
	 * Updates the DataId for the scroll list items and marks all items as "dirty" if data changed.
	 * Note: relevant only if using more than one data type for scroll box items.
	 * @param dataId The new data identifier for the scroll list items.
	 */
	UFUNCTION(BlueprintCallable, Category = "CappedScrollBox")
	void SetScrollListDataId(const FName dataId);
	
	/**
	 * Determines if a specific item is within the visible area of the scroll box.
	 * @param item The widget item to check.
	 * @param extraOffset Additional space outside the visible area to consider.
	 * @return True if the item is within the visible area (plus extra offset).
	 */
	UFUNCTION(BlueprintCallable, Category = "CappedScrollBox")
	bool IsItemInViewArea(const UWidget* item, float extraOffset = 0.0f) const;

	/**
	 * Finds the index of the child at the top or bottom boundary of the visible scroll panel area.
	 * @param topBoundary If true, searches for the top visible child; otherwise, the bottom.
	 * @return The index of the child or -1 if no child is visible near the top or bottom boundary.
	 */
	UFUNCTION(BlueprintCallable, Category = "CappedScrollBox")
	int32 FindItemIndexAtVisibleBoundary(bool topBoundary) const;
	
protected:

	//------------------------------------------
	// UScrollBox Interface
	//------------------------------------------
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ClearChildren() override;

	//------------------------------------------
	// Properties
	//------------------------------------------

	// Override to customize widget creation logic based on data at index.
	// If 'updateItem' property is valid, then existing item can be updated with new data instead of re-creating it.
	UPROPERTY(EditAnywhere, meta = (IsBindableEvent = "True"),  Category = "CappedScrollBox")
	FCreateOrUpdateItem OnCreateOrUpdateItem;
	
	// Class used for spawning ScrollBox widget items.
	// Optionally, bind function to specify custom classes per element index.
	UPROPERTY(EditAnywhere, Category = "CappedScrollBox")
	TSubclassOf<UUserWidget> ItemClass;

	// A bindable delegate for ItemClass.
	UPROPERTY()
	FGetItemClassForIndex ItemClassDelegate;

	// Optional item to use if data is not available yet.
	UPROPERTY(EditAnywhere, Category = "CappedScrollBox")
	TSubclassOf<UUserWidget> PlaceholderClass;

	// A bindable delegate for PlaceholderClass.
	UPROPERTY()
	FGetItemClassForIndex PlaceholderClassDelegate;
	
	// Current number of elements considered by this ScrollBox.
	// Use binding to return actual value from desired data list.
	// Caution! Setter will reset bound delegate.
	UPROPERTY(EditAnywhere, BlueprintSetter = SetCurrentDataCount, BlueprintGetter = GetCurrentDataCount, Category = "CappedScrollBox")
	int32 CurrentDataCount = 0;

	// A bindable delegate for CurrentDataCount.
	UPROPERTY()
	FGetInt32 CurrentDataCountDelegate;

	// How many items should be shown. If current data is not available, placeholders will be displayed.
	// Caution! Setter will reset bound delegate.
	UPROPERTY(EditAnywhere, BlueprintSetter = SetDesiredDataCount, BlueprintGetter = GetDesiredDataCount, Category = "CappedScrollBox")
	int32 DesiredDataCount = 0;

	// A bindable delegate for DesiredDataCount.
	UPROPERTY()
	FGetInt32 DesiredDataCountDelegate;
	
	// How much space outside visible area should be considered to fill.
	// ExtraFillOffset < 0 - all elements will be spawned at once.
	UPROPERTY(EditAnywhere, Category = "CappedScrollBox")
	float ExtraFillOffset = 200.0f;

	// Called when used scrolled to end.
	UPROPERTY(BlueprintAssignable, Category = "CappedScrollBox|Event")
	FOnUserScrolledToEnd OnUserScrolledToEnd;
	
private:

	//------------------------------------------
	// Internal scroll handlers
	//------------------------------------------

	/**
	 * Function handling user scroll events.
	 * Contains logic for checking if new items can be spawned or updated during scrolling.
	 * @param currentOffset The current scroll offset.
	 */
	void HandleOnUserScrolled(float currentOffset);

	/**
	 * Creates or updates an item for a given index.
	 * @param spawnedItem The current item widget at the index, if exists.
	 * @param index The index for which to create or update the item.
	 * @param dataCount The current count of data elements.
	 * @return The created or updated widget.
	 */
	UWidget* HandleCreateOrUpdateItemForIndex(UWidget* spawnedItem, int32 index, int32 dataCount);

	//------------------------------------------
	// Internal Fill ScrollList API
	//------------------------------------------

	/**
	 * Process call for FillVisibleArea and keeps track for last checked top/bottom items.
	 * Can be called only if MyScrollBox widget has been created and contains valid geometry.
	 */
	void ProcessFillScrollList();
	
	/**
	 * The core logic for filling the scrollbox visible area with items.
	 * The function stops if an item is not visible in the current scroll area.
	 * @param startIndex The index from where to start populating items.
	 * @param itemsToSpawn The total number of items to populate or update in the scroll box.
	 */
	void FillVisibleArea(int32 startIndex, int32 itemsToSpawn);
	
	/**
	 * Remove excessive spawned items to match provided data count.
	 * @param dataCount The current count of data elements which scrollbox will be reduced to.
	 * @return 
	 */
	bool RemoveExcessiveItems(int32 dataCount);
	
	/**
	 * Removes existing widget from slot and replaces it with a new one.
	 */
	bool ReplaceItemAt(UWidget* newItem, int32 index);

	/**
	 * Keeps internal scroll offset value synced.
	 */
	void ClampScrollOffsetToEnd();
	
	/**
	 * Checks if our scroll box setup allows spawning placeholder items.
	 */
	bool ArePlaceholderItemsAllowed() const;

	//------------------------------------------
	// Internal members
	//------------------------------------------

	// Cached pointer of MyScrollBox casted to SCappedScrollBox;
	TWeakPtr<class SCappedScrollBox> MyCappedScrollBox;
	
	// True if FillList will be postponed until scroll box widget has valid geometry.
	bool bIsFillListPending = false;
	
	// Internal helper structure to keep track of updated items and scroll boundaries.
	struct FUpdateTracker
	{
		FName DataId = NAME_None;
		int32 LastVisibleTopBoundaryItemIndex		= TNumericLimits<int32>::Max();	// 0: 1st element in scroll list
		int32 LastVisibleBottomBoundaryItemIndex	= INDEX_NONE;					// children count - 1: bottom/last element.
		TMap<int32, TWeakObjectPtr<UWidget>> UpdatedItemsMap;
		
		void Reset();
		bool IsEmpty() const;
		void ResetBounds();
		void MarkItemUpdated(int32 index, UWidget* widget);
		void OnEndItemRemoved(int32 index);
		bool UpdateVisibleBounds(int32 topIndex, int32 bottomIndex);
		void ClampVisibleBounds(int32 maxIndex);
	};

	FUpdateTracker UpdateTracker;
};
