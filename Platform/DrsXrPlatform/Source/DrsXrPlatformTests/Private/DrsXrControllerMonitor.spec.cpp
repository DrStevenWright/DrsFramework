// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
//
// FDrsXrControllerMonitor decisions, driven through a fake hand-tracking source, so no headset or OpenXR session is needed.
// The profile sequences mirror what device tests recorded on Meta (Link, Quest 3) and Pico (Business Streaming,
// PICO 4 Ultra).

#include "DrsXrControllerMonitor.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

namespace DrsXrControllerMonitorTest
{
	const FString TouchProfile(TEXT("/interaction_profiles/meta/touch_controller_plus"));
	const FString PicoProfile(TEXT("/interaction_profiles/bytedance/pico4s_controller"));
	const FString HandProfile(TEXT("/interaction_profiles/ext/hand_interaction_ext"));
	const FString NoProfile;
	constexpr float FrameTime = 1.0f / 72.0f;

	class FFakeSource : public IDrsXrControllerSource
	{
	public:

		virtual bool CanTrackHands() const override { return bCanTrackHands; }
		virtual bool IsTrackedHandJointTracked(EControllerHand hand) const override { return (hand == EControllerHand::Left) ? bLeftJointsTracked : bRightJointsTracked; }
		virtual bool IsPhysicalControllerGripTracked(EControllerHand hand) const override { return (hand == EControllerHand::Left) ? bLeftGripTracked : bRightGripTracked; }
		virtual bool AreMixedControllerModesEnabled() const override { return bMixedControllerModesEnabled; }
		
		void SetJointsTracked(bool isTracked) { bLeftJointsTracked = bRightJointsTracked = isTracked; }
		void SetGripsTracked(bool isTracked) { bLeftGripTracked = bRightGripTracked = isTracked; }

		bool bCanTrackHands = true;
		bool bLeftJointsTracked = false;
		bool bRightJointsTracked = false;
		bool bLeftGripTracked = false;
		bool bRightGripTracked = false;
		bool bMixedControllerModesEnabled = false;
	};

	// Updates the monitor for the given time, one frame at a time.
	void UpdateFor(FDrsXrControllerMonitor& monitor, float time)
	{
		for (float elapsed = 0.0f; elapsed < time; elapsed += FrameTime)
		{
			monitor.Update(FrameTime);
		}
	}
}

