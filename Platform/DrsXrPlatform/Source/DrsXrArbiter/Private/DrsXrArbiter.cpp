// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrArbiter/DrsXrArbiter.h"

#include "Dom/JsonObject.h"
#include "DrsBase/DrsLog.h"
#include "DrsXrRuntimeProbe.h"
#include "DrsXrRuntimeSelection.h"
#include "Engine/Engine.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#if WITH_EDITOR
#include "Editor.h"
#endif
#if PLATFORM_ANDROID
#include "Android/AndroidPlatformMisc.h"
#include <sys/system_properties.h>
#endif

DEFINE_LOG_CATEGORY(LogDrsXrArbiter);

namespace
{
	// The settings live in the plugin's own config file (Config = DrsXrPlatform): the plugin's DefaultDrsXrPlatform.ini, then
	// the project's, which wins. A plugin's Config/Game.ini would instead be applied over the project's DefaultGame.ini.
	const TCHAR* SettingsConfigName = TEXT("DrsXrPlatform");
	const TCHAR* ProjectSettingsSection = TEXT("/Script/DrsXrArbiter.DrsXrArbiterSettings");
	const TCHAR* XrArbiterProjectOverrideProperty = TEXT("XrArbiterProjectOverride");
	const TCHAR* VendorsProperty = TEXT("Vendors");
	const TCHAR* RuntimesProperty = TEXT("Runtimes");
	const TCHAR* RuntimePriorityProperty = TEXT("RuntimePriority");
	const TCHAR* EditorSettingsSection = TEXT("/Script/DrsXrPlatformEditor.DrsXrPlatformEditorSettings");
	const TCHAR* XrArbiterEditorOverrideProperty = TEXT("XrArbiterEditorOverride");
	const TCHAR* ArbitratedLoadingPhaseKey = TEXT("ArbitratedLoadingPhase");
	const TCHAR* ArbitratedXrPluginKey = TEXT("ArbitratedXrPlugin");

	// Shared state. The vendor is decided once, at PostConfigInit.
	TOptional<FName> CurrentPluginType;
	TSet<FName> ArbitratedPluginTypes; // The vendors whose modules are integrated with the arbiter.
	bool bLoadAllPluginTypes = false; // Commandlets load every vendor's modules, so that everything cooks.
	FName SelectedRuntime = PLATFORM_ANDROID ? FName(TEXT("Android")) : NAME_None;
	FName SelectedRuntimeVendor;

	bool IsArbitrationDecided() { return CurrentPluginType.IsSet() || bLoadAllPluginTypes; }

	/** Reads a module's original loading phase, ignoring any trailing note ("PostConfigInit (Note: ...)"). */
	ELoadingPhase::Type ParseOriginalLoadingPhase(const FString& value)
	{
		FString token = value.TrimStartAndEnd();
		int32 separatorIndex = INDEX_NONE;
		if (token.FindChar(TEXT(' '), separatorIndex))
		{
			token.LeftInline(separatorIndex);
		}
		return ELoadingPhase::FromString(*token);
	}

	/** A vendor name from config or the command line. "None" (or empty) is None; an unknown vendor is logged but kept. */
	FName ParseVendorName(const FString& value, const TCHAR* sourceName)
	{
		const FString trimmed = value.TrimStartAndEnd().TrimQuotes();
		if (trimmed.IsEmpty() || trimmed.Equals(TEXT("None"), ESearchCase::IgnoreCase))
		{
			return NAME_None;
		}
		const FName vendorId(*trimmed);
		DRS_CLOG(Drs::XrArbiter::FindVendor(vendorId) == nullptr, LogDrsXrArbiter, Warning, "%s names the vendor '%s', which is not in the vendor table.", sourceName, *trimmed);
		return vendorId;
	}

	/** The config file that holds the arbiter's settings. */
	FString GetSettingsFilename()
	{
		return GConfig ? GConfig->GetConfigFilename(SettingsConfigName) : FString();
	}

