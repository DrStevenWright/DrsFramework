// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#include "Widgets/Views/SortedListView.h"


void USortedListView::SetSortFunction(const FSortListViewFunctionSignature& sortFunction)
{
	SortFunction = sortFunction;
	Sort();
}


void USortedListView::Sort()
{
	if (SortFunction.IsBound())
	{
		SortAll();
		RequestRefresh();
	}
}


void USortedListView::OnItemsChanged(const TArray<UObject*>& addedItems, const TArray<UObject*>& removedItems)
{
	Super::OnItemsChanged(addedItems, removedItems);

	if (addedItems.Num() == 1)
	{
		EnsureOrder(addedItems[0]);
	}
	else if (addedItems.Num() > 1)
	{
		// Save time by using a proper sort algorithm to ensure the order of all added items
		SortAll();
	}
}


void USortedListView::SortAll()
{
	if (ListItems.Num() > 1 && SortFunction.IsBound())
	{
		ListItems.Sort([=, this](const UObject& itemA, const UObject& itemB)
		{
			return SortFunction.Execute(&itemA, &itemB);
		});
	}
}


bool USortedListView::EnsureOrder(UObject* item)
{
	int32 index = GetIndexForItem(item);
	if (index < 0 || !SortFunction.IsBound())
	{
		return false;
	}

	const bool predecessorInOrder = index > 0 ? SortFunction.Execute(ListItems[index - 1], ListItems[index]) : true;
	const bool successorInOrder = index + 1 < ListItems.Num() ? SortFunction.Execute(ListItems[index], ListItems[index + 1]) : true;

	bool bChanged = false; 
	if (!predecessorInOrder)
	{
		while (index > 0)
		{
			if (SortFunction.Execute(ListItems[index - 1], ListItems[index]))
			{
				return bChanged;
			}
			ListItems.Swap(index-1, index);
			index--;
			
			bChanged = true;
		}
	}
	else if (!successorInOrder)
	{
		while (index < ListItems.Num() - 1)
		{
			if (SortFunction.Execute(ListItems[index], ListItems[index + 1]))
			{
				return bChanged;
			}
			ListItems.Swap(index, index+1);
			index++;

			bChanged = true;
		}
	}
	
	return bChanged;
}
