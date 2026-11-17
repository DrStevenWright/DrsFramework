// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsMotionControllerButtonInput.h"

void FDrsMotionControllerButtonInput::Update(float deltaTime)
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

void FDrsMotionControllerButtonInput::Reset()
{
	bPressRegistered	= false;
	bReleaseRegistered	= false;
	bIsDown				= false;
	bEverPressed		= false;
	TimeInCurrentState	= 0.0f;
}
