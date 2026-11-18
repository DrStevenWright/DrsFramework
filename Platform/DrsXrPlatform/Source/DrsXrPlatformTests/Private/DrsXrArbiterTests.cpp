// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrArbiter/DrsXrArbiter.h"
#include "DrsXrArbiter/DrsXrArbiterTypes.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FDrsArbitrationInputs MobileInputs(const FString& deviceMake, const FString& gpuFamily)
	{
		FDrsArbitrationInputs inputs;
		inputs.DeviceMake = &deviceMake;
		inputs.GpuFamily = &gpuFamily;
		return inputs;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrsXrArbiterParseTest, "Drs.XrPlatform.Arbiter.Parse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDrsXrArbiterParseTest::RunTest(const FString& parameters)
{
	const FDrsArbitrationRule rule = FDrsArbitrationRule::ParseFromConfig(
		TEXT("(Configuration=Meta,Conditions=((ConditionType=RequiredTrue,Criteria=DeviceMake,Value=\"Oculus\"),(ConditionType=RequiredFalse,Criteria=GpuFamily,Value=\"Adreno (TM) 540\")))"));

	TestTrue(TEXT("Rule is valid"), rule.IsValid());
	TestTrue(TEXT("Configuration"), rule.Configuration == EDrsXrPluginType::Meta);
	if (TestEqual(TEXT("Condition count"), rule.Conditions.Num(), 2))
	{
		TestTrue(TEXT("First condition type"), rule.Conditions[0].ConditionType == EDrsArbitrationConditionType::RequiredTrue);
		TestTrue(TEXT("First criteria"), rule.Conditions[0].Criteria == EDrsArbitrationCriteria::DeviceMake);
		TestEqual(TEXT("First value"), rule.Conditions[0].Value, FString(TEXT("Oculus")));
		TestTrue(TEXT("Second condition type"), rule.Conditions[1].ConditionType == EDrsArbitrationConditionType::RequiredFalse);
		TestEqual(TEXT("Second value"), rule.Conditions[1].Value, FString(TEXT("Adreno (TM) 540")));
	}

	TestFalse(TEXT("Rule without conditions is invalid"), FDrsArbitrationRule::ParseFromConfig(TEXT("(Configuration=Meta)")).IsValid());

	TestTrue(TEXT("Plugin type name round trip"), Drs::XrArbiter::PluginTypeFromString(Drs::XrArbiter::PluginTypeToString(EDrsXrPluginType::Pico)) == EDrsXrPluginType::Pico);
	AddExpectedError(TEXT("Unknown XR plugin type"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Unknown plugin type name"), Drs::XrArbiter::PluginTypeFromString(TEXT("Vive")).IsSet());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrsXrArbiterRulesTest, "Drs.XrPlatform.Arbiter.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDrsXrArbiterRulesTest::RunTest(const FString& parameters)
{
	// The Quest rule from the plugin config: an Oculus device, except the Quest 1 GPU.
	const FDrsArbitrationRule questRule = FDrsArbitrationRule::ParseFromConfig(
		TEXT("(Configuration=Meta,Conditions=((ConditionType=RequiredTrue,Criteria=DeviceMake,Value=\"Oculus\"),(ConditionType=RequiredFalse,Criteria=GpuFamily,Value=\"Adreno (TM) 540\")))"));
	TestTrue(TEXT("Quest 3 matches"), questRule.Evaluate(MobileInputs(TEXT("Oculus"), TEXT("Adreno (TM) 740"))));
	TestFalse(TEXT("Quest 1 GPU is excluded"), questRule.Evaluate(MobileInputs(TEXT("Oculus"), TEXT("Adreno (TM) 540"))));
	TestFalse(TEXT("Other make does not match"), questRule.Evaluate(MobileInputs(TEXT("Pico"), TEXT("Adreno (TM) 740"))));

	// Optional conditions: any one is enough. Unknown properties count as not satisfied.
	const FDrsArbitrationRule nameRule = FDrsArbitrationRule::ParseFromConfig(
		TEXT("(Configuration=Meta,Conditions=((ConditionType=OptionalTrue,Criteria=DeviceName,Value=\"Meta\"),(ConditionType=OptionalTrue,Criteria=DeviceName,Value=\"Quest2\")))"));
	const FString quest2Name(TEXT("Oculus Quest2"));
	const FString picoName(TEXT("PICO 4 Ultra"));
	FDrsArbitrationInputs nameInputs;
	nameInputs.DeviceName = &quest2Name;
	TestTrue(TEXT("Second optional condition is enough"), nameRule.Evaluate(nameInputs));
	nameInputs.DeviceName = &picoName;
	TestFalse(TEXT("No optional condition met"), nameRule.Evaluate(nameInputs));
	TestFalse(TEXT("Unknown device name"), nameRule.Evaluate(FDrsArbitrationInputs()));

	// Runtime rule.
	const FDrsArbitrationRule runtimeRule = FDrsArbitrationRule::ParseFromConfig(
		TEXT("(Configuration=Meta,Conditions=((ConditionType=RequiredTrue,Criteria=XrRuntime,Value=\"Oculus\")))"));
	const FString oculusRuntime = Drs::XrArbiter::RuntimeTypeToString(EDrsXrRuntimeType::Oculus);
	const FString steamRuntime = Drs::XrArbiter::RuntimeTypeToString(EDrsXrRuntimeType::SteamVR);
	FDrsArbitrationInputs runtimeInputs;
	runtimeInputs.XrRuntime = &oculusRuntime;
	TestTrue(TEXT("Oculus runtime"), runtimeRule.Evaluate(runtimeInputs));
	runtimeInputs.XrRuntime = &steamRuntime;
	TestFalse(TEXT("SteamVR runtime has no rule"), runtimeRule.Evaluate(runtimeInputs));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
