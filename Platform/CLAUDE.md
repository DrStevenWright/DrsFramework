# Platform layer

The framework's boundary with the XR runtime and the vendor SDKs. Modules here may depend on Base, Common, engine XR modules (`HeadMountedDisplay`, `XRBase`, `OpenXR`) and `External/`.

**Belongs here:** everything that knows which XR system, runtime or vendor is in use: device and tracking queries, tracking-space setup, runtime selection and vendor-specific code. Today: `DrsXrPlatform` (see its own CLAUDE.md).

**Doesn't belong here:** gameplay. Components, pawns and per-world services go in `GameSupport/` or `Game/`, and call into this layer.

- **Only this layer may use `External/`.** The vendor forks (MetaXR, Pico) are private submodules and may be missing in a clone. Code that uses them must still build without them, or the plugin must be optional.
- The vendor plugins in `External/` must not depend on any Drs module, and they keep their own style and licences. Don't reformat them.
- Keep vendor-specific code behind framework APIs, so that higher layers can stay vendor-neutral.
- Platform code is local-only: it holds no replicated state. Say so in each class's doc comment, as `UDrsXrPlatformSubsystem` does.
