// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#include "Widgets/Views/FilteredListView.h"


void UFilteredListView::ReleaseSlateResources(bool bReleaseChildren)
{
	NotifySlateResourcesReleased(this);
		
	Super::ReleaseSlateResources(bReleaseChildren);
}

void UFilteredListView::SetFilterFunction(const FFilterListViewFunctionSignature& filterFunction)
{
	FilterFunction = filterFunction;
	Filter();
}


void UFilteredListView::Filter()
{
	if (FilterFunction.IsBound())
	{
		ListItems = AllItems.FilterByPredicate([=, this](const UObject* item)
		{
			return FilterFunction.Execute(item);
		});

		// Resort as we might have added additional items
		Sort();
	}
}


void UFilteredListView::FilterItem(UObject* item)
{
	const bool bFiltered = FilterFunction.Execute(item);
	for (int32 i = 0; i < ListItems.Num(); i++)
	{
		if (ListItems[i] == item)
		{
			if (!bFiltered)
			{
				ListItems.RemoveAt(i);
				RequestRefresh();
			}
			else if (EnsureOrder(item))
			{
				// Even though the filter state didnt change sort order may need to be restored
				RequestRefresh();
			}
			return;
		}
	}

	if (bFiltered)
	{
		ensure(AllItems.Contains(item));
	 
		ListItems.Add(item);
		EnsureOrder(item);
		RequestRefresh();  
	}
	else if (EnsureOrder(item))
	{
		RequestRefresh();
	}
}


void UFilteredListView::OnItemsChanged(const TArray<UObject*>& addedItems, const TArray<UObject*>& removedItems)
{
	if (FilterFunction.IsBound())
	{
		for (int32 i = addedItems.Num() - 1; i >= 0; i--)
		{
			UObject* item = addedItems[i];
			if (!FilterFunction.Execute(item))
			{
				ListItems.Remove(item);
			}
			AllItems.Add(item);
		}
		
		for (UObject* item : removedItems)
		{
			AllItems.Remove(item);
		}
	}

	Super::OnItemsChanged(addedItems, removedItems);
}
