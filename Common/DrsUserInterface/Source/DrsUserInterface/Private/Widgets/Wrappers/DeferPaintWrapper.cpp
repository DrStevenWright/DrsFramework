// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#include "Widgets/Wrappers/DeferPaintWrapper.h"
#include "Components/CanvasPanelSlot.h"

class SDeferPaintWrapper : public SBox
{
	virtual int32 OnPaint(const FPaintArgs& args, const FGeometry& allottedGeometry, const FSlateRect& myCullingRect, FSlateWindowElementList& outDrawElements, int32 layerId, const FWidgetStyle& inWidgetStyle, bool parentEnabled) const override
	{
		// Similar logic to how ComboBox items are rendered always on to of other widgets.
		FArrangedChildren arrangedChildren(EVisibility::Visible);
		this->ArrangeChildren(allottedGeometry, arrangedChildren);

		// There may be zero elements in this array if our child collapsed/hidden
		if (arrangedChildren.Num() > 0)
		{
			const FArrangedWidget& theChild = arrangedChildren[0];

			// An SPopup just queues up its children to be painted after everything in this window is done painting.
			outDrawElements.QueueDeferredPainting(
				FSlateWindowElementList::FDeferredPaint(theChild.Widget, args.WithNewParent(this), theChild.Geometry, inWidgetStyle, ShouldBeEnabled(parentEnabled))
			);
		}
		return layerId;
	}
};

UDeferPaintWrapper::UDeferPaintWrapper()
{
	SetVisibilityInternal(ESlateVisibility::SelfHitTestInvisible);
}

TSharedRef<SWidget> UDeferPaintWrapper::RebuildWidget()
{
	MyBox = SNew(SDeferPaintWrapper);

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

void UDeferPaintWrapper::OnSlotAdded(UPanelSlot* slot)
{
	if (MyBox.IsValid() && slot->Content)
	{
		MyBox->SetContent(slot->Content->TakeWidget());
	}
}

void UDeferPaintWrapper::OnSlotRemoved(UPanelSlot* slot)
{
	if (MyBox.IsValid())
	{
		MyBox->SetContent(SNullWidget::NullWidget);
	}
}

void UDeferPaintWrapper::ReleaseSlateResources(bool releaseChildren)
{
	Super::ReleaseSlateResources(releaseChildren);
	MyBox.Reset();
}
