// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrArbiter/DrsXrArbiterTypes.h"

#include "DrsBase/DrsLog.h"
#include "DrsXrArbiter/DrsXrArbiter.h"
#include "Misc/ConfigCacheIni.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsXrArbiterTypes)

using Drs::XrArbiter::FArbitrationInputs;

namespace
{
	EDrsArbitrationCriteria CriteriaFromString(const FString& criteriaString)
	{
		static const TPair<const TCHAR*, EDrsArbitrationCriteria> names[] =
		{
			{ TEXT("DeviceName"), EDrsArbitrationCriteria::DeviceName },
			{ TEXT("DeviceMake"), EDrsArbitrationCriteria::DeviceMake },
			{ TEXT("DeviceModel"), EDrsArbitrationCriteria::DeviceModel },
			{ TEXT("GpuFamily"), EDrsArbitrationCriteria::GpuFamily },
			{ TEXT("XrRuntime"), EDrsArbitrationCriteria::XrRuntime },
		};
		for (const TPair<const TCHAR*, EDrsArbitrationCriteria>& name : names)
		{
			if (criteriaString == name.Key)
			{
				return name.Value;
			}
		}
		DRS_LOG(LogDrsXrArbiter, Error, "Unknown arbitration criteria '%s'.", *criteriaString);
		return EDrsArbitrationCriteria::Invalid;
	}

	EDrsArbitrationConditionType ConditionTypeFromString(const FString& conditionString)
	{
		static const TPair<const TCHAR*, EDrsArbitrationConditionType> names[] =
		{
			{ TEXT("RequiredTrue"), EDrsArbitrationConditionType::RequiredTrue },
			{ TEXT("RequiredFalse"), EDrsArbitrationConditionType::RequiredFalse },
			{ TEXT("OptionalTrue"), EDrsArbitrationConditionType::OptionalTrue },
			{ TEXT("OptionalFalse"), EDrsArbitrationConditionType::OptionalFalse },
		};
		for (const TPair<const TCHAR*, EDrsArbitrationConditionType>& name : names)
		{
			if (conditionString == name.Key)
			{
				return name.Value;
			}
		}
		DRS_LOG(LogDrsXrArbiter, Error, "Unknown arbitration condition type '%s'.", *conditionString);
		return EDrsArbitrationConditionType::Invalid;
	}
}

TOptional<bool> FDrsArbitrationCondition::Evaluate(const FArbitrationInputs& inputs) const
{
	const FString* property = nullptr;
	switch (Criteria)
	{
	case EDrsArbitrationCriteria::DeviceName: property = inputs.DeviceName; break;
	case EDrsArbitrationCriteria::DeviceMake: property = inputs.DeviceMake; break;
	case EDrsArbitrationCriteria::DeviceModel: property = inputs.DeviceModel; break;
	case EDrsArbitrationCriteria::GpuFamily: property = inputs.GpuFamily; break;
	case EDrsArbitrationCriteria::XrRuntime: property = inputs.XrRuntime; break;
	default:
		DRS_LOG(LogDrsXrArbiter, Warning, "Unhandled arbitration criteria %d.", static_cast<int32>(Criteria));
		return {};
	}

	if (property == nullptr)
	{
		return {};
	}
	return property->Contains(Value);
}

