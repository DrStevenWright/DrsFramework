# DrsXrPlatform

Six modules with different load rules. Startup order matters: `DrsXrArbiter` starts at `EarliestPossible` and selects the desktop OpenXR runtime, OpenXR creates its instance at `PostConfigInit`, and the arbiter picks the vendor plugin type once config is ready.

| Module | Type, phase | Platforms | Role |
|---|---|---|---|
| `DrsXrArbiter` | Runtime, EarliestPossible | Win64, Android | XR arbiter: desktop OpenXR runtime and vendor plugin selection; early XR state shared by the others |
| `DrsXrPlatform` | Runtime, PostConfigInit | Win64, Android | Device state and the `UDrsXrPlatformLibrary` API |
| `DrsXrHandTrackingMeta` | ClientOnly, PostConfigInit | Win64, Android | Meta hand-tracking provider |
| `DrsXrMixedReality` | Runtime, PostConfigInit | Win64, Android (passthrough on Android only) | `UDrsXrPassthroughSubsystem` |
| `DrsXrPlatformEditor` | Editor | | Per-user editor setting for the vendor plugin type |
| `DrsXrPlatformTests` | DeveloperTool | | `Drs.XrPlatform.*` tests |

**MetaXR is required for now.** `DrsXrPlatform` and `DrsXrHandTrackingMeta` link `OculusXRHMD`/`OculusXRInput`, and the `.uplugin` lists `OculusXR`. This breaks the Platform rule that vendor code must build without its private submodule; a follow-up PR makes them optional or guarded. MetaXR is **not arbitrated**: it loads as normal. The fork never loads OVRPlugin (a `DRS:` patch in `FOculusXRHMDModule::PreInit`), since it runs on Epic's OpenXR only (`XrApi=NativeOpenXR`). There is no Pico vendor plugin yet, but the Pico vendor entry and the Pico runtimes stay in config, so one can be added without code changes. A second pass is planned to replace the remaining MetaXR uses with native OpenXR extensions, so the framework works the same with MetaXR arbitrated, unarbitrated or absent.

## DrsXrArbiter: the XR arbiter

Makes two independent choices: the desktop OpenXR runtime (at startup, see the next section) and which vendor XR plugin's modules load, so that competing vendor plugins (Meta, Pico) can be enabled together. Plugin arbitration is optional: today no vendor plugin is integrated, so it finds nothing to arbitrate. The integration steps for vendors are in the comment at the top of `Public/DrsXrArbiter/DrsXrArbiter.h`.

- **Vendors are names, not an enum.** A vendor's ID (`Meta`, `Pico`) is its `Id` in the vendor table, and is what `.uplugin` files, overrides, `GetXrPluginType()` and `FXrSelection` use. Adding a vendor needs config only.
- **Opting in:** in a `.uplugin`, set a module's `LoadingPhase` to `None`, put its real phase in `ArbitratedLoadingPhase`, and name its vendor in `ArbitratedXrPlugin`, on the module or at the plugin's top level. The arbiter reads the raw descriptor JSON because the engine drops unknown keys. `EarliestPossible` can't be arbitrated.
- **Vendor shaders need an unarbitrated mapping.** A vendor module's global shaders register as soon as its DLL loads, which happens whenever another module links it (as `DrsXrPlatform` links `OculusXRHMD`), even if the arbiter never starts it. Without the shader directory mapping the editor then fails fatally at startup. So an integrated vendor plugin needs an unarbitrated module that maps its shader directory at `PostConfigInit`. (This was tried with MetaXR and reverted when MetaXR went back to loading as normal.)
- **Choosing the vendor** (`Private/DrsXrArbiter.cpp`), once config is ready at `PostConfigInit`. The vendor table's keywords are matched, ignoring case, and the first vendor with a match wins.
  - Android: the project override, then the device's brand (`ro.product.brand`) and manufacturer. Quest reports `oculus` and `Oculus`.
  - Editor: the editor setting (`UDrsXrPlatformEditorSettings`, restart required), the project override, the selected runtime's `Vendor`, then the device name.
  - Desktop game: `-noHMD` selects none; then `-XrPlugin=<Id>`, the project override, the selected runtime's `Vendor`, and finally the HMD device name once the HMD module is up (too late for that vendor's OpenXR extensions, which only matters over SteamVR, whose system name is `SteamVR/OpenXR : oculus` for a Quest).
  - Commandlets load every vendor, so everything cooks. Dedicated servers load none.
