// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "Interfaces/SlateLifecycleAware.h"
#include "Widgets/Views/SortedListView.h"
#include "FilteredListView.generated.h"

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FFilterListViewFunctionSignature, const UObject*, item);


/**
 * List view that can be filtered and sorted.
 */
UCLASS()
class DRSUSERINTERFACE_API UFilteredListView : public USortedListView, public ISlateLifecycleAware
{
	GENERATED_BODY()

public:
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	
	// Set function that should be applied to filter list items
	// Can change during runtime
	UFUNCTION(BlueprintCallable)
	void SetFilterFunction(const FFilterListViewFunctionSignature& filterFunction);

	// Manual re-filter all items
	// The filter is automatically applied on adding item
	UFUNCTION(BlueprintCallable)
	void Filter();
	
	UFUNCTION(BlueprintCallable)
	void FilterItem(UObject* item);

	// Get all items that have been added to the list view. Includes also items that have been filtered
	UFUNCTION(BlueprintPure)
	const TArray<UObject*>& GetAllItems() const { return AllItems; }

protected:
	
	virtual void OnItemsChanged(const TArray<UObject*>& addedItems, const TArray<UObject*>& removedItems) override;
	
private:
	
	FFilterListViewFunctionSignature FilterFunction;

	UPROPERTY(Transient)
	TArray<UObject*> AllItems;
};