	/** Reads a vendor from config. Only a vendor other than None counts as set. */
	bool GetPluginTypeFromConfig(const TCHAR* section, const TCHAR* property, const FString& filename, const TCHAR* sourceName, FName& outPluginType)
	{
		FString value;
		if (GConfig && GConfig->GetString(section, property, value, filename))
		{
			const FName vendorId = ParseVendorName(value, sourceName);
			if (!vendorId.IsNone())
			{
				DRS_LOG(LogDrsXrArbiter, Log, "XR plugin from %s: '%s'.", sourceName, *vendorId.ToString());
				outPluginType = vendorId;
				return true;
			}
		}
		return false;
	}

	bool GetPluginTypeFromCommandLine(FName& outPluginType)
	{
		FString argument;
		if (FParse::Value(FCommandLine::Get(), TEXT("XrPlugin="), argument))
		{
			// None may be given, to load no vendor modules.
			outPluginType = ParseVendorName(argument, TEXT("-XrPlugin="));
			DRS_LOG(LogDrsXrArbiter, Log, "XR plugin from the command line: '%s'.", *outPluginType.ToString());
			return true;
		}
		return false;
	}

	bool GetPluginTypeFromProjectOverride(FName& outPluginType)
	{
		return GetPluginTypeFromConfig(ProjectSettingsSection, XrArbiterProjectOverrideProperty, GetSettingsFilename(), TEXT("The project override"), outPluginType);
	}

	bool GetPluginTypeFromEditorSettings(FName& outPluginType)
	{
#if WITH_EDITOR
		return GetPluginTypeFromConfig(EditorSettingsSection, XrArbiterEditorOverrideProperty, GEditorSettingsIni, TEXT("The editor setting"), outPluginType);
#else
		return false;
#endif
	}

	bool GetPluginTypeFromIdentities(const TArray<FString>& identities, FName& outPluginType)
	{
		const FName vendorId = Drs::XrArbiter::MatchVendor(Drs::XrArbiter::GetVendors(), identities);
		if (vendorId.IsNone())
		{
			return false;
		}
		outPluginType = vendorId;
		return true;
	}

#if PLATFORM_ANDROID
	FString GetAndroidSystemProperty(const char* name)
	{
		char value[PROP_VALUE_MAX] = {};
		__system_property_get(name, value);
		return UTF8_TO_TCHAR(value);
	}
#endif

	bool GetPluginTypeFromAndroidDevice(FName& outPluginType)
	{
#if PLATFORM_ANDROID
		// Quest reports brand "oculus" and manufacturer "Oculus". There is no device name on Android before the HMD is up.
		const FString brand = GetAndroidSystemProperty("ro.product.brand");
		const FString manufacturer = FAndroidMisc::GetDeviceMake();
		DRS_LOG(LogDrsXrArbiter, Log, "Choosing the XR plugin for device brand '%s', manufacturer '%s'.", *brand, *manufacturer);
		return GetPluginTypeFromIdentities({ brand, manufacturer }, outPluginType);
#else
		return false;
#endif
	}

	bool GetPluginTypeFromXrRuntime(FName& outPluginType)
	{
		// On desktop the selected runtime usually implies the vendor. Multi-vendor runtimes such as SteamVR name none, so
		// the runtime probe decides.
		const FName vendorId = Drs::XrArbiter::GetXrRuntimeVendor();
		if (vendorId.IsNone())
		{
			return false;
		}
		DRS_LOG(LogDrsXrArbiter, Log, "Choosing the XR plugin '%s' from the OpenXR runtime '%s'.", *vendorId.ToString(), *Drs::XrArbiter::GetXrRuntimeType().ToString());
		outPluginType = vendorId;
		return true;
	}

