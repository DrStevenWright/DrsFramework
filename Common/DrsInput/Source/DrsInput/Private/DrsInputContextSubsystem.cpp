// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsInput/DrsInputContextSubsystem.h"

#include "DrsInput/DrsInputLog.h"
#include "InputMappingContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsInputContextSubsystem)

void UDrsInputContextSubsystem::AddReason(const UInputMappingContext* context, FName reason)
{
	if (context)
	{
		ActiveInputContexts.FindOrAdd(context).Add(reason);
	}
}

bool UDrsInputContextSubsystem::RemoveReason(const UInputMappingContext* context, FName reason)
{
	if (TSet<FName>* reasons = context ? ActiveInputContexts.Find(context) : nullptr)
	{
		reasons->Remove(reason);
		if (reasons->IsEmpty())
		{
			ActiveInputContexts.Remove(context);
			return true;
		}
		return false;
	}

	return true;
}

bool UDrsInputContextSubsystem::HasReason(const UInputMappingContext* context, FName reason) const
{
	const TSet<FName>* reasons = context ? ActiveInputContexts.Find(context) : nullptr;
	return reasons && reasons->Contains(reason);
}

TArray<FName> UDrsInputContextSubsystem::GetReasons(const UInputMappingContext* context) const
{
	const TSet<FName>* reasons = context ? ActiveInputContexts.Find(context) : nullptr;
	return reasons ? reasons->Array() : TArray<FName>();
}

FString UDrsInputContextSubsystem::GetReasonsString(const UInputMappingContext* context) const
{
	FString reasonsString;

	for (const FName& reason : GetReasons(context))
	{
		if (!reasonsString.IsEmpty())
		{
			reasonsString += ", ";
		}
		reasonsString += reason.ToString();
	}

	return reasonsString;
}

void UDrsInputContextSubsystem::PrintReasons(const UInputMappingContext* context) const
{
	if (context)
	{
		if (ActiveInputContexts.Contains(context))
		{
			DRS_LOG(LogDrsInput, Log, "Input context '%s' was added with the following specified reasons: %s.", *context->GetName(), *GetReasonsString(context));
		}
		else
		{
			DRS_LOG(LogDrsInput, Log, "Input context '%s' was not added with any specified reasons.", *context->GetName());
		}
	}
}
