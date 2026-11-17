// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsViewFadeTransition.h"
#include "DrsViewFadeRenderer.h"

FDrsViewFadeTransition::FDrsViewFadeTransition(TSharedRef<FDrsViewFadeRenderer, ESPMode::ThreadSafe> renderer)
	: FTickableGameObject(ETickableTickType::Conditional)
	, Renderer(renderer)
{
}

void FDrsViewFadeTransition::Tick(float deltaTime)
{
	CurrentFadeTime += deltaTime;
	UpdateFade();
}

void FDrsViewFadeTransition::StartViewFade(const FRichCurve& rCurve, const FRichCurve& gCurve, const FRichCurve& bCurve, const FRichCurve& aCurve)
{
	bIsFading = true;
	CurrentFadeTime = 0.0f;
	RCurve = rCurve;
	GCurve = gCurve;
	BCurve = bCurve;
	ACurve = aCurve;

	UpdateFadeDuration();
	UpdateFade();
}

void FDrsViewFadeTransition::StopViewFade(bool shouldClearColour)
{
	FLinearColor colour = Renderer->GetColour();
	colour.A = Renderer->GetOpacity();
	FadeFinished.Broadcast(colour);

	bIsFading = false;
	if (shouldClearColour)
	{
		Renderer->SetOpacity(0.0f);
	}

	FadeFinished.Clear();
	FadeUpdated.Clear();
}

bool FDrsViewFadeTransition::GetFadeColour(FLinearColor& outColour) const
{
	if (!IsFading())
	{
		return false;
	}

	const FLinearColor& currentColour = Renderer->GetColour();
	outColour.R = RCurve.Eval(CurrentFadeTime, currentColour.R);
	outColour.G = GCurve.Eval(CurrentFadeTime, currentColour.G);
	outColour.B = BCurve.Eval(CurrentFadeTime, currentColour.B);
	outColour.A = ACurve.Eval(CurrentFadeTime, Renderer->GetOpacity());
	return true;
}

float FDrsViewFadeTransition::GetFadeTargetOpacity() const
{
	return ACurve.IsEmpty() ? 0.0f : ACurve.Keys.Last().Value;
}

void FDrsViewFadeTransition::UpdateFadeDuration()
{
	float maxTime = 0.0f;
	for (const FRichCurve* curve : { &RCurve, &GCurve, &BCurve, &ACurve })
	{
		float minCurveTime = 0.0f;
		float maxCurveTime = 0.0f;
		curve->GetTimeRange(minCurveTime, maxCurveTime);
		maxTime = FMath::Max(maxTime, maxCurveTime);
	}
	FadeDuration = maxTime;
}

void FDrsViewFadeTransition::UpdateFade()
{
	FLinearColor fadeColour;
	if (GetFadeColour(fadeColour))
	{
		Renderer->SetColour(fadeColour, false); // The alpha channel is the opacity.
		const float progress = (FadeDuration <= CurrentFadeTime) ? 1.0f : CurrentFadeTime / FadeDuration;
		FadeUpdated.Broadcast(progress, fadeColour);
	}

	if (FadeDuration <= CurrentFadeTime)
	{
		StopViewFade(); // Keeps the final opacity.
	}
}
