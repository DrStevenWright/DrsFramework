// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrArbiter/DrsXrConfigText.h"

#include "DrsBase/DrsLog.h"
#include "DrsXrArbiter/DrsXrArbiter.h"

namespace Drs::XrArbiter
{
	namespace
	{
		class FParser
		{
		public:
			explicit FParser(const FString& text) : Text(text) {}

			bool ParseValue(FConfigText& outValue)
			{
				SkipSpace();
				if (Peek() == TEXT('('))
				{
					return ParseGroup(outValue);
				}
				if (Peek() == TEXT('"'))
				{
					return ParseQuoted(outValue.Text);
				}
				outValue.Text = ParseBare();
				return true;
			}

			bool IsAtEnd()
			{
				SkipSpace();
				return Index >= Text.Len();
			}

		private:
			TCHAR Peek() const { return Index < Text.Len() ? Text[Index] : TEXT('\0'); }

			void SkipSpace()
			{
				while (Index < Text.Len() && FChar::IsWhitespace(Text[Index]))
				{
					++Index;
				}
			}

			/** Up to the next ',', ')' or '=', trimmed. */
			FString ParseBare()
			{
				const int32 start = Index;
				while (Index < Text.Len() && Text[Index] != TEXT(',') && Text[Index] != TEXT(')') && Text[Index] != TEXT('='))
				{
					++Index;
				}
				return Text.Mid(start, Index - start).TrimStartAndEnd();
			}

			bool ParseQuoted(FString& outText)
			{
				++Index; // The opening quote.
				while (Index < Text.Len())
				{
					const TCHAR character = Text[Index++];
					if (character == TEXT('"'))
					{
						return true;
					}
					if (character == TEXT('\\') && Index < Text.Len())
					{
						outText.AppendChar(Text[Index++]);
					}
					else
					{
						outText.AppendChar(character);
					}
				}
				return false; // No closing quote.
			}

			bool ParseGroup(FConfigText& outValue)
			{
				outValue.bIsGroup = true;
				++Index; // The opening parenthesis.
				SkipSpace();
				if (Peek() == TEXT(')'))
				{
					++Index;
					return true;
				}

				while (true)
				{
					// A field starts with a bare name followed by '='; anything else is a list item.
					const int32 itemStart = Index;
					FConfigText item;
					if (Peek() != TEXT('(') && Peek() != TEXT('"'))
					{
						const FString name = ParseBare();
						SkipSpace();
						if (Peek() == TEXT('='))
						{
							++Index;
							if (!ParseValue(item))
							{
								return false;
							}
							outValue.Fields.Emplace(name, MoveTemp(item));
						}
						else
						{
							Index = itemStart;
						}
					}
					if (Index == itemStart)
					{
						if (!ParseValue(item))
						{
							return false;
						}
						outValue.Items.Add(MoveTemp(item));
					}

					SkipSpace();
					const TCHAR separator = Peek();
					++Index;
					if (separator == TEXT(')'))
					{
						return true;
					}
					if (separator != TEXT(','))
					{
						return false;
					}
					SkipSpace();
				}
			}

			const FString& Text;
			int32 Index = 0;
		};
	}

	TOptional<FConfigText> FConfigText::Parse(const FString& text)
	{
		FParser parser(text);
		FConfigText value;
		if (!parser.ParseValue(value) || !parser.IsAtEnd())
		{
			DRS_LOG(LogDrsXrArbiter, Warning, "Can't parse the config value '%s'.", *text);
			return {};
		}
		return value;
	}

	const FConfigText* FConfigText::Find(const TCHAR* fieldName) const
	{
		for (const TPair<FString, FConfigText>& field : Fields)
		{
			if (field.Key.Equals(fieldName, ESearchCase::IgnoreCase))
			{
				return &field.Value;
			}
		}
		return nullptr;
	}

	FString FConfigText::GetString(const TCHAR* fieldName) const
	{
		const FConfigText* field = Find(fieldName);
		return field ? field->Text : FString();
	}

	FName FConfigText::GetName(const TCHAR* fieldName) const
	{
		const FString text = GetString(fieldName);
		return text.IsEmpty() ? NAME_None : FName(*text);
	}

	TArray<FString> FConfigText::GetStrings(const TCHAR* fieldName) const
	{
		TArray<FString> result;
		for (const FConfigText& item : GetItems(fieldName))
		{
			if (!item.bIsGroup && !item.Text.IsEmpty())
			{
				result.Add(item.Text);
			}
		}
		return result;
	}

	TArray<FName> FConfigText::GetNames(const TCHAR* fieldName) const
	{
		TArray<FName> result;
		for (const FString& text : GetStrings(fieldName))
		{
			result.Emplace(*text);
		}
		return result;
	}

	TArray<FConfigText> FConfigText::GetItems(const TCHAR* fieldName) const
	{
		const FConfigText* field = Find(fieldName);
		if (field == nullptr)
		{
			return {};
		}
		if (!field->bIsGroup)
		{
			return { *field };
		}
		// A list of structs exports as ((...),(...)); a list with one struct may come back as that struct's fields.
		if (field->Items.IsEmpty() && !field->Fields.IsEmpty())
		{
			return { *field };
		}
		return field->Items;
	}
}
