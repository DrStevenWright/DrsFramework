// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrMixedReality/DrsXrPassthroughSubsystem.h"
#include "DrsXrPassthroughBackend.h"
#include "Modules/ModuleManager.h"

using Drs::XrMixedReality::FXrPassthroughBackend;

DEFINE_LOG_CATEGORY(LogDrsXrMixedReality);

namespace
{
	/** Owns the passthrough backend, which is created at PostConfigInit so that it registers before OpenXR starts. */
	class FDrsXrMixedRealityModule : public IModuleInterface
	{
	public:
		virtual void StartupModule() override
		{
			PassthroughBackend = MakeUnique<FXrPassthroughBackend>();
		}

		virtual void ShutdownModule() override
		{
			PassthroughBackend.Reset();
		}

		FXrPassthroughBackend* GetPassthroughBackend() const { return PassthroughBackend.Get(); }

	private:
		TUniquePtr<FXrPassthroughBackend> PassthroughBackend;
	};
}

IMPLEMENT_MODULE(FDrsXrMixedRealityModule, DrsXrMixedReality)

FXrPassthroughBackend* Drs::XrMixedReality::GetPassthroughBackend()
{
	const FDrsXrMixedRealityModule* module = FModuleManager::GetModulePtr<FDrsXrMixedRealityModule>(TEXT("DrsXrMixedReality"));
	return module ? module->GetPassthroughBackend() : nullptr;
}
