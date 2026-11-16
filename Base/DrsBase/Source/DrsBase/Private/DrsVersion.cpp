// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsVersion.h"

FString Drs::GetFrameworkVersionString()
{
	return FString::Printf(TEXT("%d.%d.%d"), FrameworkVersionMajor, FrameworkVersionMinor, FrameworkVersionPatch);
}
