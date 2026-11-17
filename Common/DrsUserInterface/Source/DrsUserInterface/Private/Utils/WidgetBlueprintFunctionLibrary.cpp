// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "Utils/WidgetBlueprintFunctionLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "Input/Events.h"
#include "Utils/WidgetUtils.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SWidget.h"

void UWidgetBlueprintFunctionLibrary::ButtonSimulateMouseEnter(const UButton* button)
{
	if (IsValid(button) && !button->IsHovered())
	{
		if (SButton* slate = GetButtonSlate(button))
		{
			const FVector2D center = slate->GetCachedGeometry().GetRenderBoundingRect().GetCenter();
			slate->OnMouseEnter(button->GetCachedGeometry(), FPointerEvent(0, 0, center, FVector2D::ZeroVector, 0.f, false));
		}
	}
}

void UWidgetBlueprintFunctionLibrary::ButtonSimulateMouseExit(const UButton* button)
{
	if (IsValid(button) && button->IsHovered())
	{
		if (SButton* slate = GetButtonSlate(button))
		{
			const FVector2D center = slate->GetCachedGeometry().GetRenderBoundingRect().GetCenter();
			slate->OnMouseLeave(FPointerEvent(0, 0, FVector2D::ZeroVector, center, 0.f, false));
		}
	}
}

void UWidgetBlueprintFunctionLibrary::ButtonSimulateMouseDown(const UButton* button)
{
	if (IsValid(button) && !button->IsPressed())
	{
		if (SButton* slate = GetButtonSlate(button))
		{
			const FVector2D center = slate->GetCachedGeometry().GetRenderBoundingRect().GetCenter();
			slate->OnMouseButtonDown(button->GetCachedGeometry(), FPointerEvent(0, 0, center, center, 1.f, true));
		}
	}
}

void UWidgetBlueprintFunctionLibrary::ButtonSimulateMouseUp(const UButton* button)
{
	if (IsValid(button) && button->IsPressed())
	{
		if (SButton* slate = GetButtonSlate(button))
		{
			const FVector2D center = slate->GetCachedGeometry().GetRenderBoundingRect().GetCenter();
			slate->OnMouseButtonUp(button->GetCachedGeometry(), FPointerEvent(0, 0, center, center, 1.f, true));
		}
	}
}

FText UWidgetBlueprintFunctionLibrary::AsFileSize(float bytes)
{
	static constexpr FString sizes[] = { "B", "KB", "MB", "GB", "TB" };
	static constexpr int32 sizesLen = std::size(sizes);

	size_t order = 0;
	while (bytes >= 1000.f && order < sizesLen - 1)
	{
		order++;
		bytes /= 1000.f;
	}

	// show a single decimal place, and no space.
	return FText::FromString(FString::Printf(TEXT("%.1f %s"), bytes, *(sizes[order])));
}

void UWidgetBlueprintFunctionLibrary::SetFontSize(UTextBlock* text, const int32 fontSize)
{
	if (!IsValid(text))
	{
		return;
	}

	FSlateFontInfo font = text->GetFont();
	font.Size = fontSize;
	text->SetFont(font);
}

void UWidgetBlueprintFunctionLibrary::SetWidgetClass(UWidgetComponent* widgetComponent, TSubclassOf<UUserWidget> widgetClass)
{
	if (IsValid(widgetComponent))
	{
		widgetComponent->SetWidgetClass(widgetClass);
	}
}

TMap<FString, UButton*> UWidgetBlueprintFunctionLibrary::CacheAndSetVisibilityForWidgetButtons(UUserWidget* widget, ESlateVisibility visibility)
{
	// Map all buttons found in the widget based on their names
	TMap<FString, UButton*> buttonMap;

	TArray<UWidget*> childWidgets;
	widget->WidgetTree->GetChildWidgets(widget->GetRootWidget(), childWidgets);

	for (UWidget* childWidget : childWidgets)
	{
		// Recursion to check children of widgets inside of widgets
		if (UUserWidget* castCheck = Cast<UUserWidget>(childWidget))
		{
			TMap<FString, UButton*> recursiveButtonMap = CacheAndSetVisibilityForWidgetButtons(castCheck, visibility);

			for (auto entry : recursiveButtonMap)
			{
				buttonMap.Add(entry);
			}
		}

		if (UButton* button = Cast<UButton>(childWidget))
		{
			FString buttonName = button->GetName();
			buttonMap.Add(buttonName, button);
			button->OnReleased.RemoveAll(childWidget);
			button->SetVisibility(visibility);
		}
	}

	return buttonMap;
}

