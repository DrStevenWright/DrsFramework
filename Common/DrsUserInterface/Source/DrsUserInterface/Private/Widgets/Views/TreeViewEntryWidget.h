// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "TreeViewEntryWidget.generated.h"

/**
 * UserWidget class that inherits the UserObjectListEntry interface, in order to support tree views.
 */
UCLASS()
class DRSUSERINTERFACE_API UTreeViewEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
};
