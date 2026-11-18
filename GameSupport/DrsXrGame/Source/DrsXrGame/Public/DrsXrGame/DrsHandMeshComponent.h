// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsBase/DrsHand.h"

#include "Components/SkeletalMeshComponent.h"
#include "DrsHandMeshComponent.generated.h"

/**
 * Visual for a tracked hand, tinted per hand through the material's "Color" vector parameter.
 * By default it shows a palm-sized box (the engine's SkeletalCube, scaled) with M_DrsHandPlaceholder; set another
 * skeletal mesh and material for real hands. The material needs a "Color" parameter for the tint.
 * Attach it to a UMotionControllerComponent. It hides itself while its parent controller is not tracked.
 * Local-only: holds no replicated state.
 */
UCLASS(ClassGroup = (DRS), meta = (BlueprintSpawnableComponent))
class DRSXRGAME_API UDrsHandMeshComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	UDrsHandMeshComponent();

	/** Sets the hand and updates the tint. */
	UFUNCTION(BlueprintCallable, Category = "DRS|Hand")
	void SetHand(EDrsHand hand);

	UFUNCTION(BlueprintPure, Category = "DRS|Hand")
	EDrsHand GetHand() const { return Hand; }

	/** Default tint for a hand. */
	static FLinearColor GetDefaultHandColour(EDrsHand hand);

	virtual void BeginPlay() override;
	virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DRS|Hand")
	EDrsHand Hand = EDrsHand::Left;

	/** Hide the mesh while the parent motion controller has no tracking. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DRS|Hand")
	bool bHideWhenUntracked = true;

private:
	void ApplyColour();
};