bool FDrsArbitrationRule::Evaluate(const FArbitrationInputs& inputs) const
{
	if (!IsValid())
	{
		return false;
	}

	bool hasRequiredCondition = false;
	bool areRequiredConditionsSatisfied = true;
	bool hasOptionalCondition = false;
	bool isOptionalConditionSatisfied = false;

	for (const FDrsArbitrationCondition& condition : Conditions)
	{
		const TOptional<bool> result = condition.Evaluate(inputs);
		switch (condition.ConditionType)
		{
		case EDrsArbitrationConditionType::RequiredTrue:
			hasRequiredCondition = true;
			areRequiredConditionsSatisfied &= result.Get(false);
			break;
		case EDrsArbitrationConditionType::RequiredFalse:
			hasRequiredCondition = true;
			areRequiredConditionsSatisfied &= !result.Get(true);
			break;
		case EDrsArbitrationConditionType::OptionalTrue:
			hasOptionalCondition = true;
			isOptionalConditionSatisfied |= result.Get(false);
			break;
		case EDrsArbitrationConditionType::OptionalFalse:
			hasOptionalCondition = true;
			isOptionalConditionSatisfied |= !result.Get(true);
			break;
		default:
			DRS_LOG(LogDrsXrArbiter, Warning, "Unhandled arbitration condition type %d.", static_cast<int32>(condition.ConditionType));
			break;
		}
	}

	if (hasRequiredCondition && hasOptionalCondition)
	{
		return areRequiredConditionsSatisfied && isOptionalConditionSatisfied;
	}
	return hasRequiredCondition ? areRequiredConditionsSatisfied : isOptionalConditionSatisfied;
}

FDrsArbitrationCondition FDrsArbitrationCondition::ParseFromConfig(const FString& conditionString)
{
	FDrsArbitrationCondition condition;

	TArray<FString> propertyStrings;
	conditionString.ParseIntoArray(propertyStrings, TEXT(","), true);
	for (const FString& propertyString : propertyStrings)
	{
		FString propertyName;
		FString propertyValue;
		if (!propertyString.Split(TEXT("="), &propertyName, &propertyValue))
		{
			continue;
		}
		propertyName.TrimStartAndEndInline();
		propertyValue.TrimStartAndEndInline();

		if (propertyName == TEXT("ConditionType"))
		{
			condition.ConditionType = ConditionTypeFromString(propertyValue);
		}
		else if (propertyName == TEXT("Criteria"))
		{
			condition.Criteria = CriteriaFromString(propertyValue);
		}
		else if (propertyName == TEXT("Value"))
		{
			condition.Value = propertyValue.TrimQuotes();
		}
	}

	return condition;
}

FDrsArbitrationRule FDrsArbitrationRule::ParseFromConfig(const FString& ruleString)
{
	FDrsArbitrationRule rule;

	FString propertyString;
	FString trailingString;
	if (ruleString.Split(TEXT("Configuration="), nullptr, &propertyString))
	{
		// The type name ends at the next ',' or ')'.
		int32 endIndex = 0;
		while (endIndex < propertyString.Len() &&propertyString[endIndex] != TEXT(',') && propertyString[endIndex] != TEXT(')'))
		{
			++endIndex;
		}
		propertyString.LeftInline(endIndex);
		const TOptional<EDrsXrPluginType> pluginType = Drs::XrArbiter::PluginTypeFromString(propertyString.TrimStartAndEnd());
		if (pluginType.IsSet())
		{
			rule.Configuration = pluginType.GetValue();
		}
	}

	if (ruleString.Split(TEXT("Conditions=(("), nullptr, &propertyString))
	{
		propertyString.Split(TEXT("))"), &propertyString, &trailingString);

		TArray<FString> conditionStrings;
		propertyString.ParseIntoArray(conditionStrings, TEXT("),("), true);
		for (const FString& conditionString : conditionStrings)
		{
			const FDrsArbitrationCondition condition = FDrsArbitrationCondition::ParseFromConfig(conditionString);
			if (condition.IsValid())
			{
				rule.Conditions.Add(condition);
			}
		}
	}

	return rule;
}

TArray<FDrsArbitrationRule> FDrsArbitrationRule::ParseFromConfig(const TCHAR* section, const TCHAR* property, const FString& filename)
{
	TArray<FDrsArbitrationRule> rules;

	TArray<FString> ruleStrings;
	if (GConfig && GConfig->GetArray(section, property, ruleStrings, filename))
	{
		for (const FString& ruleString : ruleStrings)
		{
			FDrsArbitrationRule rule = ParseFromConfig(ruleString);
			if (rule.IsValid())
			{
				rules.Add(MoveTemp(rule));
			}
		}
	}

	return rules;
}