- Phases that pass before the vendor is known are queued and loaded as soon as it is.
- **Config:** `Config/DefaultDrsXrPlatform.ini` holds the vendor table (`Vendors` under `[/Script/DrsXrArbiter.DrsXrArbiterSettings]`: `Id`, `Keywords`, `MarkerModule`) and the runtime table (under `[/Script/DrsXrArbiter.DrsXrRuntimeSettings]`). It is the plugin's own config file (the settings classes have `Config = DrsXrPlatform`), so a project's `Config/DefaultDrsXrPlatform.ini` overrides it, and Project Settings writes there; `!Runtimes=ClearArray` and the like replace a table. Don't move the tables back to a plugin `Config/Game.ini`: the engine applies that over the project's `DefaultGame.ini`, so a project could add entries but not remove, reorder or edit them. Both tables are read through `GConfig` at `EarliestPossible` (plugin config is mounted before any plugin module loads), before reflection can import text, so `FConfigText` (`Public/DrsXrArbiter/DrsXrConfigText.h`) parses the struct text. It reads what Project Settings writes, and the `EditorExport` test checks that. The settings classes only make the tables editable. The vendor table is read once and cached, since `GetXrPluginType()` runs often. Both overrides list the vendor IDs through `UDrsXrArbiterSettings::GetXrPluginOptions` (`GetOptions`); the editor setting names it in full, `DrsXrArbiter.DrsXrArbiterSettings.GetXrPluginOptions`, rather than wrapping it.
- **`GetXrPluginType()` detects the running vendor** from its marker module in the vendor table (`OculusXRHMD`, `PICOOpenXRHMD`), so it is right whether the arbiter or the engine loaded the plugin, and None without one. `GetArbitratedXrPluginType()` is the arbiter's own choice. `FXrSelection` (`Public/DrsXrArbiter/DrsXrSelection.h`) records the runtime and plugin in use and how the plugin came to be (arbitrated, forced or loaded as normal), and `IsConsistent()` checks the last two against the hardware the vendor table expects and the vendor the runtime implies. That check is the only link between the runtime and the plugin (runtime selection ignores the plugin override). `SetupHmd()` calls `FXrSelection::Get().Validate()` once the HMD, and so the device name, is known; `Validate()` logs a warning when the check fails.
- **Shared early state:** `Drs::XrArbiter` also holds the selected runtime's name and vendor, the runtime table's parsing and the selection rule (`ChooseRuntime`), and `IsNonVrMode()`. These are public so that `DrsXrPlatform` and the tests can reach them; `SetXrRuntime` is private to the module.
- It may use `DrsBase` (e.g. `DRS_LOG`; see `Base/CLAUDE.md` for why that is safe this early). It may use UObject types (the settings, and the USTRUCTs as plain data), but must not touch `GEngine` or subsystems before `PostConfigInit`.

## Desktop OpenXR runtime selection

The arbiter's `StartupModule` selects the OpenXR runtime **before OpenXR creates its instance**, by setting `XR_RUNTIME_JSON`, and records it for `GetXrRuntimeType()`. It runs on Windows only, and not on dedicated servers (the rule a `ClientOnly` module would get). No effect on Android.

