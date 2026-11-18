// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrArbiter/DrsXrArbiter.h"

#include "DrsBase/DrsLog.h"

#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "IHeadMountedDisplayModule.h"
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
#endif

DEFINE_LOG_CATEGORY(LogDrsXrArbiter);

namespace
{
	const TCHAR* ProjectSettingsSection = TEXT("/Script/DrsXrArbiter.DrsXrArbiterSettings");
	const TCHAR* XrArbiterProjectOverrideProperty = TEXT("XrArbiterProjectOverride");
	const TCHAR* MobileArbitrationRulesProperty = TEXT("MobileDeviceArbitrationRules");
	const TCHAR* TetheredArbitrationRulesProperty = TEXT("TetheredDeviceArbitrationRules");
	const TCHAR* EditorSettingsSection = TEXT("/Script/DrsXrPlatformEditor.DrsXrPlatformEditorSettings");
	const TCHAR* XrArbiterEditorOverrideProperty = TEXT("XrArbiterEditorOverride");
	const TCHAR* ArbitratedLoadingPhaseKey = TEXT("ArbitratedLoadingPhase");
	const TCHAR* ArbitratedXrPluginKey = TEXT("ArbitratedXrPlugin");

	/** A module that is loaded whenever a vendor's plugin is running, arbitrated or not. Used to detect the vendor. */
	const TPair<EDrsXrPluginType, const TCHAR*> VendorMarkerModules[] =
	{
		{ EDrsXrPluginType::Meta, TEXT("OculusXRHMD") },
		{ EDrsXrPluginType::Pico, TEXT("PICOOpenXRHMD") },
	};

	const TPair<EDrsXrPluginType, const TCHAR*> PluginTypeNames[] =
	{
		{ EDrsXrPluginType::None, TEXT("None") },
		{ EDrsXrPluginType::Meta, TEXT("Meta") },
		{ EDrsXrPluginType::Pico, TEXT("Pico") },
	};

	// Shared state. The plugin type is decided once, at or after PostConfigInit.
	TOptional<EDrsXrPluginType> CurrentPluginType;
	TSet<EDrsXrPluginType> ArbitratedPluginTypes; // The types whose modules are integrated with the arbiter.
	bool bLoadAllPluginTypes = false; // Commandlets load every plugin type's modules, so that everything cooks.
	ELoadingPhase::Type CurrentLoadingPhase = ELoadingPhase::EarliestPossible;
	EDrsXrRuntimeType SelectedRuntimeType = PLATFORM_ANDROID ? EDrsXrRuntimeType::Android : EDrsXrRuntimeType::None;
	EDrsXrRuntimeType ExpectedRuntimeType = PLATFORM_ANDROID ? EDrsXrRuntimeType::Android : EDrsXrRuntimeType::None;
	uint32 SelectedRuntimeFlags = 0;

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

	/** Reads a plugin type from config. Only a type other than None counts as set. */
	bool GetPluginTypeFromConfig(const TCHAR* section, const TCHAR* property, const FString& filename, const TCHAR* sourceName, EDrsXrPluginType& outPluginType)
	{
		FString value;
		if (GConfig && GConfig->GetString(section, property, value, filename))
		{
			DRS_LOG(LogDrsXrArbiter, Log, "Plugin type from %s: '%s'.", sourceName, *value);
			const TOptional<EDrsXrPluginType> pluginType = Drs::XrArbiter::PluginTypeFromString(value);
			if (pluginType.IsSet() && pluginType.GetValue() != EDrsXrPluginType::None)
			{
				outPluginType = pluginType.GetValue();
				return true;
			}
		}
		return false;
	}

