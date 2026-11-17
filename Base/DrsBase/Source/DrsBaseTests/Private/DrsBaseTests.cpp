// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsBase.h"
#include "DrsBaseTestTypes.h"
#include "DrsHand.h"
#include "DrsHandLibrary.h"
#include "DrsLog.h"
#include "DrsObjectUtils.h"
#include "DrsVersion.h"
#include "DrsVotableState.h"
#include "InputCoreTypes.h"
#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, DrsBaseTests)

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrsBaseVersionTest, "Drs.Base.Version", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDrsBaseVersionTest::RunTest(const FString& parameters)
{
	const FString expected = FString::Printf(TEXT("%d.%d.%d"),
		Drs::FrameworkVersionMajor, Drs::FrameworkVersionMinor, Drs::FrameworkVersionPatch);
	TestEqual(TEXT("Version string"), Drs::GetFrameworkVersionString(), expected);
	DRS_LOG_GLOBAL(Display, "DrsFramework version %s", *Drs::GetFrameworkVersionString());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrsBaseDebugNameTest, "Drs.Base.DebugName", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDrsBaseDebugNameTest::RunTest(const FString& parameters)
{
	TestEqual(TEXT("Null object"), Drs::GetDebugName(nullptr), FString(TEXT("None")));
	TestTrue(TEXT("Class prefix"), Drs::GetDebugName(UObject::StaticClass()).StartsWith(TEXT("Class:")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrsBaseVotableStateTest, "Drs.Base.VotableState", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDrsBaseVotableStateTest::RunTest(const FString& parameters)
{
	TDrsVotableState<FName> nameState;
	TestEqual(TEXT("No votes string"), nameState.GetVotesString(), FString(TEXT("...")));
	TestTrue(TEXT("First vote changes the state"), nameState.CastVote(TEXT("A")));
	TestFalse(TEXT("Second vote doesn't change the state"), nameState.CastVote(NAME_None));
	TestFalse(TEXT("Repeated vote doesn't change the state"), nameState.CastVote(TEXT("A")));
	TestEqual(TEXT("Vote count"), nameState.GetVoteCount(), 2);
	TestTrue(TEXT("Vote A cast"), nameState.WasVoteCast(TEXT("A")));
	TestEqual(TEXT("Name votes string"), nameState.GetVotesString(), FString(TEXT("A, Anonymous")));
	TestFalse(TEXT("Withdrawing one of two votes doesn't change the state"), nameState.WithdrawVote(TEXT("A")));
	TestTrue(TEXT("Withdrawing the last vote changes the state"), nameState.SetVoteCast(NAME_None, false));
	TestFalse(TEXT("No votes left"), nameState.IsVoted());
	TestFalse(TEXT("Withdrawing with no votes doesn't change the state"), nameState.WithdrawVote(TEXT("A")));

	TDrsVotableState<EDrsBaseTestVoter> enumState;
	enumState.CastVote(EDrsBaseTestVoter::Second);
	TestEqual(TEXT("UENUM votes string"), enumState.GetVotesString(), FString(TEXT("Second")));

	TDrsVotableState<EDrsBaseTestPlainVoter> plainEnumState;
	plainEnumState.CastVote(EDrsBaseTestPlainVoter::Second);
	TestEqual(TEXT("Plain enum votes string"), plainEnumState.GetVotesString(), FString(TEXT("1")));

	TDrsVotableState<int32> intState;
	intState.CastVote(7);
	TestEqual(TEXT("Integer votes string"), intState.GetVotesString(), FString(TEXT("7")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrsBaseLogTest, "Drs.Base.Log", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDrsBaseLogTest::RunTest(const FString& parameters)
{
	// Warning verbosity so the automation framework sees and counts each message. Expected messages are matched
	// against the message text only, so an Exact match also checks that the macros add no prefix.
	AddExpectedMessage(TEXT("DrsLogTest plain 42 abc"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Exact, 1, false);
	DRS_LOG(LogDrsBase, Warning, "DrsLogTest plain %d %s", 42, TEXT("abc"));

	AddExpectedMessage(TEXT("DrsLogTest global"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Exact, 1, false);
	DRS_LOG_GLOBAL(Warning, "DrsLogTest global");

	// DRS_CLOG logs only when the condition is true: twice out of four.
	AddExpectedMessage(TEXT("^DrsLogTest clog [0-9]$"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Exact, 2, true);
	for (int32 index = 0; index < 4; ++index)
	{
		DRS_CLOG(index % 2 == 0, LogDrsBase, Warning, "DrsLogTest clog %d", index);
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
		DRS_LOG_ONCE(LogDrsBase, Warning, "DrsLogTest log once %d", index);
	}

	// DRS_CLOG_ONCE ignores passes with a false condition and fires the first time it is true (index 2) only.
	AddExpectedMessage(TEXT("DrsLogTest clog once 2"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Exact, 1, false);
	for (int32 index = 0; index < 5; ++index)
	{
		DRS_CLOG_ONCE(index >= 2, LogDrsBase, Warning, "DrsLogTest clog once %d", index);
	}

	// Every macro is a single statement, so it compiles in an unbraced if/else.
	AddExpectedMessage(TEXT("DrsLogTest if branch"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Exact, 1, false);
	const bool isIfBranch = parameters.IsEmpty();
	if (isIfBranch)
		DRS_LOG_ONCE(LogDrsBase, Warning, "DrsLogTest if branch");
	else
		DRS_CLOG_ONCE(true, LogDrsBase, Warning, "DrsLogTest else branch");
	if (!isIfBranch)
		DRS_LOG(LogDrsBase, Warning, "DrsLogTest else branch");
	else
		DRS_CLOG(false, LogDrsBase, Warning, "DrsLogTest else branch");

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrsBaseHandTest, "Drs.Base.Hand", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDrsBaseHandTest::RunTest(const FString& parameters)
{
	// Opposite and masks.
	TestEqual(TEXT("Opposite of left"), Drs::Opposite(EDrsHand::Left), EDrsHand::Right);
	TestEqual(TEXT("Opposite of right"), Drs::Opposite(EDrsHand::Right), EDrsHand::Left);
	TestEqual(TEXT("Left to mask"), Drs::ToMask(EDrsHand::Left), EDrsHandMask::Left);
	TestEqual(TEXT("Right to mask"), Drs::ToMask(EDrsHand::Right), EDrsHandMask::Right);
	TestEqual(TEXT("Left | Right"), EDrsHandMask::Left | EDrsHandMask::Right, EDrsHandMask::Both);

	// Contains.
	TestTrue(TEXT("Both contains left"), Drs::Contains(EDrsHandMask::Both, EDrsHand::Left));
	TestTrue(TEXT("Both contains right"), Drs::Contains(EDrsHandMask::Both, EDrsHand::Right));
	TestTrue(TEXT("Left contains left"), Drs::Contains(EDrsHandMask::Left, EDrsHand::Left));
	TestFalse(TEXT("Left contains right"), Drs::Contains(EDrsHandMask::Left, EDrsHand::Right));
	TestFalse(TEXT("None contains left"), Drs::Contains(EDrsHandMask::None, EDrsHand::Left));
	TestFalse(TEXT("None contains right"), Drs::Contains(EDrsHandMask::None, EDrsHand::Right));

	// Mask to hand: only exactly one hand converts.
	const TOptional<EDrsHand> rightMaskHand = Drs::ToHand(EDrsHandMask::Right);
	TestTrue(TEXT("Right mask has a hand"), rightMaskHand.IsSet() && rightMaskHand.GetValue() == EDrsHand::Right);
	TestFalse(TEXT("Both has no single hand"), Drs::ToHand(EDrsHandMask::Both).IsSet());
	TestFalse(TEXT("None has no single hand"), Drs::ToHand(EDrsHandMask::None).IsSet());

	// Engine enum to Drs.
	TestEqual(TEXT("Engine left to mask"), Drs::ToMask(EControllerHand::Left), EDrsHandMask::Left);
	TestEqual(TEXT("Engine right to mask"), Drs::ToMask(EControllerHand::Right), EDrsHandMask::Right);
	TestEqual(TEXT("Engine AnyHand to mask"), Drs::ToMask(EControllerHand::AnyHand), EDrsHandMask::Both);
	TestEqual(TEXT("Engine Gun to mask"), Drs::ToMask(EControllerHand::Gun), EDrsHandMask::None);
	const TOptional<EDrsHand> engineLeftHand = Drs::ToHand(EControllerHand::Left);
	TestTrue(TEXT("Engine left to hand"), engineLeftHand.IsSet() && engineLeftHand.GetValue() == EDrsHand::Left);
	TestFalse(TEXT("Engine AnyHand to hand"), Drs::ToHand(EControllerHand::AnyHand).IsSet());
	TestFalse(TEXT("Engine Gun to hand"), Drs::ToHand(EControllerHand::Gun).IsSet());

	// Drs to engine enum.
	TestEqual(TEXT("Left to engine"), Drs::ToControllerHand(EDrsHand::Left), EControllerHand::Left);
	TestEqual(TEXT("Right to engine"), Drs::ToControllerHand(EDrsHand::Right), EControllerHand::Right);
	TestEqual(TEXT("Left mask to engine"), Drs::ToControllerHand(EDrsHandMask::Left), EControllerHand::Left);
	TestEqual(TEXT("Right mask to engine"), Drs::ToControllerHand(EDrsHandMask::Right), EControllerHand::Right);
	TestEqual(TEXT("Both to engine"), Drs::ToControllerHand(EDrsHandMask::Both), EControllerHand::AnyHand);
	TestEqual(TEXT("None to engine"), Drs::ToControllerHand(EDrsHandMask::None), EControllerHand::AnyHand);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrsBaseHandLibraryTest, "Drs.Base.HandLibrary", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDrsBaseHandLibraryTest::RunTest(const FString& parameters)
{
	// The Blueprint wrappers forward to the native helpers, so this mainly checks the wrappers' own contract: failed
	// conversions return false and still assign the output.
	TestEqual(TEXT("Hand to mask"), UDrsHandLibrary::Conv_HandToHandMask(EDrsHand::Right), EDrsHandMask::Right);
	TestEqual(TEXT("Hand to engine"), UDrsHandLibrary::Conv_HandToControllerHand(EDrsHand::Right), EControllerHand::Right);
	TestEqual(TEXT("Mask to engine"), UDrsHandLibrary::HandMaskToControllerHand(EDrsHandMask::Both), EControllerHand::AnyHand);
	TestEqual(TEXT("Engine to mask"), UDrsHandLibrary::Conv_ControllerHandToHandMask(EControllerHand::AnyHand), EDrsHandMask::Both);
	TestEqual(TEXT("Opposite"), UDrsHandLibrary::OppositeHand(EDrsHand::Left), EDrsHand::Right);
	TestTrue(TEXT("Mask contains"), UDrsHandLibrary::HandMaskContains(EDrsHandMask::Both, EDrsHand::Right));
	TestFalse(TEXT("Mask does not contain"), UDrsHandLibrary::HandMaskContains(EDrsHandMask::Left, EDrsHand::Right));

	EDrsHand hand = EDrsHand::Right;
	TestTrue(TEXT("Left mask to hand"), UDrsHandLibrary::HandMaskToHand(EDrsHandMask::Left, hand));
	TestEqual(TEXT("Left mask hand"), hand, EDrsHand::Left);
	hand = EDrsHand::Right;
	TestFalse(TEXT("Both mask to hand"), UDrsHandLibrary::HandMaskToHand(EDrsHandMask::Both, hand));
	TestEqual(TEXT("Failed conversion assigns the default hand"), hand, EDrsHand::Left);

	hand = EDrsHand::Left;
	TestTrue(TEXT("Engine right to hand"), UDrsHandLibrary::ControllerHandToHand(EControllerHand::Right, hand));
	TestEqual(TEXT("Engine right hand"), hand, EDrsHand::Right);
	TestFalse(TEXT("Engine AnyHand to hand"), UDrsHandLibrary::ControllerHandToHand(EControllerHand::AnyHand, hand));
	TestEqual(TEXT("Failed engine conversion assigns the default hand"), hand, EDrsHand::Left);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