	bool GetPluginTypeFromRuntimeProbe(FName& outPluginType)
	{
		// The last step, for when nothing else identifies the hardware: the runtime names no vendor (SteamVR), none was
		// selected, or on Android the brand matches no vendor. Runs at PostConfigInit, before the engine creates its own
		// instance, so the chosen vendor's modules still get their OpenXR extensions. Its answer is final: a probe that
		// finds no headset (no instance, or no system) chooses no vendor.
		if (FParse::Param(FCommandLine::Get(), TEXT("noHMD")))
		{
			return false;
		}

		const TOptional<Drs::XrArbiter::FXrRuntimeProbe>& probe = Drs::XrArbiter::ProbeXrRuntime();
		const FName vendorId = Drs::XrArbiter::MatchVendorFromProbe(Drs::XrArbiter::GetVendors(), probe);
		if (vendorId.IsNone())
		{
			DRS_CLOG(probe.IsSet() && !probe->SystemName.IsEmpty(), LogDrsXrArbiter, Log, "The runtime probe's system '%s' matches no vendor.", *probe->SystemName);
			DRS_CLOG(!probe.IsSet() || probe->SystemName.IsEmpty(), LogDrsXrArbiter, Log, "The runtime probe found no headset, so no vendor plugin is chosen.");
			return false;
		}
		DRS_LOG(LogDrsXrArbiter, Log, "Choosing the XR plugin '%s' from the runtime probe.", *vendorId.ToString());
		outPluginType = vendorId;
		return true;
	}

	FName ChoosePluginTypeForMobileDevice()
	{
		FName pluginType;
		if (GetPluginTypeFromProjectOverride(pluginType) || GetPluginTypeFromAndroidDevice(pluginType) || GetPluginTypeFromRuntimeProbe(pluginType))
		{
			return pluginType;
		}
		return NAME_None;
	}

	FName ChoosePluginTypeForTetheredDevice()
	{
		if (Drs::XrArbiter::IsNonVrMode())
		{
			return NAME_None;
		}

		FName pluginType;
		if (GetPluginTypeFromCommandLine(pluginType)
			|| GetPluginTypeFromProjectOverride(pluginType)
			|| GetPluginTypeFromXrRuntime(pluginType)
			|| GetPluginTypeFromRuntimeProbe(pluginType))
		{
			return pluginType;
		}
		return NAME_None;
	}

	FName ChoosePluginTypeForEditor()
	{
		FName pluginType;
		if (GetPluginTypeFromEditorSettings(pluginType)
			|| GetPluginTypeFromProjectOverride(pluginType)
			|| GetPluginTypeFromXrRuntime(pluginType)
			|| GetPluginTypeFromRuntimeProbe(pluginType))
		{
			return pluginType;
		}
		return NAME_None;
	}

	struct FArbitratedModule
	{
		FName ModuleName;
		ELoadingPhase::Type LoadingPhase;
	};

	/**
	 * Selects the desktop OpenXR runtime at startup, then loads the modules of the selected vendor plugin type. Opted-in
	 * modules have LoadingPhase None in their descriptor, their real phase in ArbitratedLoadingPhase, and their type in
	 * ArbitratedXrPlugin (on the module or the plugin).
	 */
	class FDrsXrArbiterModule : public IModuleInterface
	{
	public:
		virtual void StartupModule() override
		{
			// Before OpenXR creates its instance at PostConfigInit. Runtime and plugin are chosen independently.
			Drs::XrRuntime::SelectRuntime();

			BuildConfigurations();
			if (Configurations.IsEmpty())
			{
				// Nothing to arbitrate: every vendor plugin loads as normal.
				DRS_LOG(LogDrsXrArbiter, Log, "No vendor plugin is integrated with the arbiter; vendor plugins load as normal.");
				return;
			}

			LoadingPhaseCompleteHandle = IPluginManager::Get().OnLoadingPhaseComplete().AddLambda([this](ELoadingPhase::Type loadingPhase, bool)
			{
				OnLoadingPhaseComplete(loadingPhase);
			});
		}

		virtual void ShutdownModule() override
		{
			IPluginManager::Get().OnLoadingPhaseComplete().Remove(LoadingPhaseCompleteHandle);
		}

	private:
		void BuildConfigurations()
		{
			Configurations.Empty();
			ArbitratedPluginTypes.Empty();
			for (const TSharedRef<IPlugin>& plugin : IPluginManager::Get().GetEnabledPlugins())
			{
				if (plugin->GetDescriptor().Modules.ContainsByPredicate([](const FModuleDescriptor& module) { return module.LoadingPhase == ELoadingPhase::None; }))
				{
					AddArbitratedModules(plugin.Get());
				}
			}
		}