	bool GetPluginTypeFromCommandLine(EDrsXrPluginType& outPluginType)
	{
		FString argument;
		if (FParse::Value(FCommandLine::Get(), TEXT("DrsXrPlugin="), argument))
		{
			DRS_LOG(LogDrsXrArbiter, Log, "Plugin type from the command line: '%s'.", *argument);
			const TOptional<EDrsXrPluginType> pluginType = Drs::XrArbiter::PluginTypeFromString(argument);
			if (pluginType.IsSet()) // None may be given, to load no vendor modules.
			{
				outPluginType = pluginType.GetValue();
				return true;
			}
		}
		return false;
	}

	bool GetPluginTypeFromProjectOverride(EDrsXrPluginType& outPluginType)
	{
		return GetPluginTypeFromConfig(ProjectSettingsSection, XrArbiterProjectOverrideProperty, GGameIni, TEXT("the project override"), outPluginType);
	}

	bool GetPluginTypeFromEditorSettings(EDrsXrPluginType& outPluginType)
	{
#if WITH_EDITOR
		return GetPluginTypeFromConfig(EditorSettingsSection, XrArbiterEditorOverrideProperty, GEditorSettingsIni, TEXT("the editor settings"), outPluginType);
#else
		return false;
#endif
	}

	bool GetPluginTypeFromRules(const TCHAR* property, const FDrsArbitrationInputs& inputs, EDrsXrPluginType& outPluginType)
	{
		for (const FDrsArbitrationRule& rule : FDrsArbitrationRule::ParseFromConfig(ProjectSettingsSection, property, GGameIni))
		{
			if (rule.Evaluate(inputs))
			{
				outPluginType = rule.Configuration;
				return true;
			}
		}
		return false;
	}

	bool GetPluginTypeFromAndroidDeviceProperties(EDrsXrPluginType& outPluginType)
	{
#if PLATFORM_ANDROID
		// The device name is not available on Android, so the rules use other device properties.
		const FString deviceMake = FAndroidMisc::GetDeviceMake();
		const FString deviceModel = FAndroidMisc::GetDeviceModel();
		const FString gpuFamily = FAndroidMisc::GetGPUFamily();
		DRS_LOG(LogDrsXrArbiter, Log, "Choosing the plugin type for device make '%s', model '%s', GPU '%s'.", *deviceMake, *deviceModel, *gpuFamily);

		FDrsArbitrationInputs inputs;
		inputs.DeviceMake = &deviceMake;
		inputs.DeviceModel = &deviceModel;
		inputs.GpuFamily = &gpuFamily;
		return GetPluginTypeFromRules(MobileArbitrationRulesProperty, inputs, outPluginType);
#else
		return false;
#endif
	}

	bool GetPluginTypeFromDeviceName(const FString& deviceName, EDrsXrPluginType& outPluginType)
	{
		FDrsArbitrationInputs inputs;
		inputs.DeviceName = &deviceName;
		return GetPluginTypeFromRules(TetheredArbitrationRulesProperty, inputs, outPluginType);
	}

	bool GetPluginTypeFromXrRuntime(EDrsXrRuntimeType runtimeType, EDrsXrPluginType& outPluginType)
	{
		// On desktop the selected runtime usually says which device to expect. Multi-vendor runtimes such as SteamVR have
		// no rule, so they fall back to the expected runtime, then to the device name.
		if (runtimeType == EDrsXrRuntimeType::None || CurrentLoadingPhase != ELoadingPhase::PostConfigInit)
		{
			return false;
		}

		const FString runtimeName = Drs::XrArbiter::RuntimeTypeToString(runtimeType);
		DRS_LOG(LogDrsXrArbiter, Log, "Choosing the plugin type from the XR runtime '%s'.", *runtimeName);

		FDrsArbitrationInputs inputs;
		inputs.XrRuntime = &runtimeName;
		if (GetPluginTypeFromRules(TetheredArbitrationRulesProperty, inputs, outPluginType))
		{
			return true;
		}

		const EDrsXrRuntimeType expectedRuntimeType = Drs::XrArbiter::GetExpectedXrRuntimeType();
		return expectedRuntimeType != runtimeType && GetPluginTypeFromXrRuntime(expectedRuntimeType, outPluginType);
	}

