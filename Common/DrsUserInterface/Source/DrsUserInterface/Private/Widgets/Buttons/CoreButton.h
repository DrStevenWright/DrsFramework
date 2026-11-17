// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Utils/WidgetUtils.h"
#include "CoreButton.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoreButtonEvent, UButton*, button);


/**
 * Extension of the native 'Button' class in order to provide additional information to the button events,
 * and also with more precise criteria for OInClick events.
 */
UCLASS()
class DRSUSERINTERFACE_API UCoreButton : public UButton
{
	GENERATED_BODY()

public:

	// Add support for calling nodes that require a world context to serived Blueprint classes.
	FORCEINLINE virtual UWorld* GetWorld() const override
	{
		return WidgetUtils::GetWorld(GetOuter());
	}
	
	float GetMaxMouseDeviation() const;
	static float GetMaxMouseDeviation(const UCoreButton* button);
	
protected:

	static constexpr float DefaultMaxMouseDeviation = 1.0f;
	
	// Enable checks on the deviation of the mouse position before allowing a button to be 'clicked'.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Button")
	bool bEnableMouseDeviationCheck = true;

	// The maximum amount that the mouse position is allowed to deviate between pressing and releasing a button in order
	// to trigger a click event.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Button", meta = (EditCondition = "bEnableMouseDeviationCheck"))
	float MaxMouseDeviation = DefaultMaxMouseDeviation;
	
	UPROPERTY(BlueprintAssignable, Category="Button|Event")
	FCoreButtonEvent OnClickedButton;
	
	UPROPERTY(BlueprintAssignable, Category="Button|Event")
	FCoreButtonEvent OnPressedButton;
	
	UPROPERTY(BlueprintAssignable, Category="Button|Event")
	FCoreButtonEvent OnReleasedButton;

	UPROPERTY( BlueprintAssignable, Category="Button|Event")
	FCoreButtonEvent OnHoveredButton;

	UPROPERTY( BlueprintAssignable, Category="Button|Event")
	FCoreButtonEvent OnUnhoveredButton;
	
	virtual void OnWidgetRebuilt() override;

private:
	
	FReply OverrideSlateHandleClicked() const;
	const class UWidgetInteractionComponent* FindSelectingInteractor() const;
	
	UFUNCTION()
	void ReceiveOnClicked()
	{
		OnClickedButton.Broadcast(this);
		bClickable = false;
		SelectingInteractor = nullptr;
	}

	UFUNCTION()
	void ReceiveOnPressed()
	{
		SelectingInteractor = FindSelectingInteractor();
		OnPressedButton.Broadcast(this);
		bClickable = true;
	}

	UFUNCTION()
	void ReceiveOnReleased()
	{
		OnReleasedButton.Broadcast(this);
	}

	UFUNCTION()
	void ReceiveOnHovered()
	{
		OnHoveredButton.Broadcast(this);
	}

	UFUNCTION()
	void ReceiveOnUnhovered()
	{
		OnUnhoveredButton.Broadcast(this);
		bClickable = false;
	}
	
	bool bClickable = false;
	
	UPROPERTY(Transient)
	const UWidgetInteractionComponent* SelectingInteractor = nullptr;
};