		void AddArbitratedModules(const IPlugin& plugin)
		{
			// The engine's parsed descriptor drops keys it does not know, so read the raw json for the arbitration keys.
			const FString& descriptorFileName = plugin.GetDescriptorFileName();
			FString descriptorJson;
			if (!FFileHelper::LoadFileToString(descriptorJson, *descriptorFileName))
			{
				DRS_LOG(LogDrsXrArbiter, Error, "Failed to read plugin descriptor '%s'.", *descriptorFileName);
				return;
			}

			TSharedPtr<FJsonObject> descriptorObject;
			if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(descriptorJson), descriptorObject) || !descriptorObject.IsValid())
			{
				DRS_LOG(LogDrsXrArbiter, Error, "Failed to parse plugin descriptor '%s'.", *descriptorFileName);
				return;
			}

			const TArray<TSharedPtr<FJsonValue>>* moduleValues = nullptr;
			if (!descriptorObject->TryGetArrayField(TEXT("Modules"), moduleValues))
			{
				return;
			}

			// The plugin's vendor applies to its modules that don't name their own.
			const FString& pluginName = plugin.GetName();
			FName pluginTypeForPlugin;
			FString pluginTypeName;
			if (descriptorObject->TryGetStringField(ArbitratedXrPluginKey, pluginTypeName))
			{
				pluginTypeForPlugin = ParseVendorName(pluginTypeName, *pluginName);
			}

			int32 arbitratedModuleCount = 0;
			for (const TSharedPtr<FJsonValue>& moduleValue : *moduleValues)
			{
				const TSharedPtr<FJsonObject>* moduleObject = nullptr;
				if (!moduleValue.IsValid() || !moduleValue->TryGetObject(moduleObject))
				{
					continue;
				}

				FString moduleName;
				FString originalPhase;
				if (!(*moduleObject)->TryGetStringField(TEXT("Name"), moduleName) || !(*moduleObject)->TryGetStringField(ArbitratedLoadingPhaseKey, originalPhase))
				{
					continue;
				}

				FString declaredPhase;
				(*moduleObject)->TryGetStringField(TEXT("LoadingPhase"), declaredPhase);
				if (ELoadingPhase::FromString(*declaredPhase) != ELoadingPhase::None)
				{
					DRS_LOG(LogDrsXrArbiter, Error, "Arbitrated module %s::%s still has a LoadingPhase. Set it to None, or remove %s to exempt the module.", *pluginName, *moduleName, ArbitratedLoadingPhaseKey);
					continue;
				}

				// EarliestPossible could be earlier than the arbiter itself.
				const ELoadingPhase::Type loadingPhase = ParseOriginalLoadingPhase(originalPhase);
				if (loadingPhase == ELoadingPhase::Max || loadingPhase == ELoadingPhase::None || loadingPhase == ELoadingPhase::EarliestPossible)
				{
					DRS_LOG(LogDrsXrArbiter, Error, "%s::%s has the loading phase '%s', which cannot be arbitrated.", *pluginName, *moduleName, *originalPhase);
					continue;
				}

				FName pluginType = pluginTypeForPlugin;
				if ((*moduleObject)->TryGetStringField(ArbitratedXrPluginKey, pluginTypeName))
				{
					pluginType = ParseVendorName(pluginTypeName, *pluginName);
				}
				if (pluginType.IsNone())
				{
					DRS_LOG(LogDrsXrArbiter, Error, "Arbitrated module %s::%s has no vendor. Set %s on the module or its plugin.", *pluginName, *moduleName, ArbitratedXrPluginKey);
					continue;
				}

				DRS_LOG(LogDrsXrArbiter, Verbose, "Arbitrating %s::%s (%s, %s).", *pluginName, *moduleName, ToString(loadingPhase), *pluginType.ToString());
				Configurations.FindOrAdd(pluginType).Add({ FName(*moduleName), loadingPhase });
				ArbitratedPluginTypes.Add(pluginType);
				++arbitratedModuleCount;
			}

