// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#include "Widgets/Wrappers/SkipParentDisableWrapper.h"
#include "Components/CanvasPanelSlot.h"

class SSkipParentDisableWrapper : public SBox
{
	virtual int32 OnPaint(const FPaintArgs& args, const FGeometry& allottedGeometry, const FSlateRect& myCullingRect, FSlateWindowElementList& outDrawElements, int32 layerId, const FWidgetStyle& inWidgetStyle, bool parentEnabled) const override
	{
		// We skip default widgets check for parent (IsParentEnabled && IsEnabled()) for applying disabled color.
		const bool shouldDrawEnabled = IsEnabled();
		return SBox::OnPaint(args, allottedGeometry, myCullingRect, outDrawElements, layerId, inWidgetStyle, shouldDrawEnabled);
	}
};

USkipParentDisableWrapper::USkipParentDisableWrapper()
{
	SetVisibilityInternal(ESlateVisibility::SelfHitTestInvisible);
}

TSharedRef<SWidget> USkipParentDisableWrapper::RebuildWidget()
{
	MyBox = SNew(SSkipParentDisableWrapper);

	if ( GetChildrenCount() > 0 )
	{
		const UPanelSlot* contentSlot = GetContentSlot();
		if (contentSlot->Content)
		{
			MyBox->SetContent(contentSlot->Content->TakeWidget());
		}
	}
	
	return MyBox.ToSharedRef();
}

void USkipParentDisableWrapper::OnSlotAdded(UPanelSlot* slot)
{
	if (MyBox.IsValid() && slot->Content)
	{
		MyBox->SetContent(slot->Content->TakeWidget());
	}
}

void USkipParentDisableWrapper::OnSlotRemoved(UPanelSlot* slot)
{
	if (MyBox.IsValid())
	{
		MyBox->SetContent(SNullWidget::NullWidget);
	}
}

void USkipParentDisableWrapper::ReleaseSlateResources(bool releaseChildren)
{
	Super::ReleaseSlateResources(releaseChildren);
	MyBox.Reset();
}
