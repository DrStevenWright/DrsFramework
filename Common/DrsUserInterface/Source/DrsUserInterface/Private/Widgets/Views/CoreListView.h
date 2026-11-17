// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "Components/ListView.h"
#include "Interfaces/DrsListViewInterface.h"
#include "Utils/WidgetUtils.h"
#include "CoreListView.generated.h"

class STableViewBase;

UENUM()
enum class EScrollbarPadDirection : uint8
{
	Left,
	Top,
	Right,
	Bottom
};


// Wrapper for internal SListView to access internals and expose additional functionality through UCoreListView.
template <typename ItemType>
class SCoreListView : public SListView<ItemType>
{
	friend class UCoreListView;

	void SetWidgetStyle(const FTableViewStyle* newStyle);
	void SetWidgetBarStyle(const FScrollBarStyle* newScrollBarStyle);

	/** Style resource for the scrollbar */
	const FScrollBarStyle* ScrollBarStyle = nullptr;
	
	union
	{
		// vertical scroll bar is stored in horizontal box and vice versa
		SHorizontalBox::FSlot* VerticalScrollBarSlot; // valid when Orientation == Orient_Vertical
		SVerticalBox::FSlot* HorizontalScrollBarSlot; // valid when Orientation == Orient_Horizontal
	};
};


/**
 * Extension of ListView widget that adds missing accessors needed by Core.
 * Also provides additional control over widget layout and scrollbar.
 */
UCLASS(BlueprintType, Blueprintable)
class DRSUSERINTERFACE_API UCoreListView : public UListView, public IDrsListViewInterface
{
	GENERATED_BODY()

	// Allows calling nodes that require a world context to in Blueprint classes.
	FORCEINLINE virtual UWorld* GetWorld() const override
	{
		return WidgetUtils::GetWorld(GetOuter());
	}
	
protected:

	//------------------------------------------
	// UListView Interface
	//------------------------------------------
	virtual TSharedRef<STableViewBase> RebuildListWidget() override;
	virtual void ReleaseSlateResources(bool releaseChildren) override;
	virtual void SynchronizeProperties() override;
	
	//------------------------------------------
	// IDrsListViewInterface
	//------------------------------------------
	virtual float GetWheelScrollMultiplier() const override { return WheelScrollMultiplier; }
	virtual bool IsDragScrollingEnabled() const override { return bEnableRightClickScrolling; }

	//------------------------------------------
	// Exposed API
	//------------------------------------------
	
	UFUNCTION(BlueprintCallable, Category = CoreListView)
	void SetWidgetStyle(const FTableViewStyle& newWidgetStyle);

	UFUNCTION(BlueprintCallable, Category = CoreListView)
	void SetWidgetBarStyle(const FScrollBarStyle& newScrollBarStyle);
	
	UFUNCTION(BlueprintCallable, Category = CoreListView)
	void SetScrollbarPaddingValue(EScrollbarPadDirection paddingDirection, float paddingValue); 

	UFUNCTION(BlueprintCallable, Category = CoreListView)
	void SetScrollbarThickness(const FVector2D& newScrollbarThickness);

	UFUNCTION(BlueprintCallable, Category = CoreListView)
	void SetAlwaysShowScrollbar(bool newAlwaysShowScrollbar);

	UFUNCTION(BlueprintCallable, Category = CoreListView)
	void SetAlwaysShowScrollbarTrack(bool newAlwaysShowScrollbarTrack);
	
	UFUNCTION(BlueprintCallable, Category = CoreListView)
	void SetOrientation(EOrientation newOrientation);

	UFUNCTION(BlueprintCallable, Category = CoreListView)
	void SetConsumeMouseWheel(EConsumeMouseWheel newConsumeMouseWheel);

	UFUNCTION(BlueprintCallable, Category = CoreListView)
	void SetClearSelectionOnClick(bool newClearSelectionOnClick);

	UFUNCTION(BlueprintCallable, Category = CoreListView)
	void SetIsFocusable(bool newIsFocusable);

	UFUNCTION(BlueprintCallable, Category = CoreListView, meta = (DisplayName = "Set Return Focus To Selection (Core List View)"))
	void SetReturnFocusToSelection_CoreListView(bool newReturnFocusToSelection);

	/** Sets the fixed offset in items to always apply to the top/left (depending on orientation) of the list. */
	UFUNCTION(BlueprintCallable, Category = CoreListView)
	void SetFixedLineScrollOffset(float newFixedLineScrollOffset);
	
	/** Sets whether the list should lerp between scroll offsets or jump instantly between them. */
	UFUNCTION(BlueprintCallable, Category = CoreListView)
	void SetIsScrollAnimationEnabled(bool newEnableScrollAnimation);

	UFUNCTION(BlueprintCallable, Category = CoreListView)
	bool IsAllowOverscroll() const { return AllowOverscroll; };
	
	/** Enables/disables being able to scroll with the right mouse button. */
	UFUNCTION(BlueprintCallable, Category = CoreListView)
	void SetIsRightClickScrollingEnabled(bool newEnableRightClickScrolling);

	UFUNCTION(BlueprintCallable, Category = CoreListView)
	bool IsRightClickScrollingEnabled() const { return bEnableRightClickScrolling; }

	UFUNCTION(BlueprintCallable, Category = CoreListView)
	void SetAllowDragging(bool newAllowDragging);

	/** Returns the "true" scroll offset where the list will ultimately settle (and may already be). */
	UFUNCTION(BlueprintCallable, Category = CoreListView)
	float GetTargetScrollOffset() const;
	
	/** Gets the distance that user has scrolled into the control in normalized coordinates (0 - 1) */
	UFUNCTION(BlueprintCallable, Category = CoreListView)
	float GetScrollDistance() const;
	
	/** Gets the distance that user has left to scroll in the control before reaching the end in normalized coordinates (0 - 1) */
	UFUNCTION(BlueprintCallable, Category = CoreListView)
	float GetScrollDistanceRemaining() const;

	/** Gets the scroll offset of the bottom of the list (in items). */
	UFUNCTION(BlueprintCallable, Category = CoreListView)
	float GetScrollOffsetOfEnd() const;

	/** Returns true if the user is currently interactively scrolling the view by holding either mouse button and dragging. */
	UFUNCTION(BlueprintCallable, Category = CoreListView)
	bool IsUserScrolling() const;
	
	//------------------------------------------
	// Properties
	//------------------------------------------
	
	/** The thickness of the scrollbar thumb */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = "SetScrollbarThickness", Category= "Scrolling")
	FVector2D ScrollbarThickness = FVector2D(16.0f, 16.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = "SetAlwaysShowScrollbar", Category = "Scrolling")
	bool bAlwaysShowScrollbar = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = "SetAlwaysShowScrollbarTrack", Category = "Scrolling")
	bool bAlwaysShowScrollbarTrack = false;
	
	TSharedPtr<SCoreListView<UObject*>> MyCoreListView;

private:

	// Additional synchronization of properties that are not handled by the base class.
	void SynchronizePropertiesInternal();
};

