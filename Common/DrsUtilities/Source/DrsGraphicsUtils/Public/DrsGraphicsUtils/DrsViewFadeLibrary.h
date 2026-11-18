// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Runtime/Engine/Classes/Kismet/BlueprintFunctionLibrary.h"
#include "DrsViewFadeLibrary.generated.h"

class UCurveFloat;
class UCurveLinearColor;
struct FRichCurve;

DECLARE_DYNAMIC_DELEGATE_TwoParams(FDrsViewFadeUpdateSignature, float, progress, const FLinearColor&, fadeColour);
DECLARE_DYNAMIC_DELEGATE_OneParam(FDrsViewFadeFinishedSignature, const FLinearColor&, fadeColour);

/**
 * Full-view fades, drawn by a scene view extension after the scene and before the loading screen. Unlike the camera
 * manager fade, this works on Android as well as Windows, and in stereo.
 *
 * It also draws a holdout, beneath the fade, that makes the view see-through so that passthrough (if active) shows
 * instead of the scene. A full fade always hides the holdout.
 */
UCLASS()
class DRSGRAPHICSUTILS_API UDrsViewFadeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UDrsViewFadeLibrary();

	using FViewFadeFinishedDelegate = TDelegate<void(const FLinearColor&)>;
	using FViewFadeUpdateDelegate = TDelegate<void(float, const FLinearColor&)>;
	using FViewFadeOpaqueDelegate = TMulticastDelegate<void(bool isOpaque)>;

	/** Sets the fade colour and opacity at once, stopping any fade in progress. */
	UFUNCTION(BlueprintCallable, Category = "DRS|View Fade", meta = (WorldContext = "worldContext", AutoCreateRefTerm = "colour"))
	static void SetViewFadeColour(const UObject* worldContext, const FLinearColor& colour = FLinearColor::Black, float opacity = 0.0f, bool fadeAudio = false);

	/** Sets the fade opacity at once, stopping any fade in progress. */
	UFUNCTION(BlueprintCallable, Category = "DRS|View Fade", meta = (WorldContext = "worldContext"))
	static void SetViewFadeOpacity(const UObject* worldContext, float opacity = 0.0f, bool fadeAudio = false);

	/** Fades along a colour curve (its alpha is the opacity). */
	static void StartViewFade(const UObject* worldContext, const UCurveLinearColor* fadeCurve, bool fadeAudio = false,
		const FViewFadeFinishedDelegate& onFadeFinished = FViewFadeFinishedDelegate(), const FViewFadeUpdateDelegate& onFadeUpdate = FViewFadeUpdateDelegate());

	/** Fades linearly to a colour (its alpha is the target opacity), from the current fade unless resetCurrentFade is set. */
	static void StartViewFade(const UObject* worldContext, const FLinearColor& targetColour, float duration = 0.4f, bool resetCurrentFade = false, bool fadeAudio = false,
		const FViewFadeFinishedDelegate& onFadeFinished = FViewFadeFinishedDelegate(), const FViewFadeUpdateDelegate& onFadeUpdate = FViewFadeUpdateDelegate());

	/** Fades to a colour with the opacity following a curve. */
	static void StartViewFade(const UObject* worldContext, const FLinearColor& targetColour, const UCurveFloat* alphaCurve, bool resetCurrentFadeColour = false, bool fadeAudio = false,
		const FViewFadeFinishedDelegate& onFadeFinished = FViewFadeFinishedDelegate(), const FViewFadeUpdateDelegate& onFadeUpdate = FViewFadeUpdateDelegate());

	/** Fades along one curve per channel (alpha is the opacity). */
	static void StartViewFade(const UObject* worldContext, const FRichCurve& rCurve, const FRichCurve& gCurve, const FRichCurve& bCurve, const FRichCurve& aCurve, bool fadeAudio = false,
		const FViewFadeFinishedDelegate& onFadeFinished = FViewFadeFinishedDelegate(), const FViewFadeUpdateDelegate& onFadeUpdate = FViewFadeUpdateDelegate());

	/** Stops the fade in progress, clearing the fade unless keepFadeColour is set. */
	UFUNCTION(BlueprintCallable, Category = "DRS|View Fade")
	static void StopViewFade(bool keepFadeColour = false);

	UFUNCTION(BlueprintPure, Category = "DRS|View Fade")
	static float GetViewFadeOpacity();

	UFUNCTION(BlueprintPure, Category = "DRS|View Fade")
	static void GetViewFadeColour(FLinearColor& outColour);

	/** Whether the view is faded at all (any opacity is treated as a passing state). */
	UFUNCTION(BlueprintPure, Category = "DRS|View Fade")
	static bool IsFading();

	/** Whether a fade towards a colour is in progress. */
	UFUNCTION(BlueprintPure, Category = "DRS|View Fade")
	static bool IsFadingOut();

	/** Whether a fade back from a colour is in progress. */
	UFUNCTION(BlueprintPure, Category = "DRS|View Fade")
	static bool IsFadingIn();

	/** Whether the fade is fully opaque, so only the fade colour is visible. */
	static bool IsViewFadeOpaque();

	/**
	 * Sets the holdout opacity, which makes the view see-through so that passthrough (if active) replaces the scene. At 1
	 * only the real world is visible (black without passthrough). The holdout is drawn beneath the fade.
	 */
	static void SetViewHoldoutOpacity(float opacity);
	static float GetViewHoldoutOpacity();

	/** Broadcast when the fade becomes fully opaque, or stops being so. */
	static FViewFadeOpaqueDelegate OnViewFadeOpaqueChanged;

protected:
	UFUNCTION(BlueprintCallable, Category = "DRS|View Fade", meta = (DisplayName = "Start View Fade (Colour Curve)", WorldContext = "worldContext", AdvancedDisplay = "fadeAudio,onFadeFinished,onFadeUpdate", AutoCreateRefTerm = "onFadeFinished,onFadeUpdate"))
	static void StartViewFadeColourCurve(const UObject* worldContext, const UCurveLinearColor* fadeCurve, bool fadeAudio,
		const FDrsViewFadeFinishedSignature& onFadeFinished, const FDrsViewFadeUpdateSignature& onFadeUpdate);

	UFUNCTION(BlueprintCallable, Category = "DRS|View Fade", meta = (DisplayName = "Start View Fade (Alpha Curve)", WorldContext = "worldContext", AdvancedDisplay = "resetCurrentFadeColour,fadeAudio,onFadeFinished,onFadeUpdate", AutoCreateRefTerm = "targetColour,onFadeFinished,onFadeUpdate"))
	static void StartViewFadeAlphaCurve(const UObject* worldContext, const FLinearColor& targetColour, const UCurveFloat* alphaCurve, bool resetCurrentFadeColour, bool fadeAudio,
		const FDrsViewFadeFinishedSignature& onFadeFinished, const FDrsViewFadeUpdateSignature& onFadeUpdate);

	UFUNCTION(BlueprintCallable, Category = "DRS|View Fade", meta = (DisplayName = "Start View Fade", WorldContext = "worldContext", AdvancedDisplay = "resetCurrentFade,fadeAudio,onFadeFinished,onFadeUpdate", AutoCreateRefTerm = "targetColour,onFadeFinished,onFadeUpdate"))
	static void StartViewFadeLinear(const UObject* worldContext, const FLinearColor& targetColour, float duration, bool resetCurrentFade, bool fadeAudio,
		const FDrsViewFadeFinishedSignature& onFadeFinished, const FDrsViewFadeUpdateSignature& onFadeUpdate);

private:
#if WITH_EDITOR
	static void ResetForPie(bool isSimulating);
#endif
};
