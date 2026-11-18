// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

// DRS_PASSTHROUGH_SUPPORTED is defined by DrsXrMixedReality.Build.cs.
#if DRS_PASSTHROUGH_SUPPORTED
#include "IOpenXRExtensionPlugin.h"

namespace Drs::XrMixedReality
{
	/**
	 * Drives passthrough through the OpenXR environment blend mode. In ALPHA_BLEND mode the runtime shows the passthrough
	 * feed wherever the scene alpha is see-through. This needs a runtime with ALPHA_BLEND and
	 * XR_EXT_composition_layer_inverted_alpha, which standalone Meta and Pico runtimes both have. Registered as an OpenXR
	 * extension plugin, so that support is checked and the blend mode reapplied whenever a session is created.
	 */
	class FXrPassthroughBackend : public IOpenXRExtensionPlugin
	{
	public:
		FXrPassthroughBackend();
		virtual ~FXrPassthroughBackend() override;

		bool IsSupported() const { return bIsSupported; }
		void SetPassthroughVisible(bool isVisible);

		// IOpenXRExtensionPlugin
		virtual FString GetDisplayName() override { return TEXT("DrsXrPassthrough"); }
		virtual void PostCreateSession(XrSession inSession) override;
		virtual void OnDestroySession(XrSession inSession) override;

	private:
		static bool DetermineSupport();
		void ApplyBlendMode() const;

		bool bIsSupported = false;
		bool bIsVisible = false;
	};
}

#else

namespace Drs::XrMixedReality
{
	class FXrPassthroughBackend
	{
	public:
		bool IsSupported() const { return false; }
		void SetPassthroughVisible(bool isVisible) {}
	};
}

#endif

namespace Drs::XrMixedReality
{
	/** The passthrough backend owned by the DrsXrMixedReality module, or null if the module isn't loaded. */
	FXrPassthroughBackend* GetPassthroughBackend();
}
