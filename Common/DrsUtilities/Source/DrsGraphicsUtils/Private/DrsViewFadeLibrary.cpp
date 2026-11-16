// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsViewFadeLibrary.h"
#include "AudioDevice.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveLinearColor.h"
#include "DrsDelegateUtils.h"
#include "DrsViewFadeRenderer.h"
#include "DrsViewFadeTransition.h"
#include "Engine/World.h"
#include "SceneViewExtension.h"
#if WITH_EDITOR
#include "Editor.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsViewFadeLibrary)

UDrsViewFadeLibrary::FViewFadeOpaqueDelegate UDrsViewFadeLibrary::OnViewFadeOpaqueChanged;

namespace
{
	TSharedRef<FDrsViewFadeRenderer, ESPMode::ThreadSafe> GetFadeRenderer()
	{
		static TSharedRef<FDrsViewFadeRenderer, ESPMode::ThreadSafe> renderer = FSceneViewExtensions::NewExtension<FDrsViewFadeRenderer>();
		return renderer;
	}

	FDrsViewFadeTransition& GetFadeTransition()
	{
		static FDrsViewFadeTransition transition(GetFadeRenderer());
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

void UDrsViewFadeLibrary::SetViewFadeColor(const UObject* worldContext, const FLinearColor& color, float opacity, bool fadeAudio)
{
	SetViewFadeOpacity(worldContext, opacity, fadeAudio);
	GetFadeRenderer()->SetColor(color);
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
	FDrsViewFadeTransition& transition = GetFadeTransition();
	transition.StopViewFade();
	transition.OnFadeFinished().Add(onFadeFinished);
	transition.OnFadeUpdated().Add(onFadeUpdate);

	if (fadeAudio && IsValid(worldContext))
	{
		UWorld* world = worldContext->GetWorld();
		transition.OnFadeUpdated().AddWeakLambda(world, [world](float progress, const FLinearColor& color)
		{
			SetAudioVolume(world, 1.0f - color.A);
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

void UDrsViewFadeLibrary::StartViewFade(const UObject* worldContext, const FLinearColor& targetColor, float duration, bool resetCurrentFade, bool fadeAudio,
	const FViewFadeFinishedDelegate& onFadeFinished, const FViewFadeUpdateDelegate& onFadeUpdate)
{
	FRichCurve rCurve, gCurve, bCurve, aCurve;
	if (resetCurrentFade || FMath::IsNearlyZero(GetViewFadeOpacity()))
	{
		rCurve.AddKey(0.0f, targetColor.R);
		gCurve.AddKey(0.0f, targetColor.G);
		bCurve.AddKey(0.0f, targetColor.B);
		aCurve.AddKey(0.0f, 0.0f);
	}
	else if (duration > 0.0f)
	{
		const FLinearColor& currentColor = GetFadeRenderer()->GetColor();
		rCurve.AddKey(0.0f, currentColor.R);
		gCurve.AddKey(0.0f, currentColor.G);
		bCurve.AddKey(0.0f, currentColor.B);
		aCurve.AddKey(0.0f, GetViewFadeOpacity());
	}

	rCurve.AddKey(duration, targetColor.R);
	gCurve.AddKey(duration, targetColor.G);
	bCurve.AddKey(duration, targetColor.B);
	aCurve.AddKey(duration, targetColor.A);

	StartViewFade(worldContext, rCurve, gCurve, bCurve, aCurve, fadeAudio, onFadeFinished, onFadeUpdate);
}

void UDrsViewFadeLibrary::StartViewFade(const UObject* worldContext, const FLinearColor& targetColor, const UCurveFloat* alphaCurve, bool resetCurrentFadeColor, bool fadeAudio,
	const FViewFadeFinishedDelegate& onFadeFinished, const FViewFadeUpdateDelegate& onFadeUpdate)
{
	check(alphaCurve);

	FRichCurve rCurve, gCurve, bCurve;
	const FLinearColor startColor = (resetCurrentFadeColor || FMath::IsNearlyZero(GetViewFadeOpacity())) ? targetColor : GetFadeRenderer()->GetColor();
	rCurve.AddKey(0.0f, startColor.R);
	gCurve.AddKey(0.0f, startColor.G);
	bCurve.AddKey(0.0f, startColor.B);

	float minTime = 0.0f;
	float maxTime = 0.0f;
	alphaCurve->FloatCurve.GetTimeRange(minTime, maxTime);
	rCurve.AddKey(maxTime, targetColor.R);
	gCurve.AddKey(maxTime, targetColor.G);
	bCurve.AddKey(maxTime, targetColor.B);

	StartViewFade(worldContext, rCurve, gCurve, bCurve, alphaCurve->FloatCurve, fadeAudio, onFadeFinished, onFadeUpdate);
}

void UDrsViewFadeLibrary::StopViewFade(bool keepFadeColor)
{
	GetFadeTransition().StopViewFade(!keepFadeColor);
}

float UDrsViewFadeLibrary::GetViewFadeOpacity()
{
	return GetFadeRenderer()->GetOpacity();
}

void UDrsViewFadeLibrary::GetViewFadeColor(FLinearColor& outColor)
{
	outColor = GetFadeRenderer()->GetColor();
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

void UDrsViewFadeLibrary::StartViewFadeColorCurve(const UObject* worldContext, const UCurveLinearColor* fadeCurve, bool fadeAudio,
	const FDrsViewFadeFinishedSignature& onFadeFinished, const FDrsViewFadeUpdateSignature& onFadeUpdate)
{
	StartViewFade(worldContext, fadeCurve, fadeAudio, Drs::AsNative<FViewFadeFinishedDelegate>(onFadeFinished), Drs::AsNative<FViewFadeUpdateDelegate>(onFadeUpdate));
}

void UDrsViewFadeLibrary::StartViewFadeAlphaCurve(const UObject* worldContext, const FLinearColor& targetColor, const UCurveFloat* alphaCurve, bool resetCurrentFadeColor, bool fadeAudio,
	const FDrsViewFadeFinishedSignature& onFadeFinished, const FDrsViewFadeUpdateSignature& onFadeUpdate)
{
	StartViewFade(worldContext, targetColor, alphaCurve, resetCurrentFadeColor, fadeAudio, Drs::AsNative<FViewFadeFinishedDelegate>(onFadeFinished), Drs::AsNative<FViewFadeUpdateDelegate>(onFadeUpdate));
}

void UDrsViewFadeLibrary::StartViewFadeLinear(const UObject* worldContext, const FLinearColor& targetColor, float duration, bool resetCurrentFade, bool fadeAudio,
	const FDrsViewFadeFinishedSignature& onFadeFinished, const FDrsViewFadeUpdateSignature& onFadeUpdate)
{
	StartViewFade(worldContext, targetColor, duration, resetCurrentFade, fadeAudio, Drs::AsNative<FViewFadeFinishedDelegate>(onFadeFinished), Drs::AsNative<FViewFadeUpdateDelegate>(onFadeUpdate));
}
