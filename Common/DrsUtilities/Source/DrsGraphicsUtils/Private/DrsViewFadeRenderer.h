// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "SceneViewExtension.h"

namespace Drs::Graphics
{
	/**
	 * Draws the view fade after the scene and post-processing, before the loading screen.
	 *
	 * Scene alpha matters when passthrough is composited behind the scene (engine convention: 0 = opaque, 1 = see-through).
	 * Two full-screen passes are drawn, holdout first so that the fade always wins:
	 * - Holdout: makes the view see-through, so passthrough (if active) replaces the scene. It clears colour towards black
	 *   and alpha towards see-through; the compositor then adds passthrough to that black.
	 * - Fade: blends towards the fade colour and keeps the view opaque, so a full fade never reveals passthrough.
	 */
	class FViewFadeRenderer : public FSceneViewExtensionBase
	{
	public:
		FViewFadeRenderer(const FAutoRegister& autoRegister) : FSceneViewExtensionBase(autoRegister) {}

		virtual void SetupViewFamily(FSceneViewFamily& inViewFamily) override {}
		virtual void SetupView(FSceneViewFamily& inViewFamily, FSceneView& inView) override {}
		virtual void BeginRenderViewFamily(FSceneViewFamily& inViewFamily) override {}
		virtual void PostRenderView_RenderThread(FRDGBuilder& graphBuilder, FSceneView& inView) override;

		/** Before the loading screen, which has priority 0. */
		virtual int32 GetPriority() const override { return 100; }

		void SetColour(const FLinearColor& colour, bool shouldIgnoreAlpha = true);
		void SetOpacity(float opacity);
		void SetHoldoutOpacity(float opacity) { HoldoutOpacity = opacity; }

		const FLinearColor& GetColour() const { return FadeColour; }
		float GetOpacity() const { return FadeOpacity; }
		float GetHoldoutOpacity() const { return HoldoutOpacity; }
		bool IsOpaque() const { return FadeOpacity >= 1.0f - KINDA_SMALL_NUMBER; }

	protected:
		virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& context) const override;

	private:
		FLinearColor FadeColour = FLinearColor::Black;
		float FadeOpacity = 0.0f;
		float HoldoutOpacity = 0.0f;
	};
}