	bool GetPluginTypeFromConnectedDevice(EDrsXrPluginType& outPluginType)
	{
		// The device name is only known once the HMD module has initialised, which commits the engine to OpenXR before
		// the chosen plugin's modules can register their OpenXR extensions. This only affects tethered sessions over
		// multi-vendor runtimes (e.g. SteamVR) whose device could not be hinted another way.
		if (!IHeadMountedDisplayModule::IsAvailable() || CurrentLoadingPhase <= ELoadingPhase::PostConfigInit)
		{
			return false;
		}

		const FString deviceName = IHeadMountedDisplayModule::Get().GetDeviceSystemName();
		DRS_LOG(LogDrsXrArbiter, Log, "Choosing the plugin type for the device '%s'.", *deviceName);
		return GetPluginTypeFromDeviceName(deviceName, outPluginType);
	}

	EDrsXrPluginType ChoosePluginTypeForMobileDevice()
	{
		EDrsXrPluginType pluginType = EDrsXrPluginType::None;
		if (GetPluginTypeFromProjectOverride(pluginType) || GetPluginTypeFromAndroidDeviceProperties(pluginType))
		{
			return pluginType;
		}
		return EDrsXrPluginType::None;
	}

	TOptional<EDrsXrPluginType> ChoosePluginTypeForTetheredDevice()
	{
		if (Drs::XrArbiter::IsNonVrMode())
		{
			return EDrsXrPluginType::None;
		}

		EDrsXrPluginType pluginType = EDrsXrPluginType::None;
		if (GetPluginTypeFromCommandLine(pluginType)
			|| GetPluginTypeFromProjectOverride(pluginType)
			|| GetPluginTypeFromXrRuntime(Drs::XrArbiter::GetXrRuntimeType(), pluginType)
			|| GetPluginTypeFromConnectedDevice(pluginType))
		{
			return pluginType;
		}

		return {}; // Not enough information yet; there may be more at a later loading phase.
	}

	EDrsXrPluginType ChoosePluginTypeForEditor()
	{
		EDrsXrPluginType pluginType = EDrsXrPluginType::None;
		if (GetPluginTypeFromEditorSettings(pluginType)
			|| GetPluginTypeFromProjectOverride(pluginType)
			|| GetPluginTypeFromXrRuntime(Drs::XrArbiter::GetXrRuntimeType(), pluginType)
			|| GetPluginTypeFromConnectedDevice(pluginType))
		{
			return pluginType;
		}
		return EDrsXrPluginType::None;
	}

	struct FDrsArbitratedModule
	{
		FName ModuleName;
		ELoadingPhase::Type LoadingPhase;
	};

	/**
	 * Loads the modules of the selected vendor plugin type. Opted-in modules have LoadingPhase None in their descriptor,
	 * their real phase in ArbitratedLoadingPhase, and their type in ArbitratedXrPlugin (on the module or the plugin).
	 */
	class FDrsXrArbiterModule : public IModuleInterface
	{
	public:
		virtual void StartupModule() override
		{
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

			// The plugin's type applies to its modules that don't name their own.
			TOptional<EDrsXrPluginType> pluginTypeForPlugin;
			FString pluginTypeName;
			if (descriptorObject->TryGetStringField(ArbitratedXrPluginKey, pluginTypeName))
			{
				pluginTypeForPlugin = Drs::XrArbiter::PluginTypeFromString(pluginTypeName);
			}

			const FString& pluginName = plugin.GetName();
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

				TOptional<EDrsXrPluginType> pluginType = pluginTypeForPlugin;
				if ((*moduleObject)->TryGetStringField(ArbitratedXrPluginKey, pluginTypeName))
				{
					pluginType = Drs::XrArbiter::PluginTypeFromString(pluginTypeName);
				}
				if (!pluginType.IsSet())
				{
					DRS_LOG(LogDrsXrArbiter, Error, "Arbitrated module %s::%s has no valid plugin type. Set %s on the module or its plugin.", *pluginName, *moduleName, ArbitratedXrPluginKey);
					continue;
				}
				if (!Drs::XrArbiter::IsXrPluginTypeEnabled(pluginType.GetValue()))
				{
					continue; // Never loaded, not even for cooking.
				}

				DRS_LOG(LogDrsXrArbiter, Verbose, "Arbitrating %s::%s (%s, %s).", *pluginName, *moduleName, ToString(loadingPhase), *Drs::XrArbiter::PluginTypeToString(pluginType.GetValue()));
				Configurations.FindOrAdd(pluginType.GetValue()).Add({ FName(*moduleName), loadingPhase });
				ArbitratedPluginTypes.Add(pluginType.GetValue());
				++arbitratedModuleCount;
			}

