// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "CoreListView.h"
#include "SortedListView.generated.h"

DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(bool, FSortListViewFunctionSignature, const UObject*, itemA, const UObject*, itemB);


/**
 * List view that can be sorted.
 */
UCLASS()
class DRSUSERINTERFACE_API USortedListView : public UCoreListView
{
	GENERATED_BODY()
	
public:
	
	// Set function that does sorting of the list view
	// Can be changed during runtime
	UFUNCTION(BlueprintCallable)
	void SetSortFunction(const FSortListViewFunctionSignature& sortFunction);

	// Manual sort all items
	// Adding/Removing an item will automatically preserve sorted order
	UFUNCTION(BlueprintCallable)
	void Sort();
	
protected:
	
	virtual void OnItemsChanged(const TArray<UObject*>& addedItems, const TArray<UObject*>& removedItems) override;

	void SortAll();
	bool EnsureOrder(UObject* item);
	
private:
	
	FSortListViewFunctionSignature SortFunction;
};