			UE_CLOG(arbitratedModuleCount > 0, LogDrsXrArbiter, Log, TEXT("Arbitrating %d module(s) of %s %s."), arbitratedModuleCount, *pluginName, *plugin.GetDescriptor().VersionName);
		}

		void OnLoadingPhaseComplete(ELoadingPhase::Type loadingPhase)
		{
			if (IsRunningDedicatedServer())
			{
				return; // Servers load no vendor XR modules.
			}

			// The vendor is decided at PostConfigInit (commandlets at once), and no arbitrated module loads earlier, so no
			// phase is ever missed.
			ChoosePluginType(loadingPhase);
			if (IsArbitrationDecided())
			{
				LoadModules(loadingPhase);
			}
		}

		void ChoosePluginType(ELoadingPhase::Type loadingPhase)
		{
			if (IsArbitrationDecided())
			{
				return;
			}

			if (IsRunningCommandlet())
			{
				DRS_LOG(LogDrsXrArbiter, Log, "Selecting every vendor XR plugin for the commandlet.");
				bLoadAllPluginTypes = true;
				return;
			}

			if (loadingPhase < ELoadingPhase::PostConfigInit)
			{
				return; // Config is not ready yet.
			}

#if PLATFORM_ANDROID
			CurrentPluginType = ChoosePluginTypeForMobileDevice();
#else
			CurrentPluginType = GIsEditor ? ChoosePluginTypeForEditor() : ChoosePluginTypeForTetheredDevice();
#endif
			const FName pluginType = CurrentPluginType.GetValue();
			DRS_LOG(LogDrsXrArbiter, Log, "Selected the XR plugin '%s'.", *pluginType.ToString());
			DRS_CLOG(!pluginType.IsNone() && !Configurations.Contains(pluginType), LogDrsXrArbiter, Log, "No arbitrated modules found for the XR plugin '%s'.", *pluginType.ToString());
		}

		void LoadModules(ELoadingPhase::Type loadingPhase)
		{
			TSet<FName> modulesToLoad;
			for (const TPair<FName, TArray<FArbitratedModule>>& configuration : Configurations)
			{
				if (bLoadAllPluginTypes || configuration.Key == CurrentPluginType.Get(NAME_None))
				{
					for (const FArbitratedModule& module : configuration.Value)
					{
						if (module.LoadingPhase == loadingPhase)
						{
							modulesToLoad.Add(module.ModuleName);
						}
					}
				}
			}
			if (modulesToLoad.IsEmpty())
			{
				return;
			}

			for (const TSharedRef<IPlugin>& plugin : IPluginManager::Get().GetEnabledPlugins())
			{
				for (const FModuleDescriptor& moduleDescriptor : plugin->GetDescriptor().Modules)
				{
					if (moduleDescriptor.LoadingPhase != ELoadingPhase::None || !modulesToLoad.Contains(moduleDescriptor.Name)
						|| FModuleManager::Get().IsModuleLoaded(moduleDescriptor.Name) || !moduleDescriptor.IsLoadedInCurrentConfiguration())
					{
						continue;
					}

					DRS_LOG(LogDrsXrArbiter, Log, "Loading %s::%s (%s).", *plugin->GetName(), *moduleDescriptor.Name.ToString(), ToString(loadingPhase));
					EModuleLoadResult loadResult;
					FModuleManager::Get().LoadModuleWithFailureReason(moduleDescriptor.Name, loadResult);
					UE_CLOG(loadResult != EModuleLoadResult::Success, LogDrsXrArbiter, Error, TEXT("Failed to load %s::%s."), *plugin->GetName(), *moduleDescriptor.Name.ToString());
				}
			}
		}

		TMap<FName, TArray<FArbitratedModule>> Configurations;
		FDelegateHandle LoadingPhaseCompleteHandle;
	};
}

IMPLEMENT_MODULE(FDrsXrArbiterModule, DrsXrArbiter)

namespace Drs::XrArbiter
{
	const FName AndroidRuntime(TEXT("Android"));

	FName GetXrPluginType()
	{
		// Detect the running vendor plugin, whether the arbiter loaded it or the engine did.
		for (const FDrsXrVendor& vendor : GetVendors())
		{
			if (!vendor.MarkerModule.IsNone() && FModuleManager::Get().IsModuleLoaded(vendor.MarkerModule))
			{
				return vendor.Id;
			}
		}
		return NAME_None;
	}

