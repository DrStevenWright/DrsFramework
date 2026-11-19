// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

namespace Drs::XrArbiter
{
	/**
	 * A value in the struct text that config arrays hold, such as (Id="Meta",Keywords=("Meta","Oculus")). It is a plain
	 * value (a quoted or bare token), a list of values, or a set of named fields. The arbiter reads its vendor and runtime
	 * tables at EarliestPossible, before reflection can import text, so it parses them with this. It reads what the editor
	 * writes (ExportText): quoted strings with backslash escapes, bare tokens, and nesting.
	 */
	struct DRSXRARBITER_API FConfigText
	{
		/** Parses one value. Unset (with a warning logged) when the text is malformed. */
		static TOptional<FConfigText> Parse(const FString& text);

		/** A field's value, or null when there is no such field. Field names are not case-sensitive. */
		const FConfigText* Find(const TCHAR* fieldName) const;

		/** A field's text, or an empty string. */
		FString GetString(const TCHAR* fieldName) const;
		/** A field's text as a name, or None. */
		FName GetName(const TCHAR* fieldName) const;
		/** A field's list of texts. A plain value counts as a list of one. */
		TArray<FString> GetStrings(const TCHAR* fieldName) const;
		/** A field's list of texts as names. */
		TArray<FName> GetNames(const TCHAR* fieldName) const;
		/** A field's list of values. A single value counts as a list of one. */
		TArray<FConfigText> GetItems(const TCHAR* fieldName) const;

		/** The text of a plain value, unquoted and unescaped. */
		FString Text;
		/** The values of a list, in order. */
		TArray<FConfigText> Items;
		/** The fields of a struct, in order. */
		TArray<TPair<FString, FConfigText>> Fields;
		/** Whether this was written in parentheses (a list or a struct, possibly empty). */
		bool bIsGroup = false;
	};
}
