# DrsXrPlatform

Seven modules with different load rules. Startup order matters: `DrsXrArbiter` and `DrsXrRuntime` run at `EarliestPossible`, OpenXR creates its instance at `PostConfigInit`, and the arbiter picks the vendor plugin type once config is ready.

| Module | Type, phase | Platforms | Role |
|---|---|---|---|
| `DrsXrArbiter` | Runtime, EarliestPossible | Win64, Android | Plugin arbiter; early XR state shared by the others |
| `DrsXrRuntime` | ClientOnly, EarliestPossible | Win64 | Desktop OpenXR runtime selection |
| `DrsXrPlatform` | Runtime, PostConfigInit | Win64, Android | Device state and the `UDrsXrPlatformLibrary` API |
| `DrsXrHandTrackingMeta` | ClientOnly, PostConfigInit | Win64, Android | Meta hand-tracking provider |
| `DrsXrMixedReality` | Runtime, PostConfigInit | Win64, Android (passthrough on Android only) | `UDrsXrPassthroughSubsystem` |
| `DrsXrPlatformEditor` | Editor | | Per-user editor setting for the vendor plugin type |
| `DrsXrPlatformTests` | DeveloperTool | | `Drs.XrPlatform.*` tests |

**MetaXR is required for now.** `DrsXrPlatform` and `DrsXrHandTrackingMeta` link `OculusXRHMD`/`OculusXRInput`, and the `.uplugin` lists `OculusXR`. This breaks the Platform rule that vendor code must build without its private submodule; a follow-up PR makes them optional or guarded. MetaXR is **not arbitrated**: it loads as normal. The fork never loads OVRPlugin (a `DRS:` patch in `FOculusXRHMDModule::PreInit`), since it runs on Epic's OpenXR only (`XrApi=NativeOpenXR`). There is no Pico vendor plugin yet, but the Pico plugin type, its arbitration rules and Pico runtime detection stay, so one can be added without code changes. A second pass is planned to replace the remaining MetaXR uses with native OpenXR extensions, so the framework works the same with MetaXR arbitrated, unarbitrated or absent.

## DrsXrArbiter: the plugin arbiter

Decides which vendor XR plugin's modules load, so that competing vendor plugins (Meta, Pico) can be enabled together. It is optional: today no vendor plugin is integrated, so it finds nothing to arbitrate and does nothing. The integration steps for vendors are in the comment at the top of `Public/DrsXrArbiter.h`.

