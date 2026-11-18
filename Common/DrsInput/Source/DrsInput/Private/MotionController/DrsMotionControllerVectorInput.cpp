// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsInput/MotionController/DrsMotionControllerVectorInput.h"

namespace Drs::Input
{
	void FMotionControllerVectorInput::Update(float deltaTime)
	{
		Magnitude = Vector.Size();
	
		const bool isIdle = (Magnitude <= IdleThreshold);
		if (isIdle != bIdle)
		{
			bIdle = isIdle;
			IdleTime = 0.0f;
		}
		else if (bIdle)
		{
			IdleTime += deltaTime;
		}
	}

	void FMotionControllerVectorInput::Reset()
	{
		Vector = FVector2D::ZeroVector;
		Magnitude = 0.0f;
		bIdle = false;
		IdleTime = 0.0f;
	}
}
