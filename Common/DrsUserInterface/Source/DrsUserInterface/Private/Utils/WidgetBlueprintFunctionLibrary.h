// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WidgetBlueprintFunctionLibrary.generated.h"

class SButton;
class UButton;
class UTextBlock;
class UWidgetComponent;
class UDrsWidgetInteractionComponent;

DECLARE_DYNAMIC_DELEGATE_ThreeParams(FWidgetSearchPredicateSignatue, UWidget*, widget, bool&, bAddToList, bool&, bSearchChildren);

UCLASS()
class DRSUSERINTERFACE_API UWidgetBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static void ButtonSimulateMouseEnter(const UButton* button);

	UFUNCTION(BlueprintCallable)
	static void ButtonSimulateMouseExit(const UButton* button);

	UFUNCTION(BlueprintCallable)
	static void ButtonSimulateMouseDown(const UButton* button);

	UFUNCTION(BlueprintCallable)
	static void ButtonSimulateMouseUp(const UButton* button);

	// Convert a size in bytes to appropriate human-readable text
	UFUNCTION(BlueprintPure)
	static FText AsFileSize(float bytes);

	UFUNCTION(BlueprintCallable)
	static void SetFontSize(UTextBlock* text, const int32 fontSize);

	UFUNCTION(BlueprintCallable)
	static void SetWidgetClass(UWidgetComponent* widgetComponent, TSubclassOf<UUserWidget> widgetClass);

	/** Please note this function also unbinds any events in buttons found from OnReleased */
	UFUNCTION(BlueprintCallable)
	static TMap<FString, UButton*> CacheAndSetVisibilityForWidgetButtons(UUserWidget* widget, ESlateVisibility visibility);

	/** Please note this function also unbinds any events in buttons found from OnReleased */
	UFUNCTION(BlueprintCallable)
	static void SetVisibilityForWidgetButtons(UUserWidget* widget, ESlateVisibility visibility);

	UFUNCTION(BlueprintCallable, meta = (DeterminesOutputType = "widgetClass", DynamicOutputParam = "outWidgets"))
	static void FindChildWidgetsByClass(UWidget* widget, TSubclassOf<UWidget> widgetClass, TArray<UWidget*>& outWidgets, bool bRecursive=true);
	
	UFUNCTION(BlueprintCallable)
	static void FindChildWidgetsByPredicate(UWidget* widget, TArray<UWidget*>& outWidgets, const FWidgetSearchPredicateSignatue& predicate);
	
	// Get all child widgets by predicate, define for each child if it should be added to the result outWidgets
	// and if we should recursively parse its children.
	static void FindChildWidgetsByPredicate(UWidget* widget, TArray<UWidget*>& outWidgets, const TFunction<void(UWidget*, bool&, bool&)>& predicate);

	/**
	 * Allows checking which WidgetInteractionComponent was used for focusing (selecting) provided widget.
	 * E.g. Which WidgetInteractionComponent should be used for updating EditableTextBox. 
	 * @param widget Currently focused widget.
	 * @return Returns player's widget interaction component used to focus provided widget.
	 */
	UFUNCTION(BlueprintCallable, Category = "Interactions", meta = (WorldContext = "worldContextObject"))
	static UDrsWidgetInteractionComponent* FindFocusedWidgetInteractionFromWidget(const UObject* worldContextObject, const UWidget* widget);
	
protected:
	
	static SButton* GetButtonSlate(const UButton* button);
};