- **The runtimes are config:** each `Runtimes` entry has a `Name`, the `Vendor` it implies (none for SteamVR), a list of `Manifest` locators tried in order, and the `RunningProcesses` that mean it is running. `RuntimePriority` lists them, one per line, so a project can reorder or remove them in its own `DefaultDrsXrPlatform.ini`.
- **Selection:** a running runtime wins, otherwise the first installed one in priority order. With none installed, the system's active runtime is used. Under `-noHMD` no runtime is selected. The plugin override is not consulted.
- **SteamVR comes first.** Meta's Link software runs from boot, so Oculus always counts as running; with Oculus first, SteamVR could never be chosen. With SteamVR first, launching SteamVR before the app chooses it. SteamVR counts as running only while `vrmonitor.exe` (its status window) is, since `vrserver.exe` can outlive it.
- **Manifest locators** (`EDrsXrManifestLocator`): an environment variable (`OculusBase`), a URI handler (`steamvr:`, which can be missing), Steam's library folders (from `steamapps/libraryfolders.vdf`), Program Files (Pico), and the OpenXR loader's registered runtimes.
- **The code is in `Private/DrsXrRuntimeSelection.*`** under `Drs::XrRuntime`, with the Windows parts (the locators and process checks) inside `PLATFORM_WINDOWS`. It runs before the engine is up, so it uses no UObjects (the runtime USTRUCTs are used as plain data) and reads options from the command line. It logs with its own category, `LogDrsXrRuntime`, which `Tools/CheckXrRuntime.ps1` looks for.
- **Checking it on a real machine:** the `Drs.XrPlatform.Arbiter.*` tests cover the selection rule with fake inputs; `Tools/CheckXrRuntime.ps1` covers the real locators and process checks. After changing the runtime table, the locators or the selection, run it three ways: with SteamVR open (`-Expect SteamVR`), with SteamVR fully closed so `vrmonitor.exe` has exited (`-Expect Oculus`, with Meta's Link software installed), and with `-Expect None -ExtraArgs '-noHMD'`. No headset is needed, since the runtime is selected before OpenXR starts; without one, OpenXR then fails to start, which is expected.

## DrsXrPlatform: device state and API

- **Higher layers call `UDrsXrPlatformLibrary`** instead of the engine's XR APIs, so vendor and runtime details stay in this plugin. It is a static library: there is no XR subsystem.
- **The state lives in the module** (`FDrsXrPlatformModule::GetDeviceInfo()`), not a subsystem, because `FXrDeviceInfo` must register as an OpenXR extension plugin at `PostConfigInit`, before engine subsystems exist. That is also why the module loads at `PostConfigInit`. There is no device info on servers or in commandlets; the library returns defaults there.
- `FXrDeviceInfo` tracks the worn state (`XR_EXT_user_presence`, else the session state), the system menu (several frames in `VISIBLE`), focus, display refresh rate (`XR_FB_display_refresh_rate`) and controller modes. `FXrDelegates` broadcasts the changes.
- `FXrControllerMonitor` decides each hand's mode from its interaction profile, falling back to joint tracking. It is public and takes an `IXrControllerSource`, so the tests drive it without a headset.
- Vendor hand tracking comes from an `IXrHandTrackingProvider` modular feature, registered by an arbitrated vendor module.
- `SetFloorTrackingOrigin()` asks for OpenXR `LOCAL_FLOOR`, which the runtime may replace with `LOCAL`.
- Config: `Config/Input.ini` sets the OpenXRHandTracking source names that `GetMotionControllerSource` relies on.

## DrsXrMixedReality: passthrough

`UDrsXrPassthroughSubsystem` (game instance) turns passthrough on and off through requests and suspensions; `FXrPassthroughBackend` switches the OpenXR environment blend mode. Passthrough is compiled in only on Android (`DRS_PASSTHROUGH_SUPPORTED`) and needs `xr.OpenXRInvertAlpha` and `r.Mobile.PropagateAlpha`. It is suspended while the `UDrsViewFadeLibrary` fade is opaque, and blending uses the fade's holdout.

## Tests

`DrsXrPlatformTests` holds `Drs.XrPlatform.ControllerMonitor` (an Automation Spec), `Drs.XrPlatform.Arbiter.*` (config text parsing, the vendor and runtime tables, keyword matching, runtime selection, the Project Settings export round trip, and `Arbiter.Selection` for `FXrSelection::IsConsistent`).
