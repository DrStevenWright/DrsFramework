// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrArbiter/DrsXrArbiter.h"
#include "DrsXrArbiter/DrsXrSelection.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

using Drs::XrArbiter::EXrPluginSource;
using Drs::XrArbiter::FXrSelection;

namespace
{
	const FName Meta(TEXT("Meta"));
	const FName Pico(TEXT("Pico"));

	/** A runtime by name: Oculus implies Meta, PicoConnect implies Pico, and SteamVR and Android imply no vendor. */
	FXrSelection MakeSelection(FName plugin, EXrPluginSource source, FName expected, const TCHAR* runtime)
	{
		FXrSelection selection;
		selection.Plugin = plugin;
		selection.PluginSource = source;
		selection.ExpectedPlugin = expected;
		selection.Runtime = runtime;
		const FString runtimeName(runtime);
		selection.RuntimeVendor = runtimeName == TEXT("Oculus") ? Meta : runtimeName == TEXT("PicoConnect") ? Pico : NAME_None;
		return selection;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrsXrSelectionConsistencyTest, "Drs.XrPlatform.Arbiter.Selection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDrsXrSelectionConsistencyTest::RunTest(const FString& parameters)
{
	using ESource = EXrPluginSource;
	const FName none;

	TestTrue(TEXT("No vendor plugin (plain OpenXR)"), MakeSelection(none, ESource::None, Meta, TEXT("Oculus")).IsValid());
	TestTrue(TEXT("Arbitrated from the hardware"), MakeSelection(Pico, ESource::Arbitrated, Meta, TEXT("PicoConnect")).IsValid());
	TestTrue(TEXT("Unrecognised hardware"), MakeSelection(Meta, ESource::Loaded, none, TEXT("SteamVR")).IsValid());

	TestTrue(TEXT("Loaded Meta on Meta over Link"), MakeSelection(Meta, ESource::Loaded, Meta, TEXT("Oculus")).IsValid());
	TestTrue(TEXT("Loaded Meta on Meta over SteamVR"), MakeSelection(Meta, ESource::Loaded, Meta, TEXT("SteamVR")).IsValid());
	TestTrue(TEXT("Loaded Meta on a Quest"), MakeSelection(Meta, ESource::Loaded, Meta, TEXT("Android")).IsValid());
	TestTrue(TEXT("No runtime selected"), MakeSelection(Meta, ESource::Loaded, Meta, TEXT("None")).IsValid());
	TestFalse(TEXT("Loaded Meta on Pico hardware"), MakeSelection(Meta, ESource::Loaded, Pico, TEXT("SteamVR")).IsValid());
	TestFalse(TEXT("Loaded Meta over the Pico runtime"), MakeSelection(Meta, ESource::Loaded, Meta, TEXT("PicoConnect")).IsValid());
	TestFalse(TEXT("Loaded Pico over the Oculus runtime, hardware unrecognised"), MakeSelection(Pico, ESource::Loaded, none, TEXT("Oculus")).IsValid());
	TestTrue(TEXT("Loaded Pico over SteamVR, hardware unrecognised"), MakeSelection(Pico, ESource::Loaded, none, TEXT("SteamVR")).IsValid());

	TestFalse(TEXT("Forced Pico on Meta hardware"), MakeSelection(Pico, ESource::Forced, Meta, TEXT("Oculus")).IsValid());
	TestTrue(TEXT("Forced Pico on Pico hardware"), MakeSelection(Pico, ESource::Forced, Pico, TEXT("PicoConnect")).IsValid());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
