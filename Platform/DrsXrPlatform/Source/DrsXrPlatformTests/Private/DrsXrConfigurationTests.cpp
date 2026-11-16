// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrConfiguration.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FDrsXrConfiguration MakeConfiguration(EDrsXrPluginType selected, EDrsXrPluginSource source, EDrsXrPluginType expected, EDrsXrRuntimeType runtime)
	{
		FDrsXrConfiguration configuration;
		configuration.XrSelectedPlugin = selected;
		configuration.XrPluginSource = source;
		configuration.XrExpectedPlugin = expected;
		configuration.XrRuntimeType = runtime;
		return configuration;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrsXrConfigurationValidityTest, "Drs.XrPlatform.Configuration.IsValid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDrsXrConfigurationValidityTest::RunTest(const FString& parameters)
{
	using EPlugin = EDrsXrPluginType;
	using ESource = EDrsXrPluginSource;
	using ERuntime = EDrsXrRuntimeType;

	TestTrue(TEXT("No vendor plugin (plain OpenXR)"), MakeConfiguration(EPlugin::None, ESource::None, EPlugin::Meta, ERuntime::Oculus).IsValid());
	TestTrue(TEXT("Arbitrated from the hardware"), MakeConfiguration(EPlugin::Pico, ESource::Arbitrated, EPlugin::Meta, ERuntime::Pico).IsValid());
	TestTrue(TEXT("Unrecognised hardware"), MakeConfiguration(EPlugin::Meta, ESource::Loaded, EPlugin::None, ERuntime::SteamVR).IsValid());

	TestTrue(TEXT("Loaded Meta on Meta over Link"), MakeConfiguration(EPlugin::Meta, ESource::Loaded, EPlugin::Meta, ERuntime::Oculus).IsValid());
	TestTrue(TEXT("Loaded Meta on Meta over SteamVR"), MakeConfiguration(EPlugin::Meta, ESource::Loaded, EPlugin::Meta, ERuntime::SteamVR).IsValid());
	TestTrue(TEXT("Loaded Meta on a Quest"), MakeConfiguration(EPlugin::Meta, ESource::Loaded, EPlugin::Meta, ERuntime::Android).IsValid());
	TestFalse(TEXT("Loaded Meta on Pico hardware"), MakeConfiguration(EPlugin::Meta, ESource::Loaded, EPlugin::Pico, ERuntime::SteamVR).IsValid());
	TestFalse(TEXT("Loaded Meta over the Pico runtime"), MakeConfiguration(EPlugin::Meta, ESource::Loaded, EPlugin::Meta, ERuntime::Pico).IsValid());

	TestFalse(TEXT("Forced Pico on Meta hardware"), MakeConfiguration(EPlugin::Pico, ESource::Forced, EPlugin::Meta, ERuntime::Oculus).IsValid());
	TestTrue(TEXT("Forced Pico on Pico hardware"), MakeConfiguration(EPlugin::Pico, ESource::Forced, EPlugin::Pico, ERuntime::Pico).IsValid());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
