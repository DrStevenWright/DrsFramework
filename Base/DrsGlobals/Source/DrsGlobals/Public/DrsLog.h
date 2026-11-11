// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include <atomic>

/** Framework-wide log category, for messages that belong to no single module. */
DRSGLOBALS_API DECLARE_LOG_CATEGORY_EXTERN(LogDrs, Log, All);

/**
 * UE_LOG that wraps the format string in TEXT() itself, so pass a plain string literal.
 * Usage: DRS_LOG(LogDrsXrPlatform, Warning, "No HMD found (%d)", value);
 */
#define DRS_LOG(CategoryName, Verbosity, Format, ...) \
	do \
	{ \
		UE_LOG(CategoryName, Verbosity, TEXT(Format), ##__VA_ARGS__); \
	} \
	while (0)

/**
 * UE_CLOG that wraps the format string in TEXT() itself. Like UE_CLOG, the condition is only evaluated when the
 * category is active at that verbosity.
 * Usage: DRS_CLOG(!isTracked, LogDrsXrGame, Verbose, "Hand %d not tracked", hand);
 */
#define DRS_CLOG(Condition, CategoryName, Verbosity, Format, ...) \
	do \
	{ \
		UE_CLOG(Condition, CategoryName, Verbosity, TEXT(Format), ##__VA_ARGS__); \
	} \
	while (0)

#if NO_LOGGING

// With NO_LOGGING only Fatal messages are logged, and they don't return, so "once" needs no state.
#define DRS_LOG_ONCE(CategoryName, Verbosity, Format, ...) DRS_LOG(CategoryName, Verbosity, Format, ##__VA_ARGS__)
#define DRS_CLOG_ONCE(Condition, CategoryName, Verbosity, Format, ...) DRS_CLOG(Condition, CategoryName, Verbosity, Format, ##__VA_ARGS__)

#else

/**
 * DRS_LOG that logs only the first time this call site is reached (thread-safe).
 * Calls made while the category is inactive at that verbosity don't count.
 * Usage: DRS_LOG_ONCE(LogDrsCore, Warning, "No XR system (%d)", value);
 */
#define DRS_LOG_ONCE(CategoryName, Verbosity, Format, ...) \
	do \
	{ \
		static std::atomic<bool> drsLogOnceHasFired(false); \
		UE_CLOG(!drsLogOnceHasFired.exchange(true, std::memory_order_relaxed), CategoryName, Verbosity, TEXT(Format), ##__VA_ARGS__); \
	} \
	while (0)

/**
 * DRS_CLOG that logs only the first time the condition is true at this call site (thread-safe).
 * Calls with a false condition don't count. The condition is only evaluated when the category is active.
 * Usage: DRS_CLOG_ONCE(!isTracked, LogDrsXrGame, Warning, "Hand %d lost tracking", hand);
 */
#define DRS_CLOG_ONCE(Condition, CategoryName, Verbosity, Format, ...) \
	do \
	{ \
		static std::atomic<bool> drsLogOnceHasFired(false); \
		UE_CLOG((Condition) && !drsLogOnceHasFired.exchange(true, std::memory_order_relaxed), CategoryName, Verbosity, TEXT(Format), ##__VA_ARGS__); \
	} \
	while (0)

#endif // NO_LOGGING

/** DRS_LOG into the framework-wide LogDrs category. */
#define DRS_LOG_GLOBAL(Verbosity, Format, ...) DRS_LOG(LogDrs, Verbosity, Format, ##__VA_ARGS__)
