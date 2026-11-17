// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#include "Widgets/Buttons/TouchButton.h"

#include "Components/ButtonSlot.h"
#include "Styling/UMGCoreStyle.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SNullWidget.h"

static FButtonStyle* DefaultButtonStyle = nullptr;

UTouchButton::UTouchButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Default style same as UButton class.
	if (DefaultButtonStyle == nullptr)
	{
		DefaultButtonStyle = new FButtonStyle(FUMGCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button"));

		// Unlink UMG default colors.
		DefaultButtonStyle->UnlinkColors();
	}

	WidgetStyle = *DefaultButtonStyle;
	WidgetStyle.Normal.TintColor = FLinearColor::White;
	
	ColorAndOpacity = FLinearColor::White;
	BackgroundColor = FLinearColor::White;

	ClickMethod = EButtonClickMethod::DownAndUp;
	TouchMethod = EButtonTouchMethod::DownAndUp;

	IsFocusable = true;
}

void UTouchButton::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MyButton.Reset();
}

TSharedRef<SWidget> UTouchButton::RebuildWidget()
{
	MyButton = SNew(SButton)
		.OnClicked(BIND_UOBJECT_DELEGATE(FOnClicked, SlateHandleClicked))
		.OnPressed(BIND_UOBJECT_DELEGATE(FSimpleDelegate, SlateHandlePressed))
		.OnReleased(BIND_UOBJECT_DELEGATE(FSimpleDelegate, SlateHandleReleased))
		.OnHovered_UObject(this, &ThisClass::SlateHandleHovered)
		.OnUnhovered_UObject(this, &ThisClass::SlateHandleUnhovered)
		.ButtonStyle(&WidgetStyle)
		.ClickMethod(ClickMethod)
		.TouchMethod(TouchMethod)
		.IsFocusable(IsFocusable)
		;

	if (GetChildrenCount() > 0)
	{
		Cast<UButtonSlot>(GetContentSlot())->BuildSlot(MyButton.ToSharedRef());
	}

	return MyButton.ToSharedRef();
}

void UTouchButton::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	MyButton->SetColorAndOpacity(ColorAndOpacity);
	MyButton->SetBorderBackgroundColor(BackgroundColor);
}

UClass* UTouchButton::GetSlotClass() const
{
	return UButtonSlot::StaticClass();
}

void UTouchButton::OnSlotAdded(UPanelSlot* slot)
{
	// Add the child to the live slot if it already exists
	if (MyButton.IsValid())
	{
		CastChecked<UButtonSlot>(slot)->BuildSlot(MyButton.ToSharedRef());
	}
}

void UTouchButton::OnSlotRemoved(UPanelSlot* slot)
{
	// Remove the widget from the live slot if it exists.
	if (MyButton.IsValid())
	{
		MyButton->SetContent(SNullWidget::NullWidget);
	}
}

void UTouchButton::SetStyle(const FButtonStyle& style)
{
	WidgetStyle = style;
	if (MyButton.IsValid())
	{
		MyButton->SetButtonStyle(&WidgetStyle);
	}
}

void UTouchButton::SetColorAndOpacity(const FLinearColor& colorAndOpacity)
{
	ColorAndOpacity = colorAndOpacity;
	if (MyButton.IsValid())
	{
		MyButton->SetColorAndOpacity(colorAndOpacity);
	}
}

void UTouchButton::SetBackgroundColor(const FLinearColor& backgroundColor)
{
	BackgroundColor = backgroundColor;
	if (MyButton.IsValid())
	{
		MyButton->SetBorderBackgroundColor(backgroundColor);
	}
}

bool UTouchButton::IsPressed() const
{
	if (MyButton.IsValid())
	{
		return MyButton->IsPressed();
	}

	return false;
}

void UTouchButton::SetClickMethod(EButtonClickMethod::Type clickMethod)
{
	ClickMethod = clickMethod;
	if (MyButton.IsValid())
	{
		MyButton->SetClickMethod(ClickMethod);
	}
}

void UTouchButton::SetTouchMethod(EButtonTouchMethod::Type touchMethod)
{
	TouchMethod = touchMethod;
	if (MyButton.IsValid())
	{
		MyButton->SetTouchMethod(TouchMethod);
	}
}

void UTouchButton::SetPressMethod(EButtonPressMethod::Type pressMethod)
{
	PressMethod = pressMethod;
	if (MyButton.IsValid())
	{
		MyButton->SetPressMethod(PressMethod);
	}
}

void UTouchButton::PostLoad()
{
	Super::PostLoad();

	if (GetChildrenCount() > 0)
	{
		//TODO UMG Pre-Release Upgrade, now buttons have slots of their own.  Convert existing slot to new slot.
		if (UPanelSlot* panelSlot = GetContentSlot())
		{
			UButtonSlot* buttonSlot = Cast<UButtonSlot>(panelSlot);
			if (buttonSlot == nullptr)
			{
				buttonSlot = NewObject<UButtonSlot>(this);
				buttonSlot->Content = GetContentSlot()->Content;
				buttonSlot->Content->Slot = buttonSlot;
				Slots[0] = buttonSlot;
			}
		}
	}
}

FReply UTouchButton::SlateHandleClicked()
{
	OnClicked.Broadcast();

	return FReply::Handled();
}

void UTouchButton::SlateHandlePressed()
{
	OnPressed.Broadcast();
	bPressed = true;
}

void UTouchButton::SlateHandleReleased()
{
	if (bPressed && bHovered)
	{
		OnReleased.Broadcast();
	}

	bPressed = false;
}

void UTouchButton::SlateHandleHovered()
{
	OnHovered.Broadcast();
	bHovered = true;
}

void UTouchButton::SlateHandleUnhovered()
{
	OnUnhovered.Broadcast();
	bHovered = false;
}

#if WITH_EDITOR

const FText UTouchButton::GetPaletteCategory()
{
	return INVTEXT("Common");
}

#endif
