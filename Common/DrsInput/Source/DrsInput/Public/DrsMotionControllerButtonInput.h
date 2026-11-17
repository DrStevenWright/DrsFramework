// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

/**
 * Per-frame state tracker for a digital button. Register presses and releases as they arrive (SetPressed/SetReleased),
 * then call Update once per frame. The registered change takes effect on that Update, so Pressed/Released are true for
 * exactly one frame.
 */
struct DRSINPUT_API FDrsMotionControllerButtonInput
{
	void	Update(float deltaTime);
	void	Reset();
	
	void	SetPressed()			{ bPressRegistered = true; bReleaseRegistered = false; }
	void	SetReleased()			{ bReleaseRegistered = true; bPressRegistered = false; }
	
	bool	IsPressed() const		{ return bIsDown && TimeInCurrentState == 0.0f; }	// Only true for one frame.
	bool	IsReleased() const		{ return !bIsDown && TimeInCurrentState == 0.0f; }	// Only true for one frame.
	bool	IsHeld() const			{ return bIsDown && TimeInCurrentState > 0.0f; }	// True from the 2nd frame that the button is kept down.
	bool	IsDown() const			{ return bIsDown; }
	bool	IsUp() const			{ return !bIsDown; }

	bool	WasEverPressed() const	{ return bEverPressed; }
	float	GetHeldTime() const		{ return bIsDown ? TimeInCurrentState : 0.0f; }
	float	GetUpTime() const		{ return !bIsDown ? TimeInCurrentState : 0.0f; }
	float	GetReleasedTime() const	{ return bEverPressed ? GetUpTime() : TNumericLimits<float>::Max(); }

	bool	IsDownNextFrame() const	{ return bIsDown ? !bReleaseRegistered : bPressRegistered; }
	
private:

	bool	bPressRegistered	= false;
	bool	bReleaseRegistered	= false;
	bool	bIsDown				= false;
	bool	bEverPressed		= false;
	float	TimeInCurrentState	= 0.0f;
};
