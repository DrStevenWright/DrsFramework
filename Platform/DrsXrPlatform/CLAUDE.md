# DrsXrPlatform

Two modules with different load rules.

## DrsXrPlatform (Runtime, Default)

- `UDrsXrPlatformSubsystem` is an **engine** subsystem, not a world subsystem, because the XR system exists for the engine's whole lifetime. Get it with `GEngine->GetEngineSubsystem<UDrsXrPlatformSubsystem>()`.
- It wraps the engine's XR tracking system (`HeadMountedDisplay`, `XRBase`, `OpenXR`). It answers whether XR is active and what it's called, and it sets up the tracking space. `SetFloorTrackingOrigin()` asks for OpenXR `LOCAL_FLOOR`, which the runtime may replace with `LOCAL`.
- Higher layers should call this subsystem instead of using the engine's XR APIs directly. That keeps vendor and runtime details in this plugin.

## DrsXrRuntime (ClientOnly, EarliestPossible, Win64 only)

Selects the desktop OpenXR runtime **before the OpenXR plugin creates its instance at PostConfigInit**. It does this by setting `XR_RUNTIME_JSON` for the process. It currently always picks the Meta (Oculus) runtime when it's installed, even if a different runtime is the system's active one. `-DrsKeepXrRuntime` turns this off.

Because it loads so early:
- **Depend on `Core` only.** Don't use `DrsGlobals`, `DRS_LOG` or UObjects. Log with `UE_LOG(LogDrsXrRuntime, ...)`; the conventions doc lists this as an allowed exception.
- **No engine subsystems, `GEngine` or config-driven settings,** because none of them exist yet. Read options from the command line (`FParse::Param`).
- The Win64 logic (finding the manifest under `OculusBase` or in the OpenXR loader's registry entries) is in `Private/DrsXrRuntimeUtils.*` under `Drs::XrRuntime`. Keep platform-specific code there, so the module startup stays readable.
- It has no effect on Android, where the device provides the runtime.