- **Opting in:** in a `.uplugin`, set a module's `LoadingPhase` to `None`, put its real phase in `ArbitratedLoadingPhase`, and name its type (`Meta`, `Pico`) in `ArbitratedXrPlugin`, on the module or at the plugin's top level. The arbiter reads the raw descriptor JSON because the engine drops unknown keys. `EarliestPossible` can't be arbitrated.
- **Vendor shaders need an unarbitrated mapping.** A vendor module's global shaders register as soon as its DLL loads, which happens whenever another module links it (as `DrsXrPlatform` links `OculusXRHMD`), even if the arbiter never starts it. Without the shader directory mapping the editor then fails fatally at startup. So an integrated vendor plugin needs an unarbitrated module that maps its shader directory at `PostConfigInit`. (This was tried with MetaXR and reverted when MetaXR went back to loading as normal.)
- **Choosing the type** (`Private/DrsXrArbiter.cpp`), once config is ready at `PostConfigInit`:
  - Android: the project override, then `MobileDeviceArbitrationRules` (device make, model, GPU).
  - Editor: the editor setting (`UDrsXrPlatformEditorSettings`, restart required), the project override, then the runtime and device-name rules.
  - Desktop game: `-noHMD` selects none; then `-DrsXrPlugin=<Type>`, the project override, `TetheredDeviceArbitrationRules` by runtime, and finally by HMD device name once the HMD module is up (too late for that vendor's OpenXR extensions, which only matters over SteamVR).
  - Commandlets load every type, so everything cooks. Dedicated servers load none.
- Phases that pass before the type is known are queued and loaded as soon as it is.
- **Config:** the rules are in `Config/Game.ini` under `[/Script/DrsXrArbiter.DrsXrArbiterSettings]`. The arbiter reads them through `GConfig` because it decides before `UDrsXrArbiterSettings` loads; the class only makes them editable in Project Settings.
- **`GetXrPluginType()` detects the running vendor** from its loaded modules (`OculusXRHMD`, `PICOOpenXRHMD`), so it is right whether the arbiter or the engine loaded the plugin, and None without one. `GetArbitratedXrPluginType()` is the arbiter's own choice. `FDrsXrConfiguration` records how the plugin came to be in use (arbitrated, forced or loaded as normal) and checks the last two against the hardware the rules expect.
- **Shared early state:** `Drs::XrArbiter` also holds the selected runtime type and flags (set by `DrsXrRuntime`) and `IsNonVrMode()`. The UENUMs (`EDrsXrPluginType`, `EDrsXrRuntimeType`) live here so that every module can use them.
- It may use `DrsBase` (e.g. `DRS_LOG`; see `Base/CLAUDE.md` for why that is safe this early). It may use UObject types (the settings and UENUMs), but must not touch `GEngine` or subsystems before `PostConfigInit`.

## DrsXrRuntime: desktop runtime selection

Selects the OpenXR runtime **before OpenXR creates its instance**, by setting `XR_RUNTIME_JSON`, and records it with `Drs::XrArbiter::SetXrRuntime`. `-DrsKeepXrRuntime` turns this off. No effect on Android.

- Candidates, in order: a forced Meta type gives Oculus, then SteamVR; a forced Pico type gives SteamVR, Pico Business Streaming, PICO Connect. Otherwise a running Pico client means SteamVR, then the Pico runtimes (and the expected runtime is Pico); else Oculus, the Pico runtimes, then SteamVR. A running runtime wins over an installed one. SteamVR is skipped in non-VR mode.
- Runtime flags: SteamVR is `NonVrNotSupported`; the Pico runtimes are `ControllerLosesFocus`.
- **Depend on `Core`, `DrsBase` and `DrsXrArbiter` only**, use no UObjects, and log with `DRS_LOG(LogDrsXrRuntime, ...)`. Read options from the command line.
- The Win64 logic (manifest search under `OculusBase` and the OpenXR loader's registry entries, the SteamVR URI handler, Program Files for Pico, process checks) is in `Private/DrsXrRuntimeUtils.*` under `Drs::XrRuntime`.

## DrsXrPlatform: device state and API

- **Higher layers call `UDrsXrPlatformLibrary`** instead of the engine's XR APIs, so vendor and runtime details stay in this plugin. It is a static library: there is no XR subsystem.
- **The state lives in the module** (`FDrsXrPlatformModule::GetDeviceInfo()`), not a subsystem, because `FDrsXrDeviceInfo` must register as an OpenXR extension plugin at `PostConfigInit`, before engine subsystems exist. That is also why the module loads at `PostConfigInit`. There is no device info on servers or in commandlets; the library returns defaults there.
- `FDrsXrDeviceInfo` tracks the worn state (`XR_EXT_user_presence`, else the session state), the system menu (several frames in `VISIBLE`), focus, display refresh rate (`XR_FB_display_refresh_rate`) and controller modes. `FDrsXrDelegates` broadcasts the changes.
- `FDrsXrControllerMonitor` decides each hand's mode from its interaction profile, falling back to joint tracking. It is public and takes an `IDrsXrControllerSource`, so the tests drive it without a headset.
- Vendor hand tracking comes from an `IDrsXrHandTrackingProvider` modular feature, registered by an arbitrated vendor module.
- `SetFloorTrackingOrigin()` asks for OpenXR `LOCAL_FLOOR`, which the runtime may replace with `LOCAL`.
- Config: `Config/Input.ini` sets the OpenXRHandTracking source names that `GetMotionControllerSource` relies on.

## DrsXrMixedReality: passthrough

`UDrsXrPassthroughSubsystem` (game instance) turns passthrough on and off through requests and suspensions; `FDrsXrPassthroughBackend` switches the OpenXR environment blend mode. Passthrough is compiled in only on Android (`DRS_PASSTHROUGH_SUPPORTED`) and needs `xr.OpenXRInvertAlpha` and `r.Mobile.PropagateAlpha`. It is suspended while the `UDrsViewFadeLibrary` fade is opaque, and blending uses the fade's holdout.

## Tests

`DrsXrPlatformTests` holds `Drs.XrPlatform.ControllerMonitor` (an Automation Spec), `Drs.XrPlatform.Arbiter.*` (rule parsing and matching) and `Drs.XrPlatform.Configuration.IsValid`.
