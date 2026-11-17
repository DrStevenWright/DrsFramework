// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "Widgets/Text/CoreTextBlock.h"
#include "RunningTextBlock.generated.h"

class SScrollBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEndOfTextReachedDelegate);


/**
 * Text block that auto-scrolls on hover.
 */
UCLASS()
class DRSUSERINTERFACE_API URunningTextBlock : public UCoreTextBlock
{
	GENERATED_BODY()

public:
	
	URunningTextBlock();

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	// Start the running text 
	UFUNCTION(BlueprintCallable, Category = "Widget")
	void StartRunningText();

	// Stop the running text
	// Set resetText = true, if the text should be resetted to its beginning
	UFUNCTION(BlueprintCallable, Category = "Widget")
	void StopRunningText(const bool resetText);

protected:
	
	TSharedPtr<SScrollBox> MyScrollBox;
	FTSTicker::FDelegateHandle TickHandle;

	UPROPERTY(BlueprintAssignable)
	FEndOfTextReachedDelegate OnEndOfTextReached;

	// The speed with which the text will move
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Widget", meta=(ClampMin="0.1"))
	float RunningSpeed;

	// Defines if the text should scroll back after it reaches the end with a defined "Delay"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Widget")
	bool bScrollBackAndForth = false;

	// Delay for the scrolling to wait at the end before scrolling in other direction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Widget", meta=(ClampMin="0.0", EditCondition="bScrollBackAndForth"))
	float Delay = 0.0f;

	// If the text should automatically be running on mouse enter of the textbox or only with a custom event
	// that calls "StartRunningText" and "StopRunningText" directly
	// Note: Calling event and enable OnMouseEnter could cause unwanted results, one or the other should be chosen!
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Widget")
	bool bStartRunningTextOnMouseEnter = true;

	bool bScrollBackwards = false;

	void OnMouseEnter(const FGeometry& geometry, const FPointerEvent& pointerEvent);
	void OnMouseExit(const FPointerEvent& pointerEvent);

	void StopScroll();
};
