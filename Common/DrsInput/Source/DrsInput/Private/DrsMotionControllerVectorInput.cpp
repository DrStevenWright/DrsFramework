// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsMotionControllerVectorInput.h"

void FDrsMotionControllerVectorInput::Update(float deltaTime)
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

void FDrsMotionControllerVectorInput::Reset()
{
	Vector = FVector2D::ZeroVector;
	Magnitude = 0.0f;
	bIdle = false;
	IdleTime = 0.0f;
}
