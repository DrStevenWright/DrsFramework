// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrArbiter/DrsXrArbiterTypes.h"

#include "DrsBase/DrsLog.h"
#include "DrsXrArbiter/DrsXrArbiter.h"
#include "DrsXrArbiter/DrsXrConfigText.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DrsXrArbiterTypes)

namespace
{
	using Drs::XrArbiter::FConfigText;

	const TPair<const TCHAR*, EDrsXrManifestLocator> LocatorNames[] =
	{
		{ TEXT("EnvironmentVariable"), EDrsXrManifestLocator::EnvironmentVariable },
		{ TEXT("UriHandler"), EDrsXrManifestLocator::UriHandler },
		{ TEXT("SteamLibrary"), EDrsXrManifestLocator::SteamLibrary },
		{ TEXT("ProgramFiles"), EDrsXrManifestLocator::ProgramFiles },
		{ TEXT("RegisteredRuntime"), EDrsXrManifestLocator::RegisteredRuntime },
	};

	/** The enum's value for a name in a table; reflection isn't available when the tables are read. */
	template <typename TEnum, int32 Count>
	TOptional<TEnum> FromName(const TPair<const TCHAR*, TEnum> (&names)[Count], const FString& name)
	{
		for (const TPair<const TCHAR*, TEnum>& entry : names)
		{
			if (name.Equals(entry.Key, ESearchCase::IgnoreCase))
			{
				return entry.Value;
			}
		}
		return {};
	}

	FDrsXrManifestLocator ParseLocator(const FConfigText& text)
	{
		FDrsXrManifestLocator locator;
		const FString locatorName = text.GetString(TEXT("Locator"));
		locator.Locator = FromName(LocatorNames, locatorName).Get(EDrsXrManifestLocator::Invalid);
		DRS_CLOG(locator.Locator == EDrsXrManifestLocator::Invalid, LogDrsXrArbiter, Warning, "Unknown manifest locator '%s'.", *locatorName);
		locator.Name = text.GetString(TEXT("Name"));
		locator.Path = text.GetString(TEXT("Path"));
		return locator;
	}
}

namespace Drs::XrArbiter
{
	TArray<FDrsXrVendor> ParseVendors(const TArray<FString>& entries)
	{
		TArray<FDrsXrVendor> vendors;
		for (const FString& entry : entries)
		{
			const TOptional<FConfigText> text = FConfigText::Parse(entry);
			if (!text.IsSet())
			{
				continue;
			}

			FDrsXrVendor vendor;
			vendor.Id = text->GetName(TEXT("Id"));
			vendor.Keywords = text->GetStrings(TEXT("Keywords"));
			vendor.MarkerModule = text->GetName(TEXT("MarkerModule"));
			if (vendor.IsValid())
			{
				vendors.Add(MoveTemp(vendor));
			}
			else
			{
				DRS_LOG(LogDrsXrArbiter, Warning, "Skipping the vendor entry '%s', which has no Id.", *entry);
			}
		}
		return vendors;
	}

	FName MatchVendor(const TArray<FDrsXrVendor>& vendors, const TArray<FString>& identities)
	{
		for (const FDrsXrVendor& vendor : vendors)
		{
			for (const FString& keyword : vendor.Keywords)
			{
				for (const FString& identity : identities)
				{
					if (!keyword.IsEmpty() && identity.Contains(keyword, ESearchCase::IgnoreCase))
					{
						return vendor.Id;
					}
				}
			}
		}
		return NAME_None;
	}

	TArray<FDrsXrRuntime> ParseRuntimes(const TArray<FString>& entries)
	{
		TArray<FDrsXrRuntime> runtimes;
		for (const FString& entry : entries)
		{
			const TOptional<FConfigText> text = FConfigText::Parse(entry);
			if (!text.IsSet())
			{
				continue;
			}

			FDrsXrRuntime runtime;
			runtime.Name = text->GetName(TEXT("Name"));
			runtime.Vendor = text->GetName(TEXT("Vendor"));
			for (const FConfigText& locatorText : text->GetItems(TEXT("Manifest")))
			{
				FDrsXrManifestLocator locator = ParseLocator(locatorText);
				if (locator.IsValid())
				{
					runtime.Manifest.Add(MoveTemp(locator));
				}
			}
			runtime.RunningProcesses = text->GetStrings(TEXT("RunningProcesses"));
			if (runtime.IsValid())
			{
				runtimes.Add(MoveTemp(runtime));
			}
			else
			{
				DRS_LOG(LogDrsXrArbiter, Warning, "Skipping the runtime entry '%s', which needs a Name and a valid Manifest.", *entry);
			}
		}
		return runtimes;
	}

	TArray<FDrsXrRuntime> OrderRuntimes(const TArray<FDrsXrRuntime>& runtimes, const TArray<FName>& priority)
	{
		TArray<FDrsXrRuntime> ordered;
		for (const FName runtimeName : priority)
		{
			const FDrsXrRuntime* runtime = runtimes.FindByPredicate([runtimeName](const FDrsXrRuntime& candidate) { return candidate.Name == runtimeName; });
			if (runtime == nullptr)
			{
				DRS_LOG(LogDrsXrArbiter, Warning, "The runtime priority names '%s', which is not in the runtime table.", *runtimeName.ToString());
			}
			else if (!ordered.ContainsByPredicate([runtimeName](const FDrsXrRuntime& candidate) { return candidate.Name == runtimeName; }))
			{
				ordered.Add(*runtime);
			}
		}
		return ordered;
	}

	const FDrsXrRuntime* ChooseRuntime(const TArray<FDrsXrRuntime>& runtimes,
		TFunctionRef<FString(const FDrsXrRuntime&)> findManifest, TFunctionRef<bool(const FDrsXrRuntime&)> isRunning, FString& outManifest)
	{
		outManifest.Empty();
		for (const FDrsXrRuntime& runtime : runtimes)
		{
			const FString manifest = findManifest(runtime);
			if (!manifest.IsEmpty() && isRunning(runtime))
			{
				outManifest = manifest;
				return &runtime;
			}
		}
		return nullptr;
	}
}
