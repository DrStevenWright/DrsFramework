// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#include "Widgets/Text/RunningTextBlock.h"

#include "Containers/Ticker.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"


URunningTextBlock::URunningTextBlock()
{
	RunningSpeed = 100.f;
	SetClipping(EWidgetClipping::ClipToBounds);
}


TSharedRef<SWidget> URunningTextBlock::RebuildWidget()
{
	StopScroll();
	
	MyTextBlock = SNew(STextBlock)
		.SimpleTextMode(bSimpleTextMode)
		.Visibility(EVisibility::SelfHitTestInvisible);

	TSharedPtr<SConstraintCanvas> HitBox = SNew(SConstraintCanvas)
		.Visibility(EVisibility::Visible)
		+ SConstraintCanvas::Slot()
		.AutoSize(true)
		.Anchors(FAnchors(0.f, 0.5f, 1.f, 0.5f)) // FillCenter
		.Alignment(FVector2D(0.f, 0.5f))
		[
			MyTextBlock.ToSharedRef()
		];

	MyScrollBox = SNew(SScrollBox)
		.Visibility(EVisibility::HitTestInvisible)
		.ScrollBarAlwaysVisible(false)
		.ScrollBarVisibility(EVisibility::Collapsed)
		.Orientation(EOrientation::Orient_Horizontal)
		.ConsumeMouseWheel(EConsumeMouseWheel::Never)
		.NavigationDestination(EDescendantScrollDestination::Center)
		.NavigationScrollPadding(0.f)
		+ SScrollBox::Slot()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Right)
		[
			HitBox.ToSharedRef()
		];

	if(bStartRunningTextOnMouseEnter)
	{
		HitBox->SetOnMouseEnter(BIND_UOBJECT_DELEGATE(FNoReplyPointerEventHandler, OnMouseEnter));
		HitBox->SetOnMouseLeave(BIND_UOBJECT_DELEGATE(FSimpleNoReplyPointerEventHandler, OnMouseExit));
	}

	return MyScrollBox.ToSharedRef();
}


void URunningTextBlock::ReleaseSlateResources(bool bReleaseChildren)
{
	StopScroll();
	MyScrollBox.Reset();
	
	Super::ReleaseSlateResources(bReleaseChildren);
}


void URunningTextBlock::StartRunningText()
{
	StopScroll();
	
	// make sure the TickHandle is only assigned once
	if (MyScrollBox && !TickHandle.IsValid())
	{
		TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([=, this](float deltaTime) -> bool
		{
			float newScrollOffset = 0.0f;
			const float delayDist = Delay * RunningSpeed;
			
			if(!bScrollBackwards)
			{
				newScrollOffset = FMath::Min(MyScrollBox->GetScrollOffsetOfEnd() + delayDist, MyScrollBox->GetScrollOffset() + deltaTime * RunningSpeed);
				const bool hasReachedEnd = FMath::IsNearlyEqual(newScrollOffset, MyScrollBox->GetScrollOffsetOfEnd() + delayDist);
				bScrollBackwards = bScrollBackAndForth ? hasReachedEnd : false;

				if(hasReachedEnd)
				{
					OnEndOfTextReached.Broadcast();
				}
			}
			if(bScrollBackAndForth && bScrollBackwards)
			{
				newScrollOffset = FMath::Max(0.0f - delayDist, MyScrollBox->GetScrollOffset() - deltaTime * RunningSpeed);
				const bool hasReachedEnd = FMath::IsNearlyEqual(newScrollOffset, 0.0f - delayDist);
				bScrollBackwards = !hasReachedEnd;

				if(hasReachedEnd)
				{
					OnEndOfTextReached.Broadcast();
				}
			}
			MyScrollBox->SetScrollOffset(newScrollOffset);

			// always return true when scrolling back and forth to keep forever going. 
			return bScrollBackAndForth ? true : !FMath::IsNearlyEqual(newScrollOffset, MyScrollBox->GetScrollOffsetOfEnd());
		}));
	}
}


void URunningTextBlock::StopRunningText(const bool resetText)
{
	StopScroll();
	if (MyScrollBox && resetText)
	{
		MyScrollBox->ScrollToStart();
		bScrollBackwards = false;
	}
}


void URunningTextBlock::OnMouseEnter(const FGeometry& geometry, const FPointerEvent& pointerEvent)
{
	StartRunningText();
}


void URunningTextBlock::OnMouseExit(const FPointerEvent& pointerEvent)
{
	StopRunningText(true);
}


void URunningTextBlock::StopScroll()
{
	if (TickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
		TickHandle.Reset();
	}
}
