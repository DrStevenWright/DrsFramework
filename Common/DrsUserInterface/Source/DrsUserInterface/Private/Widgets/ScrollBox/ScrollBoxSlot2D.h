// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "Components/PanelSlot.h"
#include "Components/SlateWrapperTypes.h"
#include "Layout/Margin.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"
#include "Widgets/ScrollBox/SScrollBox2D.h"
#include "ScrollBoxSlot2D.generated.h"

/** The Slot for the UScrollBox2D, contains the widget that are scrollable */
UCLASS()
class DRSUSERINTERFACE_API UScrollBoxSlot2D : public UPanelSlot
{
	GENERATED_UCLASS_BODY()

public:

	// Builds the underlying FSlot for the Slate layout panel. 
	void BuildSlot(TSharedRef<SScrollBox2D> scrollBox);
	
	void SetSize(FSlateChildSize inSize);
	FSlateChildSize GetSize() const;
	
	UFUNCTION(BlueprintCallable, Category="Layout|ScrollBox Slot")
	void SetPadding(FMargin inPadding);
	
	UFUNCTION(BlueprintGetter, Category="Layout|ScrollBox Slot")
	FMargin GetPadding() const;

	UFUNCTION(BlueprintCallable, Category="Layout|ScrollBox Slot")
	void SetHorizontalAlignment(EHorizontalAlignment inHorizontalAlignment);

	UFUNCTION(BlueprintGetter, Category="Layout|ScrollBox Slot")
	EHorizontalAlignment GetHorizontalAlignment() const;
	
	UFUNCTION(BlueprintCallable, Category = "Layout|ScrollBox Slot")
	void SetVerticalAlignment(EVerticalAlignment inVerticalAlignment);

	UFUNCTION(BlueprintGetter, Category="Layout|ScrollBox Slot")
	EVerticalAlignment GetVerticalAlignment() const;

protected:

	// UPanelSlot interface
	virtual void SynchronizeProperties() override;
	// End of UPanelSlot interface

	// UWidget interface
	virtual void ReleaseSlateResources(bool releaseChildren) override;
	// End of UWidget interface
	
	/** The padding area between the slot and the content it contains. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, BlueprintGetter=GetPadding, Category="Layout|ScrollBox Slot")
	FMargin Padding;
	
	/** The alignment of the object horizontally. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, BlueprintGetter=GetHorizontalAlignment, Category="Layout|ScrollBox Slot")
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;
	
	/** The alignment of the object vertically. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, BlueprintGetter=GetVerticalAlignment, Category = "Layout|ScrollBox Slot")
	TEnumAsByte<EVerticalAlignment> VerticalAlignment;

private:

	/** How much space this slot should occupy in the direction of the panel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter, Setter, Category = "Layout|ScrollBox Slot", meta = (AllowPrivateAccess = "true", DisplayAfter = "Padding"))
	FSlateChildSize Size;

	/** A raw pointer to the slot to allow us to adjust the size, padding...etc at runtime. */
	SScrollBox2D::FSlot* Slot = nullptr;
};
