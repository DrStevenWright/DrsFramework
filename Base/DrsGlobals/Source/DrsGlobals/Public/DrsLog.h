// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

/** Framework-wide log category, for messages that belong to no single module. */
DRSGLOBALS_API DECLARE_LOG_CATEGORY_EXTERN(LogDrs, Log, All);

/**
 * UE_LOG with the calling function's name prepended.
 * Usage: DRS_LOG(LogDrsXrPlatform, Warning, TEXT("No HMD found (%d)"), Value);
 */
#define DRS_LOG(CategoryName, Verbosity, Format, ...) \
	UE_LOG(CategoryName, Verbosity, TEXT("%hs: ") Format, __FUNCTION__, ##__VA_ARGS__)

/** DRS_LOG into the framework-wide LogDrs category. */
#define DRS_LOG_GLOBAL(Verbosity, Format, ...) DRS_LOG(LogDrs, Verbosity, Format, ##__VA_ARGS__)