BEGIN_DEFINE_SPEC(FDrsXrControllerMonitorSpec, TEXT("Drs.XrPlatform.ControllerMonitor"), EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

	using EMode = EDrsXrControllerMode;

	void TestModes(const FDrsXrControllerMonitor& monitor, EMode left, EMode right)
	{
		TestTrue(TEXT("Left mode"), monitor.GetState(EControllerHand::Left).Mode == left);
		TestTrue(TEXT("Right mode"), monitor.GetState(EControllerHand::Right).Mode == right);
	}

	void TestModes(const FDrsXrControllerMonitor& monitor, EMode expected)
	{
		TestModes(monitor, expected, expected);
	}

	void TestControllerProfiles(const FDrsXrControllerMonitor& monitor, const FString& left, const FString& right)
	{
		TestEqual(TEXT("Left controller profile"), monitor.GetState(EControllerHand::Left).PhysicalControllerProfilePath, left);
		TestEqual(TEXT("Right controller profile"), monitor.GetState(EControllerHand::Right).PhysicalControllerProfilePath, right);
	}

	void TestActive(const FDrsXrControllerMonitor& monitor, bool isLeftActive, bool isRightActive)
	{
		TestEqual(TEXT("Left active"), monitor.IsPhysicalControllerActive(EControllerHand::Left), isLeftActive);
		TestEqual(TEXT("Right active"), monitor.IsPhysicalControllerActive(EControllerHand::Right), isRightActive);
	}

	void TestPresent(const FDrsXrControllerMonitor& monitor, bool isLeftPresent, bool isRightPresent)
	{
		TestEqual(TEXT("Left present"), monitor.IsControllerPresent(EControllerHand::Left), isLeftPresent);
		TestEqual(TEXT("Right present"), monitor.IsControllerPresent(EControllerHand::Right), isRightPresent);
	}

END_DEFINE_SPEC(FDrsXrControllerMonitorSpec)

void FDrsXrControllerMonitorSpec::Define()
{
	using namespace DrsXrControllerMonitorTest;

	Describe(TEXT("Classify"), [this]()
	{
		It(TEXT("classifies empty, controller and hand profiles"), [this]()
		{
			TestTrue(TEXT("Empty"), Drs::XrInteractionProfiles::DetermineControllerMode(NoProfile) == EDrsXrControllerMode::Unknown);
			TestTrue(TEXT("Touch"), Drs::XrInteractionProfiles::DetermineControllerMode(TouchProfile) == EDrsXrControllerMode::PhysicalController);
			TestTrue(TEXT("PICO"), Drs::XrInteractionProfiles::DetermineControllerMode(PicoProfile) == EDrsXrControllerMode::PhysicalController);
			TestTrue(TEXT("EXT hand"), Drs::XrInteractionProfiles::DetermineControllerMode(HandProfile) == EDrsXrControllerMode::TrackedHand);
			TestTrue(TEXT("MSFT hand"), Drs::XrInteractionProfiles::DetermineControllerMode(TEXT("/interaction_profiles/microsoft/hand_interaction")) == EDrsXrControllerMode::TrackedHand);
		});
	});

	Describe(TEXT("Mode"), [this]()
	{
		It(TEXT("enters hand tracking when both hands report a hand profile"), [this]()
		{
			FFakeSource source;
			FDrsXrControllerMonitor monitor(source);
			monitor.SetProfiles(HandProfile, HandProfile);

			TestTrue(TEXT("Changed"), monitor.Update(FrameTime));
			TestModes(monitor, EMode::TrackedHand);
			TestControllerProfiles(monitor, NoProfile, NoProfile);
		});

		It(TEXT("enters hand tracking without profiles when the joints are tracked (controllers off at launch)"), [this]()
		{
			FFakeSource source;
			FDrsXrControllerMonitor monitor(source);

			TestFalse(TEXT("No change before hands are seen"), monitor.Update(FrameTime));
			TestModes(monitor, EMode::PhysicalController);

			source.SetJointsTracked(true);
			TestTrue(TEXT("Changed"), monitor.Update(FrameTime));
			TestModes(monitor, EMode::TrackedHand);
		});

		It(TEXT("enters hand tracking without profiles when only one hand is tracked"), [this]()
		{
			FFakeSource source;
			FDrsXrControllerMonitor monitor(source);
			source.bRightJointsTracked = true;

			monitor.Update(FrameTime);
			TestModes(monitor, EMode::TrackedHand);
		});

		It(TEXT("stays on controllers when hands can't be tracked"), [this]()
		{
			FFakeSource source;
			source.bCanTrackHands = false;
			source.SetJointsTracked(true);
			FDrsXrControllerMonitor monitor(source);

			TestFalse(TEXT("No change"), monitor.Update(FrameTime));
			TestModes(monitor, EMode::PhysicalController);
		});

		It(TEXT("uses controllers for both hands while either reports a controller profile (one controller never on)"), [this]()
		{
			FFakeSource source;
			FDrsXrControllerMonitor monitor(source);
			source.bLeftJointsTracked = true;
			monitor.SetProfiles(NoProfile, PicoProfile);

			monitor.Update(FrameTime);
			TestModes(monitor, EMode::PhysicalController);
		});

		It(TEXT("ignores tracked joints while controllers are held (e.g. with mixed controller modes enabled)"), [this]()
		{
			FFakeSource source;
			FDrsXrControllerMonitor monitor(source);
			source.SetJointsTracked(true);
			monitor.SetProfiles(TouchProfile, TouchProfile);

			monitor.Update(FrameTime);
			TestModes(monitor, EMode::PhysicalController);
		});

		It(TEXT("switches between controllers and hands as the profiles change"), [this]()
		{
			FFakeSource source;
			FDrsXrControllerMonitor monitor(source);
			monitor.SetProfiles(TouchProfile, TouchProfile);
			monitor.Update(FrameTime);
			TestModes(monitor, EMode::PhysicalController);

			// Controllers put down: Meta reports empty profiles for a few hundred ms before the hand profiles.
			monitor.SetProfiles(NoProfile, NoProfile);
			TestFalse(TEXT("No change while untracked"), monitor.Update(FrameTime));
			TestModes(monitor, EMode::PhysicalController);

			monitor.SetProfiles(HandProfile, HandProfile);
			TestTrue(TEXT("Changed to hands"), monitor.Update(FrameTime));
			TestModes(monitor, EMode::TrackedHand);

			monitor.SetProfiles(TouchProfile, TouchProfile);
			TestTrue(TEXT("Changed back"), monitor.Update(FrameTime));
			TestModes(monitor, EMode::PhysicalController);
		});

		It(TEXT("keeps hand tracking through joint dropouts"), [this]()
		{
			FFakeSource source;
			FDrsXrControllerMonitor monitor(source);
			source.SetJointsTracked(true);
			monitor.Update(FrameTime);
			TestModes(monitor, EMode::TrackedHand);

			source.SetJointsTracked(false);
			TestFalse(TEXT("No change"), monitor.Update(FrameTime));
			TestModes(monitor, EMode::TrackedHand);
		});

		It(TEXT("keeps controllers when a controller loses its profile until the hand is tracked"), [this]()
		{
			FFakeSource source;
			FDrsXrControllerMonitor monitor(source);
			monitor.SetProfiles(PicoProfile, PicoProfile);
			monitor.Update(FrameTime);

			monitor.SetProfiles(NoProfile, NoProfile);
			TestFalse(TEXT("No change"), monitor.Update(FrameTime));
			TestModes(monitor, EMode::PhysicalController);

			source.SetJointsTracked(true);
			TestTrue(TEXT("Changed"), monitor.Update(FrameTime));
			TestModes(monitor, EMode::TrackedHand);
		});
	});

	Describe(TEXT("Mixed controller modes"), [this]()
	{
		It(TEXT("gives each hand its own mode"), [this]()
		{
			FFakeSource source;
			source.bMixedControllerModesEnabled = true;
			FDrsXrControllerMonitor monitor(source);
			monitor.SetProfiles(TouchProfile, HandProfile);

			TestTrue(TEXT("Changed"), monitor.Update(FrameTime));
			TestModes(monitor, EMode::PhysicalController, EMode::TrackedHand);
		});

		It(TEXT("uses joint tracking for a hand without a profile"), [this]()
		{
			FFakeSource source;
			source.bMixedControllerModesEnabled = true;
			source.bRightJointsTracked = true;
			FDrsXrControllerMonitor monitor(source);
			monitor.SetProfiles(TouchProfile, NoProfile);

			monitor.Update(FrameTime);
			TestModes(monitor, EMode::PhysicalController, EMode::TrackedHand);
		});

		It(TEXT("keeps a hand's mode while it has no profile"), [this]()
		{
			FFakeSource source;
			source.bMixedControllerModesEnabled = true;
			FDrsXrControllerMonitor monitor(source);
			monitor.SetProfiles(TouchProfile, HandProfile);
			monitor.Update(FrameTime);

			monitor.SetProfiles(TouchProfile, NoProfile);
			TestFalse(TEXT("No change"), monitor.Update(FrameTime));
			TestModes(monitor, EMode::PhysicalController, EMode::TrackedHand);
		});

		It(TEXT("collapses again when mixed controller modes are no longer enabled"), [this]()
		{
			FFakeSource source;
			source.bMixedControllerModesEnabled = true;
			FDrsXrControllerMonitor monitor(source);
			monitor.SetProfiles(TouchProfile, HandProfile);
			monitor.Update(FrameTime);

			source.bMixedControllerModesEnabled = false;
			TestTrue(TEXT("Changed"), monitor.Update(FrameTime));
			TestModes(monitor, EMode::PhysicalController);
		});
	});

	Describe(TEXT("Notification"), [this]()
	{
		It(TEXT("reports a new controller profile even when the mode is unchanged"), [this]()
		{
			FFakeSource source;
			FDrsXrControllerMonitor monitor(source);
			monitor.SetProfiles(TouchProfile, TouchProfile);
			TestTrue(TEXT("First controller profile"), monitor.Update(FrameTime));
			TestControllerProfiles(monitor, TouchProfile, TouchProfile);
			TestFalse(TEXT("Nothing new"), monitor.Update(FrameTime));

			const FString otherProfile(TEXT("/interaction_profiles/oculus/touch_controller"));
			monitor.SetProfiles(otherProfile, otherProfile);
			TestTrue(TEXT("Different controller"), monitor.Update(FrameTime));
			TestControllerProfiles(monitor, otherProfile, otherProfile);
		});

		It(TEXT("keeps each hand's own controller profile"), [this]()
		{
			FFakeSource source;
			FDrsXrControllerMonitor monitor(source);
			monitor.SetProfiles(NoProfile, PicoProfile);
			TestTrue(TEXT("Right controller profile"), monitor.Update(FrameTime));
			TestControllerProfiles(monitor, NoProfile, PicoProfile);

			monitor.SetProfiles(TouchProfile, PicoProfile);
			TestTrue(TEXT("Left controller profile"), monitor.Update(FrameTime));
			TestControllerProfiles(monitor, TouchProfile, PicoProfile);
		});

		It(TEXT("keeps the controller profile while hands are tracked"), [this]()
		{
			FFakeSource source;
			FDrsXrControllerMonitor monitor(source);
			monitor.SetProfiles(TouchProfile, TouchProfile);
			monitor.Update(FrameTime);
			monitor.SetProfiles(HandProfile, HandProfile);
			monitor.Update(FrameTime);

			TestControllerProfiles(monitor, TouchProfile, TouchProfile);
		});

		It(TEXT("does not report a profile change that leaves the mode unchanged"), [this]()
		{
			FFakeSource source;
			FDrsXrControllerMonitor monitor(source);
			monitor.SetProfiles(HandProfile, HandProfile);
			monitor.Update(FrameTime);

			// Hand profile to empty (e.g. while a controller wakes up) is only logged.
			monitor.SetProfiles(NoProfile, NoProfile);
			TestFalse(TEXT("No change"), monitor.Update(FrameTime));
		});

		It(TEXT("forgets everything on reset"), [this]()
		{
			FFakeSource source;
			FDrsXrControllerMonitor monitor(source);
			monitor.SetProfiles(HandProfile, TouchProfile);
			monitor.Update(FrameTime);
			monitor.Reset();

			TestModes(monitor, EMode::PhysicalController);
			TestControllerProfiles(monitor, NoProfile, NoProfile);
			TestTrue(TEXT("No left profile"), monitor.GetState(EControllerHand::Left).ProfilePath.IsEmpty());
			TestFalse(TEXT("No pending change"), monitor.Update(FrameTime));
		});
	});

	Describe(TEXT("Active"), [this]()
	{
		It(TEXT("activates a controller once its grip is tracked"), [this]()
		{
			FFakeSource source;
			FDrsXrControllerMonitor monitor(source);
			monitor.SetProfiles(TouchProfile, TouchProfile);

			monitor.Update(FrameTime);
			TestActive(monitor, false, false);
			TestPresent(monitor, false, false);

			source.SetGripsTracked(true);
			monitor.Update(FrameTime);
			TestActive(monitor, true, true);
			TestPresent(monitor, true, true);
		});

		It(TEXT("keeps a hand without a powered controller inactive despite its profile (PICO standalone)"), [this]()
		{
			FFakeSource source;
			FDrsXrControllerMonitor monitor(source);
			monitor.SetProfiles(PicoProfile, PicoProfile);
			source.bRightGripTracked = true;

			monitor.Update(FrameTime);
			TestActive(monitor, false, true);
			TestPresent(monitor, false, true);
		});

		It(TEXT("keeps a controller active after its grip loses tracking"), [this]()
		{
			FFakeSource source;
			FDrsXrControllerMonitor monitor(source);
			monitor.SetProfiles(PicoProfile, PicoProfile);
			source.SetGripsTracked(true);
			monitor.Update(FrameTime);

			source.SetGripsTracked(false);
			UpdateFor(monitor, 2.0f);
			TestActive(monitor, true, true);
		});

		It(TEXT("stays active through a short gap without a profile (Meta switching to hands)"), [this]()
		{
			FFakeSource source;
			FDrsXrControllerMonitor monitor(source);
			monitor.SetProfiles(TouchProfile, TouchProfile);
			source.SetGripsTracked(true);
			monitor.Update(FrameTime);

			monitor.SetProfiles(NoProfile, NoProfile);
			UpdateFor(monitor, 0.5f);
			TestActive(monitor, true, true);

			monitor.SetProfiles(TouchProfile, TouchProfile);
			UpdateFor(monitor, 0.5f);
			TestActive(monitor, true, true);
		});

		It(TEXT("becomes inactive once the controller profile has been gone for the delay"), [this]()
		{
			FFakeSource source;
			FDrsXrControllerMonitor monitor(source);
			monitor.SetProfiles(TouchProfile, TouchProfile);
			source.SetGripsTracked(true);
			monitor.Update(FrameTime);

			monitor.SetProfiles(TouchProfile, NoProfile);
			UpdateFor(monitor, FDrsXrControllerMonitor::PhysicalControllerInactiveDelay - 0.1f);
			TestActive(monitor, true, true);

			UpdateFor(monitor, 0.2f);
			TestActive(monitor, true, false);
			TestPresent(monitor, true, false);
		});

		It(TEXT("requires the grip to be tracked again when a controller becomes active again"), [this]()
		{
			FFakeSource source;
			FDrsXrControllerMonitor monitor(source);
			monitor.SetProfiles(TouchProfile, TouchProfile);
			source.SetGripsTracked(true);
			monitor.Update(FrameTime);

			monitor.SetProfiles(NoProfile, NoProfile);
			source.SetGripsTracked(false);
			UpdateFor(monitor, FDrsXrControllerMonitor::PhysicalControllerInactiveDelay + 0.1f);
			TestActive(monitor, false, false);

			monitor.SetProfiles(TouchProfile, TouchProfile);
			monitor.Update(FrameTime);
			TestActive(monitor, false, false);

			source.SetGripsTracked(true);
			monitor.Update(FrameTime);
			TestActive(monitor, true, true);
		});

		It(TEXT("forgets active controllers on reset"), [this]()
		{
			FFakeSource source;
			FDrsXrControllerMonitor monitor(source);
			monitor.SetProfiles(TouchProfile, TouchProfile);
			source.SetGripsTracked(true);
			monitor.Update(FrameTime);
			monitor.Reset();

			TestActive(monitor, false, false);
			TestPresent(monitor, false, false);
		});
	});

	Describe(TEXT("Present"), [this]()
	{
		It(TEXT("reports a tracked hand once it has been seen, and keeps it through dropouts"), [this]()
		{
			FFakeSource source;
			FDrsXrControllerMonitor monitor(source);
			monitor.SetProfiles(HandProfile, HandProfile);
			monitor.Update(FrameTime);
			TestModes(monitor, EMode::TrackedHand);
			TestPresent(monitor, false, false);

			source.bLeftJointsTracked = true;
			monitor.Update(FrameTime);
			TestPresent(monitor, true, false);

			source.bLeftJointsTracked = false;
			monitor.Update(FrameTime);
			TestPresent(monitor, true, false);
		});

		It(TEXT("stays present while switching from controller to hand"), [this]()
		{
			FFakeSource source;
			FDrsXrControllerMonitor monitor(source);
			monitor.SetProfiles(TouchProfile, TouchProfile);
			source.SetGripsTracked(true);
			monitor.Update(FrameTime);

			// Meta: the controller profile drops to empty, then the hand profile appears and the hands are tracked.
			monitor.SetProfiles(NoProfile, NoProfile);
			UpdateFor(monitor, 0.3f);
			TestPresent(monitor, true, true);

			monitor.SetProfiles(HandProfile, HandProfile);
			source.SetJointsTracked(true);
			UpdateFor(monitor, 1.0f);
			TestModes(monitor, EMode::TrackedHand);
			TestActive(monitor, false, false);
			TestPresent(monitor, true, true);
		});

		It(TEXT("forgets a seen tracked hand when switching back to controllers"), [this]()
		{
			FFakeSource source;
			FDrsXrControllerMonitor monitor(source);
			source.SetJointsTracked(true);
			monitor.Update(FrameTime);
			TestPresent(monitor, true, true);

			// Only the right controller is switched on.
			monitor.SetProfiles(TouchProfile, TouchProfile);
			source.bRightGripTracked = true;
			monitor.Update(FrameTime);
			TestModes(monitor, EMode::PhysicalController);
			TestPresent(monitor, false, true);
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
