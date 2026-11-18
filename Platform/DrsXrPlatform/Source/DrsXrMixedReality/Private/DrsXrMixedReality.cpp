// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrMixedReality/DrsXrPassthroughSubsystem.h"
#include "DrsXrPassthroughBackend.h"

#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogDrsXrMixedReality);

namespace
{
	/** Owns the passthrough backend, which is created at PostConfigInit so that it registers before OpenXR starts. */
	class FDrsXrMixedRealityModule : public IModuleInterface
	{
	public:
		virtual void StartupModule() override
		{
			PassthroughBackend = MakeUnique<FDrsXrPassthroughBackend>();
		}

		virtual void ShutdownModule() override
		{
			PassthroughBackend.Reset();
		}

		FDrsXrPassthroughBackend* GetPassthroughBackend() const { return PassthroughBackend.Get(); }

	private:
		TUniquePtr<FDrsXrPassthroughBackend> PassthroughBackend;
	};
}

IMPLEMENT_MODULE(FDrsXrMixedRealityModule, DrsXrMixedReality)

FDrsXrPassthroughBackend* GetDrsXrPassthroughBackend()
{
	const FDrsXrMixedRealityModule* module = FModuleManager::GetModulePtr<FDrsXrMixedRealityModule>(TEXT("DrsXrMixedReality"));
	return module ? module->GetPassthroughBackend() : nullptr;
}
