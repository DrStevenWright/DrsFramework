// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#include "Widgets/ScrollBox/ScrollBoxSlot2D.h"
#include "Components/Widget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ScrollBoxSlot2D)


UScrollBoxSlot2D::UScrollBoxSlot2D(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
{
	HorizontalAlignment = HAlign_Fill;
	VerticalAlignment = VAlign_Fill;
	Size = FSlateChildSize(ESlateSizeRule::Automatic);
}

void UScrollBoxSlot2D::BuildSlot(TSharedRef<SScrollBox2D> scrollBox)
{
	scrollBox->AddSlot()
		.Padding(Padding)
		.HAlign(HorizontalAlignment)
		.VAlign(VerticalAlignment)
		.Expose(Slot)
		.SizeParam(UWidget::ConvertSerializedSizeParamToRuntime(Size))
	[
		Content == nullptr ? SNullWidget::NullWidget : Content->TakeWidget()
	];
}

FMargin UScrollBoxSlot2D::GetPadding() const
{
	return Slot ? Slot->GetPadding() : Padding;
}

void UScrollBoxSlot2D::SetPadding(FMargin inPadding)
{
	Padding = inPadding;
	if (Slot)
	{
		Slot->SetPadding(inPadding);
	}
}

FSlateChildSize UScrollBoxSlot2D::GetSize() const
{
	return Size;
}

void UScrollBoxSlot2D::SetSize(FSlateChildSize inSize)
{
	Size = inSize;
	if (Slot)
	{
		Slot->SetSizeParam(UWidget::ConvertSerializedSizeParamToRuntime(inSize));
	}
}

EHorizontalAlignment UScrollBoxSlot2D::GetHorizontalAlignment() const
{
	return Slot ? Slot->GetHorizontalAlignment() : HorizontalAlignment.GetValue();
}

void UScrollBoxSlot2D::SetHorizontalAlignment(EHorizontalAlignment inHorizontalAlignment)
{
	HorizontalAlignment = inHorizontalAlignment;
	if (Slot)
	{
		Slot->SetHorizontalAlignment(inHorizontalAlignment);
	}
}

EVerticalAlignment UScrollBoxSlot2D::GetVerticalAlignment() const
{
	return Slot ? Slot->GetVerticalAlignment() : VerticalAlignment.GetValue();
}

void UScrollBoxSlot2D::SetVerticalAlignment(EVerticalAlignment inVerticalAlignment)
{
	VerticalAlignment = inVerticalAlignment;
	if (Slot)
	{
		Slot->SetVerticalAlignment(inVerticalAlignment);
	}
}

void UScrollBoxSlot2D::SynchronizeProperties()
{
	SetPadding(Padding);
	SetHorizontalAlignment(HorizontalAlignment);
	SetVerticalAlignment(VerticalAlignment);
	SetSize(Size);
}

void UScrollBoxSlot2D::ReleaseSlateResources(bool releaseChildren)
{
	Super::ReleaseSlateResources(releaseChildren);
	Slot = nullptr;
}
