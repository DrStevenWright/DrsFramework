// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsBase/DrsHand.h"
#include "DrsInput/Contexts/DrsInputContextSubsystem.h"
#include "DrsInput/DrsInputAction.h"
#include "DrsInput/DrsInputMappingContext.h"
#include "DrsInput/MotionController/DrsMotionControllerButtonInput.h"
#include "DrsInput/MotionController/DrsMotionControllerKeys.h"
#include "DrsInput/MotionController/DrsMotionControllerVectorInput.h"

#include "Engine/LocalPlayer.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"
#include "UObject/UnrealType.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, DrsInputTests)

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// ActionID has no setter (it is set in the asset), so tests set it through reflection.
	UDrsInputAction* MakeAction(UObject* outer, FName assetName, FName actionID)
	{
		UDrsInputAction* action = NewObject<UDrsInputAction>(outer, assetName);
		if (FNameProperty* property = CastField<FNameProperty>(UDrsInputAction::StaticClass()->FindPropertyByName(TEXT("ActionID"))))
		{
			property->SetPropertyValue_InContainer(action, actionID);
		}
		return action;
	}
}

// The key names are serialised into mapping context assets and matched against action IDs, so they must not change.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrsInputMotionControllerKeysTest, "Drs.Input.MotionControllerKeys", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDrsInputMotionControllerKeysTest::RunTest(const FString& parameters)
{
	for (const EDrsHand hand : { EDrsHand::Left, EDrsHand::Right })
	{
		const FString side = (hand == EDrsHand::Left) ? TEXT("Left") : TEXT("Right");
		const FDrsMotionControllerHandKeys& keys = Drs::MotionControllerKeys::Get(hand);

		const TPair<const FDrsMotionInputKey*, const TCHAR*> expectedKeys[] =
		{
			{ &keys.Menu, TEXT("Menu_Press") },
			{ &keys.Grip, TEXT("Grip_Press") },
			{ &keys.GripAxis, TEXT("Grip_Axis") },
			{ &keys.Trigger, TEXT("Trigger_Press") },
			{ &keys.TriggerTouch, TEXT("Trigger_Touch") },
			{ &keys.TriggerAxis, TEXT("Trigger_Axis") },
			{ &keys.Button1, TEXT("Button1_Press") },
			{ &keys.Button1Touch, TEXT("Button1_Touch") },
			{ &keys.Button2, TEXT("Button2_Press") },
			{ &keys.Button2Touch, TEXT("Button2_Touch") },
			{ &keys.Thumb, TEXT("Thumb_Press") },
			{ &keys.ThumbTouch, TEXT("Thumb_Touch") },
			{ &keys.ThumbstickX, TEXT("Thumbstick_X") },
			{ &keys.ThumbstickY, TEXT("Thumbstick_Y") },
			{ &keys.Thumbstick2D, TEXT("Thumbstick_2D") },
		};

		for (const TPair<const FDrsMotionInputKey*, const TCHAR*>& expected : expectedKeys)
		{
			const FString name = side + TEXT("_") + expected.Value;
			TestEqual(*FString::Printf(TEXT("%s key name"), *name), expected.Key->InputKey.GetFName(), FName(*name));
			TestEqual(*FString::Printf(TEXT("%s action ID matches the key name"), *name), expected.Key->ActionID, expected.Key->InputKey.GetFName());
			TestTrue(*FString::Printf(TEXT("%s is registered with the engine"), *name), EKeys::GetKeyDetails(expected.Key->InputKey).IsValid());
		}

		const TSharedPtr<FKeyDetails> stickDetails = EKeys::GetKeyDetails(keys.Thumbstick2D.InputKey);
		TestTrue(*FString::Printf(TEXT("%s thumbstick 2D is a 2D axis"), *side), stickDetails.IsValid() && stickDetails->IsAxis2D());
	}

	TestNotEqual(TEXT("Hands have different keys"), Drs::MotionControllerKeys::Get(EDrsHand::Left).Trigger.InputKey, Drs::MotionControllerKeys::Get(EDrsHand::Right).Trigger.InputKey);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrsInputButtonInputTest, "Drs.Input.ButtonInput", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDrsInputButtonInputTest::RunTest(const FString& parameters)
{
	FDrsMotionControllerButtonInput button;
	TestFalse(TEXT("Not down initially"), button.IsDown());
	TestFalse(TEXT("Never pressed initially"), button.WasEverPressed());

	button.SetPressed();
	TestFalse(TEXT("A registered press doesn't apply before Update"), button.IsDown());
	TestTrue(TEXT("It is down next frame"), button.IsDownNextFrame());
	button.Update(0.1f);
	TestTrue(TEXT("Pressed on the Update after the press"), button.IsPressed());
	TestTrue(TEXT("Down"), button.IsDown());
	TestFalse(TEXT("Not held on the first frame"), button.IsHeld());
	TestTrue(TEXT("Was pressed"), button.WasEverPressed());

	button.Update(0.1f);
	TestFalse(TEXT("Pressed lasts one frame"), button.IsPressed());
	TestTrue(TEXT("Held from the second frame"), button.IsHeld());
	TestEqual(TEXT("Held time"), button.GetHeldTime(), 0.1f);

	button.SetReleased();
	TestFalse(TEXT("It is up next frame"), button.IsDownNextFrame());
	button.Update(0.1f);
	TestTrue(TEXT("Released on the Update after the release"), button.IsReleased());
	TestTrue(TEXT("Up"), button.IsUp());
	TestEqual(TEXT("No held time while up"), button.GetHeldTime(), 0.0f);
	button.Update(0.25f);
	TestFalse(TEXT("Released lasts one frame"), button.IsReleased());
	TestEqual(TEXT("Up time"), button.GetUpTime(), 0.25f);

	// Only the latest change registered between two Updates counts.
	FDrsMotionControllerButtonInput tap;
	tap.SetPressed();
	tap.SetReleased();
	tap.Update(0.1f);
	TestFalse(TEXT("A press and release in one frame cancel out"), tap.IsDown() || tap.IsPressed() || tap.WasEverPressed());

	button.SetPressed();
	button.Update(0.1f);
	button.SetReleased();
	button.SetPressed();
	button.Update(0.1f);
	TestTrue(TEXT("A release then a press in one frame leaves the button down"), button.IsDown());

	button.Reset();
	TestFalse(TEXT("Reset clears the state"), button.IsDown() || button.WasEverPressed());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrsInputVectorInputTest, "Drs.Input.VectorInput", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDrsInputVectorInputTest::RunTest(const FString& parameters)
{
	FDrsMotionControllerVectorInput stick;
	stick.Update(0.1f);
	TestTrue(TEXT("A centred stick is idle"), stick.IsIdle());
	TestTrue(TEXT("Just idle on the frame it became idle"), stick.IsJustIdle());
	stick.Update(0.5f);
	TestFalse(TEXT("No longer just idle"), stick.IsJustIdle());
	TestEqual(TEXT("Idle time"), stick.GetIdleTime(), 0.5f);

	stick.SetVector(FVector2D(0.6, 0.8));
	TestTrue(TEXT("The vector takes effect only on Update"), stick.IsIdle());
	stick.Update(0.1f);
	TestFalse(TEXT("Deflected stick is not idle"), stick.IsIdle());
	TestEqual(TEXT("Deflection is the vector's length"), stick.GetDeflection(), 1.0f, 0.001f);
	TestEqual(TEXT("No idle time while deflected"), stick.GetIdleTime(), 0.0f);

	stick.SetX(0.05f);
	stick.SetY(0.0f);
	stick.Update(0.1f);
	TestTrue(TEXT("Small deflection is idle"), stick.IsIdle());
	TestTrue(TEXT("Just idle again"), stick.IsJustIdle());

	stick.Reset();
	TestEqual(TEXT("Reset clears the vector"), stick.GetVector(), FVector2D::ZeroVector);
	TestFalse(TEXT("Reset clears idle"), stick.IsIdle());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrsInputContextReasonsTest, "Drs.Input.ContextReasons", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDrsInputContextReasonsTest::RunTest(const FString& parameters)
{
	// A local player subsystem must be created inside a local player. The reasons don't use the player, so the class
	// default object serves as the outer.
	UDrsInputContextSubsystem* reasons = NewObject<UDrsInputContextSubsystem>(GetMutableDefault<ULocalPlayer>());
	UInputMappingContext* contextA = NewObject<UInputMappingContext>(GetTransientPackage());
	const FName reasonOne(TEXT("One"));
	const FName reasonTwo(TEXT("Two"));

	TestTrue(TEXT("Removing a reason from an unknown context says it has none left"), reasons->RemoveReason(contextA, reasonOne));
	TestFalse(TEXT("No reason yet"), reasons->HasReason(contextA, reasonOne));

	reasons->AddReason(contextA, reasonOne);
	reasons->AddReason(contextA, reasonTwo);
	reasons->AddReason(contextA, reasonTwo);
	TestTrue(TEXT("Has reason one"), reasons->HasReason(contextA, reasonOne));
	TestEqual(TEXT("A repeated reason counts once"), reasons->GetReasons(contextA).Num(), 2);

	TestFalse(TEXT("A reason is left"), reasons->RemoveReason(contextA, reasonOne));
	TestFalse(TEXT("Reason one is gone"), reasons->HasReason(contextA, reasonOne));
	TestTrue(TEXT("Reason two is still there"), reasons->HasReason(contextA, reasonTwo));
	TestTrue(TEXT("No reasons left"), reasons->RemoveReason(contextA, reasonTwo));
	TestEqual(TEXT("Reasons string is empty"), reasons->GetReasonsString(contextA), FString());

	// Contexts are told apart by object, not by name.
	UObject* outerOne = NewObject<UInputMappingContext>(GetTransientPackage());
	UObject* outerTwo = NewObject<UInputMappingContext>(GetTransientPackage());
	const UInputMappingContext* sameNameOne = NewObject<UInputMappingContext>(outerOne, TEXT("IMC_Same"));
	const UInputMappingContext* sameNameTwo = NewObject<UInputMappingContext>(outerTwo, TEXT("IMC_Same"));
	reasons->AddReason(sameNameOne, reasonOne);
	TestFalse(TEXT("A context with the same name has its own reasons"), reasons->HasReason(sameNameTwo, reasonOne));

	// Null contexts are ignored.
	reasons->AddReason(nullptr, reasonOne);
	TestFalse(TEXT("No reasons for a null context"), reasons->HasReason(nullptr, reasonOne));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrsInputActionLookupTest, "Drs.Input.ActionLookup", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDrsInputActionLookupTest::RunTest(const FString& parameters)
{
	UObject* outer = NewObject<UInputMappingContext>(GetTransientPackage());
	UDrsInputAction* leftAction = MakeAction(outer, TEXT("IA_LeftThing"), TEXT("Left_Thing"));
	UDrsInputAction* rightAction = MakeAction(outer, TEXT("IA_RightThing"), TEXT("Right_Thing"));

	UDrsInputMappingContext* leftContext = NewObject<UDrsInputMappingContext>(outer, TEXT("IMC_Left"));
	UDrsInputMappingContext* rightContext = NewObject<UDrsInputMappingContext>(outer, TEXT("IMC_Right"));
	leftContext->MapKey(leftAction, EKeys::A);
	rightContext->MapKey(rightAction, EKeys::B);
	rightContext->MapKey(rightAction, EKeys::C);

	TestEqual(TEXT("Context finds its action by ID"), leftContext->FindActionByID(TEXT("Left_Thing")), static_cast<const UInputAction*>(leftAction));
	TestNull(TEXT("Context doesn't find an unknown ID"), leftContext->FindActionByID(TEXT("Nope")));
	TestEqual(TEXT("Context finds its action by asset name"), leftContext->FindActionByName(TEXT("IA_LeftThing")), static_cast<const UInputAction*>(leftAction));
	TestEqual(TEXT("Every key mapped to an ID is found"), rightContext->FindKeysForActionID(TEXT("Right_Thing")).Num(), 2);

	FDrsInputContextDefinitionTwoHands definition;
	definition.LeftHandContext = leftContext;
	definition.RightHandContext = rightContext;

	TestEqual(TEXT("Left action found through the definition"), definition.FindActionByID(TEXT("Left_Thing")), static_cast<const UInputAction*>(leftAction));
	// The right-hand fallback matches by ID, and the asset name differs from the ID.
	TestEqual(TEXT("Right-only action found by ID through the definition"), definition.FindActionByID(TEXT("Right_Thing")), static_cast<const UInputAction*>(rightAction));
	TestNull(TEXT("Unknown ID not found through the definition"), definition.FindActionByID(TEXT("Nope")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
