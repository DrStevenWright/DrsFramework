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

void FDrsViewFadeTransition::StopViewFade(bool shouldClearColor)
{
	FLinearColor color = Renderer->GetColor();
	color.A = Renderer->GetOpacity();
	FadeFinished.Broadcast(color);

	bIsFading = false;
	if (shouldClearColor)
	{
		Renderer->SetOpacity(0.0f);
	}

	FadeFinished.Clear();
	FadeUpdated.Clear();
}

bool FDrsViewFadeTransition::GetFadeColor(FLinearColor& outColor) const
{
	if (!IsFading())
	{
		return false;
	}

	const FLinearColor& currentColor = Renderer->GetColor();
	outColor.R = RCurve.Eval(CurrentFadeTime, currentColor.R);
	outColor.G = GCurve.Eval(CurrentFadeTime, currentColor.G);
	outColor.B = BCurve.Eval(CurrentFadeTime, currentColor.B);
	outColor.A = ACurve.Eval(CurrentFadeTime, Renderer->GetOpacity());
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
	FLinearColor fadeColor;
	if (GetFadeColor(fadeColor))
	{
		Renderer->SetColor(fadeColor, false); // The alpha channel is the opacity.
		const float progress = (FadeDuration <= CurrentFadeTime) ? 1.0f : CurrentFadeTime / FadeDuration;
		FadeUpdated.Broadcast(progress, fadeColor);
	}

	if (FadeDuration <= CurrentFadeTime)
	{
		StopViewFade(); // Keeps the final opacity.
	}
}
