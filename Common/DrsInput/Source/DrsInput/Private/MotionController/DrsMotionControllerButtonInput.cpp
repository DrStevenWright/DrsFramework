// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsInput/MotionController/DrsMotionControllerButtonInput.h"

namespace Drs::Input
{
	void FMotionControllerButtonInput::Update(float deltaTime)
	{
		TimeInCurrentState += deltaTime;
	
		if (bPressRegistered)
		{
			if (!bIsDown)
			{
				bIsDown = true;
				bEverPressed = true;
				TimeInCurrentState = 0.0f;
			}

			bPressRegistered = false;
		}
		else if (bReleaseRegistered)
		{
			if (bIsDown)
			{
				bIsDown = false;
				TimeInCurrentState = 0.0f;
			}

			bReleaseRegistered = false;
		}
	}

	void FMotionControllerButtonInput::Reset()
	{
		bPressRegistered	= false;
		bReleaseRegistered	= false;
		bIsDown				= false;
		bEverPressed		= false;
		TimeInCurrentState	= 0.0f;
	}
}
