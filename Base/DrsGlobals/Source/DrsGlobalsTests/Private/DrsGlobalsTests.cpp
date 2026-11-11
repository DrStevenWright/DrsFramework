// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsLog.h"
#include "DrsObjectUtils.h"
#include "DrsVersion.h"
#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, DrsGlobalsTests)

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrsGlobalsVersionTest, "Drs.Globals.Version",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDrsGlobalsVersionTest::RunTest(const FString& parameters)
{
	const FString expected = FString::Printf(TEXT("%d.%d.%d"),
		Drs::FrameworkVersionMajor, Drs::FrameworkVersionMinor, Drs::FrameworkVersionPatch);
	TestEqual(TEXT("Version string"), Drs::GetFrameworkVersionString(), expected);
	DRS_LOG_GLOBAL(Display, TEXT("DrsFramework version %s"), *Drs::GetFrameworkVersionString());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrsBaseUtilitiesDebugNameTest, "Drs.Globals.BaseUtilities.DebugName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDrsBaseUtilitiesDebugNameTest::RunTest(const FString& parameters)
{
	TestEqual(TEXT("Null object"), Drs::GetDebugName(nullptr), FString(TEXT("None")));
	TestTrue(TEXT("Class prefix"), Drs::GetDebugName(UObject::StaticClass()).StartsWith(TEXT("Class:")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
