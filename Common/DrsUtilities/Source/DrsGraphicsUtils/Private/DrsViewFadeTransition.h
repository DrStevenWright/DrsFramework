// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Curves/RichCurve.h"
#include "Tickable.h"

namespace Drs::Graphics { class FViewFadeRenderer; }

namespace Drs::Graphics
{
	/** Runs a view fade over time, driving the renderer's colour and opacity from per-channel curves. */
	class FViewFadeTransition : public FTickableGameObject
	{
	public:
		explicit FViewFadeTransition(TSharedRef<FViewFadeRenderer, ESPMode::ThreadSafe> renderer);

		// FTickableGameObject
		virtual void Tick(float deltaTime) override;
		virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(FViewFadeTransition, STATGROUP_Tickables); }
		virtual bool IsTickable() const override { return IsFading(); }

		void StartViewFade(const FRichCurve& rCurve, const FRichCurve& gCurve, const FRichCurve& bCurve, const FRichCurve& aCurve);

		/** Stops the fade, broadcasting OnFadeFinished, and clears the fade if shouldClearColour is set. */
		void StopViewFade(bool shouldClearColour = false);

		float GetFadeDuration() const { return FadeDuration; }
		bool GetFadeColour(FLinearColor& outColour) const;
		float GetFadeTargetOpacity() const;

		bool IsFading() const { return bIsFading; }
		bool IsFadingOut() const { return bIsFading && GetFadeTargetOpacity() > 0.0f; }
		bool IsFadingIn() const { return bIsFading && GetFadeTargetOpacity() == 0.0f; }

		using FOnViewFadeUpdated = TMulticastDelegate<void(float, const FLinearColor&)>;
		FOnViewFadeUpdated& OnFadeUpdated() { return FadeUpdated; }

		using FOnViewFadeFinished = TMulticastDelegate<void(const FLinearColor&)>;
		FOnViewFadeFinished& OnFadeFinished() { return FadeFinished; }

	private:
		void UpdateFadeDuration();
		void UpdateFade();

		TSharedRef<FViewFadeRenderer, ESPMode::ThreadSafe> Renderer;
		bool bIsFading = false;
		float CurrentFadeTime = 0.0f;
		float FadeDuration = 0.0f;

		FRichCurve RCurve;
		FRichCurve GCurve;
		FRichCurve BCurve;
		FRichCurve ACurve;

		FOnViewFadeUpdated FadeUpdated;
		FOnViewFadeFinished FadeFinished;
	};
}