			UE_CLOG(arbitratedModuleCount > 0, LogDrsXrArbiter, Log, TEXT("Arbitrating %d module(s) of %s %s."), arbitratedModuleCount, *pluginName, *plugin.GetDescriptor().VersionName);
		}

		void OnLoadingPhaseComplete(ELoadingPhase::Type loadingPhase)
		{
			CurrentLoadingPhase = loadingPhase;
			if (IsRunningDedicatedServer())
			{
				return; // Servers load no vendor XR modules.
			}

			ChoosePluginType(loadingPhase);
			if (IsArbitrationDecided())
			{
				// Catch up on the phases that passed before the type was known, then load this phase's modules.
				for (const ELoadingPhase::Type pendingPhase : PendingLoadingPhases)
				{
					LoadModules(pendingPhase);
				}
				PendingLoadingPhases.Empty();
				LoadModules(loadingPhase);
			}
			else
			{
				PendingLoadingPhases.AddUnique(loadingPhase);
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
				DRS_LOG(LogDrsXrArbiter, Log, "Selecting every plugin type for the commandlet.");
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
			if (!CurrentPluginType.IsSet())
			{
				return;
			}

			if (!Drs::XrArbiter::IsXrPluginTypeEnabled(CurrentPluginType.GetValue()))
			{
				CurrentPluginType = EDrsXrPluginType::None;
			}

			const EDrsXrPluginType pluginType = CurrentPluginType.GetValue();
			const FString pluginTypeName = Drs::XrArbiter::PluginTypeToString(pluginType);
			DRS_LOG(LogDrsXrArbiter, Log, "Selected the XR plugin type '%s'.", *pluginTypeName);
			UE_CLOG(pluginType != EDrsXrPluginType::None && !Configurations.Contains(pluginType), LogDrsXrArbiter, Log, TEXT("No arbitrated modules found for the plugin type '%s'."), *pluginTypeName);
		}

		void LoadModules(ELoadingPhase::Type loadingPhase)
		{
			TSet<FName> modulesToLoad;
			for (const TPair<EDrsXrPluginType, TArray<FDrsArbitratedModule>>& configuration : Configurations)
			{
				if (bLoadAllPluginTypes || configuration.Key == CurrentPluginType.Get(EDrsXrPluginType::None))
				{
					for (const FDrsArbitratedModule& module : configuration.Value)
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

		TMap<EDrsXrPluginType, TArray<FDrsArbitratedModule>> Configurations;
		TArray<ELoadingPhase::Type> PendingLoadingPhases;
		FDelegateHandle LoadingPhaseCompleteHandle;
	};
}

IMPLEMENT_MODULE(FDrsXrArbiterModule, DrsXrArbiter)

namespace Drs::XrArbiter
{
	EDrsXrPluginType GetXrPluginType()
	{
		// Detect the running vendor plugin, whether the arbiter loaded it or the engine did.
		for (const TPair<EDrsXrPluginType, const TCHAR*>& marker : VendorMarkerModules)
		{
			if (FModuleManager::Get().IsModuleLoaded(marker.Value))
			{
				return marker.Key;
			}
		}
		return EDrsXrPluginType::None;
	}

	EDrsXrPluginType GetArbitratedXrPluginType()
	{
		return CurrentPluginType.Get(EDrsXrPluginType::None);
	}

	bool IsXrPluginTypeArbitrated(EDrsXrPluginType pluginType)
	{
		return ArbitratedPluginTypes.Contains(pluginType);
	}

	EDrsXrPluginType GetXrPluginOverride(EDrsXrPluginOverrideType& outOverrideType)
	{
		EDrsXrPluginType pluginType = EDrsXrPluginType::None;
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

		return IsXrPluginTypeEnabled(pluginType) ? pluginType : EDrsXrPluginType::None;
	}

	EDrsXrPluginType GetExpectedXrPluginType(const FString& deviceName)
	{
		// Only the device rules, not the runtime: the runtime selection itself follows any override.
		EDrsXrPluginType pluginType = EDrsXrPluginType::None;
#if PLATFORM_ANDROID
		GetPluginTypeFromAndroidDeviceProperties(pluginType);
#else
		// The caller passes the name, because the HMD module used during arbitration may be unavailable later (e.g. in PIE).
		GetPluginTypeFromDeviceName(deviceName, pluginType);
#endif
		return pluginType;
	}

	bool IsXrPluginTypeEnabled(EDrsXrPluginType pluginType)
	{
		switch (pluginType)
		{
		case EDrsXrPluginType::Meta:
			return DRS_ENABLE_META_XR;
		case EDrsXrPluginType::Pico:
			return DRS_ENABLE_PICO_XR;
		default:
			return true;
		}
	}

	FString PluginTypeToString(EDrsXrPluginType pluginType)
	{
		for (const TPair<EDrsXrPluginType, const TCHAR*>& name : PluginTypeNames)
		{
			if (name.Key == pluginType)
			{
				return name.Value;
			}
		}
		return TEXT("Invalid");
	}

	TOptional<EDrsXrPluginType> PluginTypeFromString(const FString& pluginTypeString)
	{
		for (const TPair<EDrsXrPluginType, const TCHAR*>& name : PluginTypeNames)
		{
			if (pluginTypeString == name.Value)
			{
				return name.Key;
			}
		}
		DRS_LOG(LogDrsXrArbiter, Error, "Unknown XR plugin type '%s'.", *pluginTypeString);
		return {};
	}

	void SetXrRuntime(EDrsXrRuntimeType runtimeType, uint32 runtimeFlags)
	{
		SelectedRuntimeType = runtimeType;
		SelectedRuntimeFlags = runtimeFlags;
	}

	EDrsXrRuntimeType GetXrRuntimeType()
	{
		return SelectedRuntimeType;
	}

	uint32 GetXrRuntimeFlags()
	{
		return SelectedRuntimeFlags;
	}

	void SetExpectedXrRuntimeType(EDrsXrRuntimeType runtimeType)
	{
		ExpectedRuntimeType = runtimeType;
	}

	EDrsXrRuntimeType GetExpectedXrRuntimeType()
	{
		return ExpectedRuntimeType;
	}

	FString RuntimeTypeToString(EDrsXrRuntimeType runtimeType)
	{
		switch (runtimeType)
		{
		case EDrsXrRuntimeType::Android: return TEXT("Android");
		case EDrsXrRuntimeType::Oculus: return TEXT("Oculus");
		case EDrsXrRuntimeType::Pico: return TEXT("Pico");
		case EDrsXrRuntimeType::SteamVR: return TEXT("SteamVR");
		default: return TEXT("None");
		}
	}

	bool IsNonVrMode()
	{
		static TOptional<bool> isNonVrMode;

		// In the editor the answer changes as VR Preview starts and stops, so it is never cached there.
		if (!isNonVrMode.IsSet() || GIsEditor)
		{
			if (FString(FCommandLine::Get()).Contains(TEXT("noHMD")))
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
