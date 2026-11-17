// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#include "Widgets/Buttons/CoreButton.h"

#include "CoreUserInterfaceSettings.h"
#include "Components/WidgetInteractionComponent.h"
#include "Components/CoreWidgetInteractionComponent.h"
#include "Interfaces/CoreWidgetInteractionInterface.h"
#include "UserInterfaceSubsystem.h"


float UCoreButton::GetMaxMouseDeviation() const
{
	return GetMaxMouseDeviation(this);
}


float UCoreButton::GetMaxMouseDeviation(const UCoreButton* button)
{
	return (button ? button->MaxMouseDeviation : DefaultMaxMouseDeviation) * UCoreUserInterfaceSettings::Get().MaxMouseDeviationMultiplier;
}


void UCoreButton::OnWidgetRebuilt()
{
	Super::OnWidgetRebuilt();
		
	OnClicked.AddUniqueDynamic(this, &UCoreButton::ReceiveOnClicked);
	OnPressed.AddUniqueDynamic(this, &UCoreButton::ReceiveOnPressed);
	OnReleased.AddUniqueDynamic(this, &UCoreButton::ReceiveOnReleased);
	OnHovered.AddUniqueDynamic(this, &UCoreButton::ReceiveOnHovered);
	OnUnhovered.AddUniqueDynamic(this, &UCoreButton::ReceiveOnUnhovered);
	MyButton->SetOnClicked(BIND_UOBJECT_DELEGATE(FOnClicked, OverrideSlateHandleClicked));
}


FReply UCoreButton::OverrideSlateHandleClicked() const
{
	bool allowClick = true;
	
	if (!bClickable)
	{
		// Don't allow click events if the button is not in an appropriate state.
		allowClick = false;
	}
	else if (const ICoreWidgetInteractionInterface* widgetInteractionInterface = SelectingInteractor ? Cast<ICoreWidgetInteractionInterface>(SelectingInteractor) : Cast<ICoreWidgetInteractionInterface>(FindSelectingInteractor()))
	{
		switch (widgetInteractionInterface->AllowButtonClickEvents())
		{
		case EAllowClickEvents::No:
			// Don't allow click events if the interactor is not in an appropriate state.
			allowClick = false;
			break;

		case EAllowClickEvents::Yes:
			allowClick = true;
			break;

		case EAllowClickEvents::WithMouseDeviationCheck:
			// Don't allow click events if the interactor has moved too far between press and release.
			allowClick = !(bEnableMouseDeviationCheck && widgetInteractionInterface->GetMouseDeviation() > GetMaxMouseDeviation());
			break;
		};
	}
	
	if (allowClick)
	{
		OnClicked.Broadcast();
	}
	
	return FReply::Handled();
}


const UWidgetInteractionComponent* UCoreButton::FindSelectingInteractor() const
{
	if (const UUserInterfaceSubsystem* subsystem = UUserInterfaceSubsystem::Get(this))
	{
		for (const UWidgetInteractionComponent* widgetInteraction : subsystem->GetRegisteredWidgetInteractors())
		{
			if (const ICoreWidgetInteractionInterface* widgetInteractionInterface = widgetInteraction ? Cast<ICoreWidgetInteractionInterface>(widgetInteraction) : nullptr)
			{
				if (widgetInteractionInterface->IsSelecting(this))
				{
					return widgetInteraction;
				}
			}
		}
	}
	return nullptr;
}
