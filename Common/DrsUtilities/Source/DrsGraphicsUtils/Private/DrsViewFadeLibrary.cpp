// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsGraphicsUtils/DrsViewFadeLibrary.h"

#include "AudioDevice.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveLinearColor.h"
#include "DrsBase/DrsDelegateUtils.h"
#include "DrsViewFadeRenderer.h"
#include "DrsViewFadeTransition.h"
#include "Engine/World.h"
#include "SceneViewExtension.h"
#if WITH_EDITOR
#include "Editor.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsViewFadeLibrary)

using Drs::Graphics::FViewFadeRenderer;
using Drs::Graphics::FViewFadeTransition;

UDrsViewFadeLibrary::FViewFadeOpaqueDelegate UDrsViewFadeLibrary::OnViewFadeOpaqueChanged;

namespace
{
	TSharedRef<FViewFadeRenderer, ESPMode::ThreadSafe> GetFadeRenderer()
	{
		static TSharedRef<FViewFadeRenderer, ESPMode::ThreadSafe> renderer = FSceneViewExtensions::NewExtension<FViewFadeRenderer>();
		return renderer;
	}

	FViewFadeTransition& GetFadeTransition()
	{
		static FViewFadeTransition transition(GetFadeRenderer());
		return transition;
	}

	void SetAudioVolume(const UObject* worldContext, float volume)
	{
		const UWorld* world = IsValid(worldContext) ? worldContext->GetWorld() : nullptr;
		if (FAudioDevice* audioDevice = IsValid(world) ? world->GetAudioDeviceRaw() : nullptr)
		{
			audioDevice->SetTransientPrimaryVolume(volume);
		}
	}
}

UDrsViewFadeLibrary::UDrsViewFadeLibrary()
{
#if WITH_EDITOR
	// Clear the fade around PIE, so that the editor viewport doesn't stay black or partly faded.
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FEditorDelegates::BeginPIE.AddStatic(&UDrsViewFadeLibrary::ResetForPie);
		FEditorDelegates::EndPIE.AddStatic(&UDrsViewFadeLibrary::ResetForPie);
	}
#endif
}

#if WITH_EDITOR
void UDrsViewFadeLibrary::ResetForPie(bool isSimulating)
{
	GetFadeRenderer()->SetOpacity(0.0f);
	GetFadeRenderer()->SetHoldoutOpacity(0.0f);
}
#endif

void UDrsViewFadeLibrary::SetViewFadeColour(const UObject* worldContext, const FLinearColor& colour, float opacity, bool fadeAudio)
{
	SetViewFadeOpacity(worldContext, opacity, fadeAudio);
	GetFadeRenderer()->SetColour(colour);
}

void UDrsViewFadeLibrary::SetViewFadeOpacity(const UObject* worldContext, float opacity, bool fadeAudio)
{
	GetFadeTransition().StopViewFade();
	GetFadeRenderer()->SetOpacity(opacity);
	if (fadeAudio)
	{
		SetAudioVolume(worldContext, 1.0f - opacity);
	}
}

void UDrsViewFadeLibrary::StartViewFade(const UObject* worldContext, const FRichCurve& rCurve, const FRichCurve& gCurve, const FRichCurve& bCurve, const FRichCurve& aCurve,
	bool fadeAudio, const FViewFadeFinishedDelegate& onFadeFinished, const FViewFadeUpdateDelegate& onFadeUpdate)
{
	FViewFadeTransition& transition = GetFadeTransition();
	transition.StopViewFade();
	transition.OnFadeFinished().Add(onFadeFinished);
	transition.OnFadeUpdated().Add(onFadeUpdate);

	if (fadeAudio && IsValid(worldContext))
	{
		UWorld* world = worldContext->GetWorld();
		transition.OnFadeUpdated().AddWeakLambda(world, [world](float progress, const FLinearColor& colour)
		{
			SetAudioVolume(world, 1.0f - colour.A);
		});
	}

	transition.StartViewFade(rCurve, gCurve, bCurve, aCurve);
}

void UDrsViewFadeLibrary::StartViewFade(const UObject* worldContext, const UCurveLinearColor* fadeCurve, bool fadeAudio,
	const FViewFadeFinishedDelegate& onFadeFinished, const FViewFadeUpdateDelegate& onFadeUpdate)
{
	check(fadeCurve);
	StartViewFade(worldContext, fadeCurve->FloatCurves[0], fadeCurve->FloatCurves[1], fadeCurve->FloatCurves[2], fadeCurve->FloatCurves[3], fadeAudio, onFadeFinished, onFadeUpdate);
}

