// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsXrPlatform/DrsXrControllerType.h"

#include "Delegates/Delegate.h"

/** Native events for XR device state changes. Local-only: holds no replicated state. */
class DRSXRPLATFORM_API FDrsXrDelegates
{
public:
	/** The HMD was put on or taken off. */
	using FHmdWornStateChanged = TMulticastDelegate<void(bool isWorn)>;
	static FHmdWornStateChanged OnHmdWornStateChanged;

	/** The system menu was opened or closed. */
	using FSystemMenuStateChanged = TMulticastDelegate<void(bool isSystemMenuOpen)>;
	static FSystemMenuStateChanged OnSystemMenuStateChanged;

	/** The app gained or lost XR focus (the HMD was taken off, or the system menu opened or closed). */
	using FFocusStateChanged = TMulticastDelegate<void(bool hasFocus)>;
	static FFocusStateChanged OnXrFocusStateChanged;

	/** The controller type was first identified, or changed (including between physical controllers and tracked hands). */
	using FControllerTypeChanged = TMulticastDelegate<void(const FDrsXrControllerType& controllerType)>;
	static FControllerTypeChanged OnControllerTypeChanged;
};
