// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "DrsVotableState.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DrsXrPassthroughSubsystem.generated.h"

DRSXRMIXEDREALITY_API DECLARE_LOG_CATEGORY_EXTERN(LogDrsXrMixedReality, Log, All);

/**
 * Turns passthrough on and off, on every vendor.
 *
 * While passthrough is active, the real world shows wherever the scene's alpha is see-through: the background of a level
 * without a sky, or windows with an AlphaHoldout material. Content decides where and when; this subsystem only switches
 * it. Passthrough is suspended while a view fade is fully opaque (e.g. during a level transition).
 * Local-only: holds no replicated state.
 */
UCLASS()
class DRSXRMIXEDREALITY_API UDrsXrPassthroughSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UDrsXrPassthroughSubsystem* Get(const UObject* worldContext, EGetWorldErrorMode errorMode = EGetWorldErrorMode::ReturnNull);

	virtual void Initialize(FSubsystemCollectionBase& collection) override;
	virtual void Deinitialize() override;

	/** Whether the headset supports passthrough. False until the XR session exists. */
	UFUNCTION(BlueprintPure, Category = "Drs|XR|Passthrough")
	bool IsPassthroughSupported() const;

	/**
	 * Requests passthrough. The request is scoped to the world context's world and released when that world is torn down
	 * (a null world context makes it persistent).
	 * @param requestId Unique ID of this request; release it with the same ID.
	 */
	UFUNCTION(BlueprintCallable, Category = "Drs|XR|Passthrough", meta = (WorldContext = "worldContext"))
	void RequestPassthrough(const UObject* worldContext, FName requestId);

	/** Releases a passthrough request. */
	UFUNCTION(BlueprintCallable, Category = "Drs|XR|Passthrough")
	void ReleasePassthrough(FName requestId);

	/**
	 * Suspends passthrough until every suspension is withdrawn.
	 * @param reason The reason, which is also the ID to unsuspend with.
	 */
	UFUNCTION(BlueprintCallable, Category = "Drs|XR|Passthrough")
	void SuspendPassthrough(FName reason);

	/** Withdraws a suspension. */
	UFUNCTION(BlueprintCallable, Category = "Drs|XR|Passthrough")
	void UnsuspendPassthrough(FName reason);

	/**
	 * Blends passthrough in gradually (e.g. as the headset leaves a safe area). Without passthrough this fades to black.
	 * @param amount 0 = no passthrough, 1 = passthrough only.
	 */
	UFUNCTION(BlueprintCallable, Category = "Drs|XR|Passthrough", meta = (WorldContext = "worldContext"))
	void SetPassthroughBlend(const UObject* worldContext, float amount);

	UFUNCTION(BlueprintPure, Category = "Drs|XR|Passthrough")
	float GetPassthroughBlend() const;

	UFUNCTION(BlueprintPure, Category = "Drs|XR|Passthrough")
	bool IsPassthroughRequested() const { return !Requests.IsEmpty(); }

	UFUNCTION(BlueprintPure, Category = "Drs|XR|Passthrough")
	bool IsPassthroughSuspended() const { return Suspensions.IsVoted(); }

	/** Whether passthrough is visible now. */
	UFUNCTION(BlueprintPure, Category = "Drs|XR|Passthrough")
	bool IsPassthroughActive() const { return IsPassthroughRequested() && !IsPassthroughSuspended() && IsPassthroughSupported(); }

private:
	void UpdatePassthroughState();
	void OnWorldCleanup(UWorld* world, bool hasSessionEnded, bool shouldCleanupResources);
	void OnViewFadeOpaqueChanged(bool isOpaque);

	/** Passthrough requests by ID, with the world each is scoped to (null = persistent). */
	TMap<FName, TWeakObjectPtr<const UWorld>> Requests;
	TDrsVotableState<FName> Suspensions;
};
