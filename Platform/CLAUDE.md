# Platform layer

The framework's boundary with the XR runtime and the vendor SDKs. Modules here may depend on Base, Common and engine XR modules (`HeadMountedDisplay`, `XRBase`, `OpenXR`).

**Belongs here:** everything that knows which XR system, runtime or vendor is in use: device and tracking queries, tracking-space setup, runtime selection and vendor-specific code. Today: `DrsXrPlatform` (see its own CLAUDE.md), including the XR arbiter (runtime and vendor plugin selection) and passthrough.

**Doesn't belong here:** gameplay. Components, pawns and per-world services go in `GameSupport/` or `Game/`, and call into this layer.

- **No vendor plugin dependencies.** Vendor XR plugins (MetaXR, Pico) live in host projects and may be absent, so no Drs module links them or lists them in a `.uplugin`. Use the OpenXR extension instead, checked with `IsExtensionEnabled` and the system properties, so the code works whoever provides it.
- Keep XR details behind framework APIs, so that higher layers can stay vendor-neutral.
- Platform code is local-only: it holds no replicated state. Say so in each class's doc comment, as `UDrsXrPlatformLibrary` does.
