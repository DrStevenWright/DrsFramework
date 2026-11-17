// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
#include "CoreUserInterfaceSettings.h"

#include "XrUtilitiesFunctionLibrary.h"


void UCoreUserInterfaceSettings::SetUIInputScrollSpeedVR(float newScrollSpeed)
{
	UIInputScrollSpeedVR = newScrollSpeed;
	ApplyInputScrollSpeedSetting();
}


void UCoreUserInterfaceSettings::SetUIInputScrollSpeedNonVR(float newScrollSpeed)
{
	
	UIInputScrollSpeedNonVR = newScrollSpeed;
	ApplyInputScrollSpeedSetting();
}


void UCoreUserInterfaceSettings::ApplyInputScrollSpeedSetting()
{
#if WITH_EDITOR
	// Skip if the game is not in an active play mode (can't determine vr mode).
	if (GWorld == nullptr || !GWorld->IsGameWorld())
	{
		return;
	}
#endif

	// Modify Slate.GlobalScrollAmount cvar based on the current VR or Non-VR mode.
	static auto slateGlobalScrollAmount = IConsoleManager::Get().FindConsoleVariable(TEXT("Slate.GlobalScrollAmount"));
	if (slateGlobalScrollAmount)
	{
		const float useInputScrollSpeed = XrUtils::IsNonVrMode() ? UIInputScrollSpeedNonVR : UIInputScrollSpeedVR;
		if (useInputScrollSpeed != slateGlobalScrollAmount->GetFloat())
		{
			slateGlobalScrollAmount->Set(useInputScrollSpeed, ECVF_SetByCode);
		}
	}
}