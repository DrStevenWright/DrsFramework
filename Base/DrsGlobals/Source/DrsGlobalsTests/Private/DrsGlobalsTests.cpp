// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsGlobals.h"
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
	DRS_LOG_GLOBAL(Display, "DrsFramework version %s", *Drs::GetFrameworkVersionString());
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrsGlobalsLogTest, "Drs.Globals.Log",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDrsGlobalsLogTest::RunTest(const FString& parameters)
{
	// Warning verbosity so the automation framework sees and counts each message. Expected messages are matched
	// against the message text only, so an Exact match also checks that the macros add no prefix.
	AddExpectedMessage(TEXT("DrsLogTest plain 42 abc"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Exact, 1, false);
	DRS_LOG(LogDrsGlobals, Warning, "DrsLogTest plain %d %s", 42, TEXT("abc"));

	AddExpectedMessage(TEXT("DrsLogTest global"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Exact, 1, false);
	DRS_LOG_GLOBAL(Warning, "DrsLogTest global");

	// DRS_CLOG logs only when the condition is true: twice out of four.
	AddExpectedMessage(TEXT("^DrsLogTest clog [0-9]$"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Exact, 2, true);
	for (int32 index = 0; index < 4; ++index)
	{
		DRS_CLOG(index % 2 == 0, LogDrsGlobals, Warning, "DrsLogTest clog %d", index);
	}

	// The _ONCE variants keep their state for the whole process, so they only fire in the first run of this test.
	static int32 runCount = 0;
	if (++runCount > 1)
	{
		AddInfo(TEXT("DRS_LOG_ONCE and DRS_CLOG_ONCE already fired in this process; run the test in a new process to check them."));
		return true;
	}

	// DRS_LOG_ONCE fires on the first of three passes only.
	AddExpectedMessage(TEXT("^DrsLogTest log once [0-9]$"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Exact, 1, true);
	for (int32 index = 0; index < 3; ++index)
	{
		DRS_LOG_ONCE(LogDrsGlobals, Warning, "DrsLogTest log once %d", index);
	}

	// DRS_CLOG_ONCE ignores passes with a false condition and fires the first time it is true (index 2) only.
	AddExpectedMessage(TEXT("DrsLogTest clog once 2"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Exact, 1, false);
	for (int32 index = 0; index < 5; ++index)
	{
		DRS_CLOG_ONCE(index >= 2, LogDrsGlobals, Warning, "DrsLogTest clog once %d", index);
	}

	// Every macro is a single statement, so it compiles in an unbraced if/else.
	AddExpectedMessage(TEXT("DrsLogTest if branch"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Exact, 1, false);
	const bool isIfBranch = parameters.IsEmpty();
	if (isIfBranch)
		DRS_LOG_ONCE(LogDrsGlobals, Warning, "DrsLogTest if branch");
	else
		DRS_CLOG_ONCE(true, LogDrsGlobals, Warning, "DrsLogTest else branch");
	if (!isIfBranch)
		DRS_LOG(LogDrsGlobals, Warning, "DrsLogTest else branch");
	else
		DRS_CLOG(false, LogDrsGlobals, Warning, "DrsLogTest else branch");

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
