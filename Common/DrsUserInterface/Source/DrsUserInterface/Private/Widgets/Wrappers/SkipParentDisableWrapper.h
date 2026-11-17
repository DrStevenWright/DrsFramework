// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "Components/ContentWidget.h"
#include "SkipParentDisableWrapper.generated.h"

/**
 * Simple wrapper for widgets to prevent applying disable-drawing effect from parent.
 * Use case: wrap textbox nested under the button to prevent greying it out when the button gets disabled.
 */
UCLASS()
class DRSUSERINTERFACE_API USkipParentDisableWrapper : public UContentWidget
{
	GENERATED_BODY()

public:
	
	USkipParentDisableWrapper();
	
	// UPanelWidget interface
	virtual void OnSlotAdded(UPanelSlot* slot) override;
	virtual void OnSlotRemoved(UPanelSlot* slot) override;
	// End of UPanelWidget interface

	// UVisual interface
	virtual void ReleaseSlateResources(bool releaseChildren) override;
	// End of UVisual interface
	
protected:
	
	virtual TSharedRef<SWidget> RebuildWidget() override;

	TSharedPtr<SBox> MyBox;
};