void UDrsViewFadeLibrary::StartViewFade(const UObject* worldContext, const FLinearColor& targetColour, float duration, bool resetCurrentFade, bool fadeAudio,
	const FViewFadeFinishedDelegate& onFadeFinished, const FViewFadeUpdateDelegate& onFadeUpdate)
{
	FRichCurve rCurve, gCurve, bCurve, aCurve;
	if (resetCurrentFade || FMath::IsNearlyZero(GetViewFadeOpacity()))
	{
		rCurve.AddKey(0.0f, targetColour.R);
		gCurve.AddKey(0.0f, targetColour.G);
		bCurve.AddKey(0.0f, targetColour.B);
		aCurve.AddKey(0.0f, 0.0f);
	}
	else if (duration > 0.0f)
	{
		const FLinearColor& currentColour = GetFadeRenderer()->GetColour();
		rCurve.AddKey(0.0f, currentColour.R);
		gCurve.AddKey(0.0f, currentColour.G);
		bCurve.AddKey(0.0f, currentColour.B);
		aCurve.AddKey(0.0f, GetViewFadeOpacity());
	}

	rCurve.AddKey(duration, targetColour.R);
	gCurve.AddKey(duration, targetColour.G);
	bCurve.AddKey(duration, targetColour.B);
	aCurve.AddKey(duration, targetColour.A);

	StartViewFade(worldContext, rCurve, gCurve, bCurve, aCurve, fadeAudio, onFadeFinished, onFadeUpdate);
}

void UDrsViewFadeLibrary::StartViewFade(const UObject* worldContext, const FLinearColor& targetColour, const UCurveFloat* alphaCurve, bool resetCurrentFadeColour, bool fadeAudio,
	const FViewFadeFinishedDelegate& onFadeFinished, const FViewFadeUpdateDelegate& onFadeUpdate)
{
	check(alphaCurve);

	FRichCurve rCurve, gCurve, bCurve;
	const FLinearColor startColour = (resetCurrentFadeColour || FMath::IsNearlyZero(GetViewFadeOpacity())) ? targetColour : GetFadeRenderer()->GetColour();
	rCurve.AddKey(0.0f, startColour.R);
	gCurve.AddKey(0.0f, startColour.G);
	bCurve.AddKey(0.0f, startColour.B);

	float minTime = 0.0f;
	float maxTime = 0.0f;
	alphaCurve->FloatCurve.GetTimeRange(minTime, maxTime);
	rCurve.AddKey(maxTime, targetColour.R);
	gCurve.AddKey(maxTime, targetColour.G);
	bCurve.AddKey(maxTime, targetColour.B);

	StartViewFade(worldContext, rCurve, gCurve, bCurve, alphaCurve->FloatCurve, fadeAudio, onFadeFinished, onFadeUpdate);
}

void UDrsViewFadeLibrary::StopViewFade(bool keepFadeColour)
{
	GetFadeTransition().StopViewFade(!keepFadeColour);
}

float UDrsViewFadeLibrary::GetViewFadeOpacity()
{
	return GetFadeRenderer()->GetOpacity();
}

void UDrsViewFadeLibrary::GetViewFadeColour(FLinearColor& outColour)
{
	outColour = GetFadeRenderer()->GetColour();
}

bool UDrsViewFadeLibrary::IsFading()
{
	return GetViewFadeOpacity() > UE_SMALL_NUMBER;
}

bool UDrsViewFadeLibrary::IsFadingOut()
{
	return GetFadeTransition().IsFadingOut();
}

bool UDrsViewFadeLibrary::IsFadingIn()
{
	return GetFadeTransition().IsFadingIn();
}

bool UDrsViewFadeLibrary::IsViewFadeOpaque()
{
	return GetFadeRenderer()->IsOpaque();
}

void UDrsViewFadeLibrary::SetViewHoldoutOpacity(float opacity)
{
	GetFadeRenderer()->SetHoldoutOpacity(FMath::Clamp(opacity, 0.0f, 1.0f));
}

float UDrsViewFadeLibrary::GetViewHoldoutOpacity()
{
	return GetFadeRenderer()->GetHoldoutOpacity();
}

void UDrsViewFadeLibrary::StartViewFadeColourCurve(const UObject* worldContext, const UCurveLinearColor* fadeCurve, bool fadeAudio,
	const FDrsViewFadeFinishedSignature& onFadeFinished, const FDrsViewFadeUpdateSignature& onFadeUpdate)
{
	StartViewFade(worldContext, fadeCurve, fadeAudio, Drs::AsNative<FViewFadeFinishedDelegate>(onFadeFinished), Drs::AsNative<FViewFadeUpdateDelegate>(onFadeUpdate));
}

void UDrsViewFadeLibrary::StartViewFadeAlphaCurve(const UObject* worldContext, const FLinearColor& targetColour, const UCurveFloat* alphaCurve, bool resetCurrentFadeColour, bool fadeAudio,
	const FDrsViewFadeFinishedSignature& onFadeFinished, const FDrsViewFadeUpdateSignature& onFadeUpdate)
{
	StartViewFade(worldContext, targetColour, alphaCurve, resetCurrentFadeColour, fadeAudio, Drs::AsNative<FViewFadeFinishedDelegate>(onFadeFinished), Drs::AsNative<FViewFadeUpdateDelegate>(onFadeUpdate));
}

void UDrsViewFadeLibrary::StartViewFadeLinear(const UObject* worldContext, const FLinearColor& targetColour, float duration, bool resetCurrentFade, bool fadeAudio,
	const FDrsViewFadeFinishedSignature& onFadeFinished, const FDrsViewFadeUpdateSignature& onFadeUpdate)
{
	StartViewFade(worldContext, targetColour, duration, resetCurrentFade, fadeAudio, Drs::AsNative<FViewFadeFinishedDelegate>(onFadeFinished), Drs::AsNative<FViewFadeUpdateDelegate>(onFadeUpdate));
}
