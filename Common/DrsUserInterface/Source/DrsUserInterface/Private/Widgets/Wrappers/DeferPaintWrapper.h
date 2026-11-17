// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "Components/ContentWidget.h"
#include "DeferPaintWrapper.generated.h"

/**
 * Wrapper for widgets to allow drawing them on top regardless of their hierarchy.
 * Use case: wrap tooltip message box into 'DeferPaintWrapper' to allow showing it on top of ComboBox dropdown list without being clipped.
 */
UCLASS()
class DRSUSERINTERFACE_API UDeferPaintWrapper : public UContentWidget
{
	GENERATED_BODY()

public:
	
	UDeferPaintWrapper();
	
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