void UWidgetBlueprintFunctionLibrary::SetVisibilityForWidgetButtons(UUserWidget* widget, ESlateVisibility visibility)
{
	TArray<UWidget*> childWidgets;
	widget->WidgetTree->GetChildWidgets(widget->GetRootWidget(), childWidgets);

	for (UWidget* childWidget : childWidgets)
	{
		// Recursion to check children of widgets inside of widgets
		if (UUserWidget* castCheck = Cast<UUserWidget>(childWidget))
		{
			SetVisibilityForWidgetButtons(castCheck, visibility);
		}

		if (UButton* button = Cast<UButton>(childWidget))
		{
			button->OnReleased.RemoveAll(childWidget);
			button->SetVisibility(visibility);
		}
	}
}

void UWidgetBlueprintFunctionLibrary::FindChildWidgetsByClass(UWidget* widget, TSubclassOf<UWidget> widgetClass,
	TArray<UWidget*>& outWidgets, bool bRecursive)
{
	FindChildWidgetsByPredicate(widget, outWidgets, [widgetClass, bRecursive](const UWidget* thisWidget, bool& bAddToList, bool& bSearchChildren)
	{
		bAddToList = thisWidget->IsA(widgetClass);
		bSearchChildren = bRecursive;
	});
}

void UWidgetBlueprintFunctionLibrary::FindChildWidgetsByPredicate(UWidget* widget,  TArray<UWidget*>& outWidgets,
	const FWidgetSearchPredicateSignatue& predicate)
{
	FindChildWidgetsByPredicate(widget, outWidgets, [predicate](UWidget* thisWidget, bool& bAddToList, bool& bSearchChildren)
	{
		predicate.ExecuteIfBound(thisWidget, bAddToList, bSearchChildren);
	});
}

void UWidgetBlueprintFunctionLibrary::FindChildWidgetsByPredicate(UWidget* widget, TArray<UWidget*>& outWidgets,
	const TFunction<void(UWidget*, bool&, bool&)>& predicate)
{
	if (!IsValid(widget))
	{
		return;
	}
	
	TArray<UWidget*> children;
	if (const UUserWidget* userWidget = Cast<UUserWidget>(widget))
	{
		if (UWidget* root = userWidget->WidgetTree ? userWidget->WidgetTree->RootWidget : nullptr)
		{
			check(root != widget);
			children.Add(root);
		}
	}
	else
	{
		UWidgetTree::GetChildWidgets(widget, children);
	}
	
	for (UWidget* child : children)
	{
		if (IsValid(child))
		{
			bool bAddToList = false;
			bool bSearchChildren = false;
			predicate(child, bAddToList, bSearchChildren);
		
			if (bAddToList)
			{
				outWidgets.Add(child);
			}
			if (bSearchChildren)
			{
				FindChildWidgetsByPredicate(child, outWidgets, predicate);
			}
		}
	}
}

SButton* UWidgetBlueprintFunctionLibrary::GetButtonSlate(const UButton* button)
{
	if (IsValid(button))
	{
		const TSharedPtr<SWidget> slatePointer = button->GetCachedWidget();
		if (slatePointer.IsValid())
		{
			return static_cast<SButton*>(slatePointer.Get());
		}
	}
	return nullptr;
}

UDrsWidgetInteractionComponent* UWidgetBlueprintFunctionLibrary::FindFocusedWidgetInteractionFromWidget(const UObject* worldContextObject, const UWidget* widget)
{
	return WidgetUtils::FindFocusedWidgetInteractionFromWidget(worldContextObject, widget);
}