	FName GetArbitratedXrPluginType()
	{
		return CurrentPluginType.Get(NAME_None);
	}

	bool IsXrPluginTypeArbitrated(FName vendorId)
	{
		return ArbitratedPluginTypes.Contains(vendorId);
	}

	FName GetXrPluginOverride(EDrsXrPluginOverrideType& outOverrideType)
	{
		FName pluginType;
		if (GIsEditor && GetPluginTypeFromEditorSettings(pluginType))
		{
			outOverrideType = EDrsXrPluginOverrideType::EditorSettings;
		}
		else if (GetPluginTypeFromCommandLine(pluginType))
		{
			outOverrideType = EDrsXrPluginOverrideType::CommandLine;
		}
		else if (GetPluginTypeFromProjectOverride(pluginType))
		{
			outOverrideType = EDrsXrPluginOverrideType::ProjectSettings;
		}
		else
		{
			outOverrideType = EDrsXrPluginOverrideType::None;
		}
		return pluginType;
	}

	FName GetExpectedXrPluginType(const FString& deviceName)
	{
		// Only the hardware, not the runtime or any override.
		FName pluginType;
#if PLATFORM_ANDROID
		GetPluginTypeFromAndroidDevice(pluginType);
#else
		// The caller passes the name, because the HMD module used during arbitration may be unavailable later (e.g. in PIE).
		GetPluginTypeFromIdentities({ deviceName }, pluginType);
#endif
		return pluginType;
	}

	const TArray<FDrsXrVendor>& GetVendors()
	{
		// Read once: GetXrPluginType is called often, and config changes need a restart anyway.
		static const TArray<FDrsXrVendor> vendors = []
		{
			TArray<FString> entries;
			if (GConfig)
			{
				GConfig->GetArray(ProjectSettingsSection, VendorsProperty, entries, GetSettingsFilename());
			}
			return ParseVendors(entries);
		}();
		return vendors;
	}

	const FDrsXrVendor* FindVendor(FName vendorId)
	{
		return GetVendors().FindByPredicate([vendorId](const FDrsXrVendor& vendor) { return vendor.Id == vendorId; });
	}

	TArray<FDrsXrRuntime> GetPrioritisedRuntimes()
	{
		TArray<FString> runtimeEntries;
		TArray<FString> priorityEntries;
		if (GConfig)
		{
			const FString filename = GetSettingsFilename();
			GConfig->GetArray(ProjectSettingsSection, RuntimesProperty, runtimeEntries, filename);
			GConfig->GetArray(ProjectSettingsSection, RuntimePriorityProperty, priorityEntries, filename);
		}

		TArray<FName> priority;
		for (const FString& entry : priorityEntries)
		{
			priority.Emplace(*entry.TrimStartAndEnd().TrimQuotes());
		}
		return OrderRuntimes(ParseRuntimes(runtimeEntries), priority);
	}

	void SetXrRuntime(FName runtimeName, FName vendorId)
	{
		SelectedRuntime = runtimeName;
		SelectedRuntimeVendor = vendorId;
	}

	FName GetXrRuntimeType()
	{
		return SelectedRuntime;
	}

	FName GetXrRuntimeVendor()
	{
		return SelectedRuntimeVendor;
	}

	bool IsNonVrMode()
	{
		static TOptional<bool> isNonVrMode;

		// In the editor the answer changes as VR Preview starts and stops, so it is never cached there.
		if (!isNonVrMode.IsSet() || GIsEditor)
		{
			if (FParse::Param(FCommandLine::Get(), TEXT("noHMD")))
			{
				isNonVrMode = true;
			}
#if WITH_EDITOR
			else if (GIsEditor)
			{
				if (GEditor)
				{
					isNonVrMode = !GEditor->IsVRPreviewActive();
				}
			}
#endif
			else if (GEngine)
			{
				isNonVrMode = !UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayConnected();
			}
		}

		return isNonVrMode.Get(false);
	}
}
