// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "DrsHandPlaceholderComponent.generated.h"

/** Which of the local user's hands a component represents. */
UENUM(BlueprintType)
enum class EDrsHand : uint8
{
	Left,
	Right
};

/**
 * Placeholder visual for a tracked hand: a palm-sized box, tinted per hand.
 * Attach it to a UMotionControllerComponent. It hides itself while its parent controller is not tracked.
 * Local-only: holds no replicated state.
 */
UCLASS(ClassGroup = "Drs", meta = (BlueprintSpawnableComponent))
class DRSXRGAME_API UDrsHandPlaceholderComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UDrsHandPlaceholderComponent();

	/** Sets the hand and updates the tint. */
	UFUNCTION(BlueprintCallable, Category = "Drs|Hand")
	void SetHand(EDrsHand hand);

	UFUNCTION(BlueprintPure, Category = "Drs|Hand")
	EDrsHand GetHand() const { return Hand; }

	/** Default tint for a hand. */
	static FLinearColor GetDefaultHandColor(EDrsHand hand);

	virtual void BeginPlay() override;
	virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drs|Hand")
	EDrsHand Hand = EDrsHand::Left;

	/** Hide the placeholder while the parent motion controller has no tracking. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drs|Hand")
	bool bHideWhenUntracked = true;

private:
	void ApplyColor();
};
