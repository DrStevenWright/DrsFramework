// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

namespace Drs
{
	/** Framework version, kept in step with VersionName in the framework's .uplugin files. */
	inline constexpr int32 FrameworkVersionMajor = 0;
	inline constexpr int32 FrameworkVersionMinor = 1;
	inline constexpr int32 FrameworkVersionPatch = 0;

	/** Returns the framework version as "Major.Minor.Patch". */
	DRSGLOBALS_API FString GetFrameworkVersionString();
}
