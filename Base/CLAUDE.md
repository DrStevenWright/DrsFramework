# Base layer

The lowest framework layer. Everything else may depend on it, and it depends on no framework module, only on engine modules (`Core`, `CoreUObject`).

**Belongs here:** framework-wide plumbing that every module needs and that has no XR, gameplay or platform knowledge. Today that means `DrsGlobals`: the `LogDrs` category and the `DRS_LOG` macros, the framework version (`DrsVersion.h`), and `DrsBaseUtilities` (generic object helpers such as `Drs::GetDebugName`).

**Doesn't belong here:** anything that only some modules need. Maths, graphics and similar helpers go in `Common/`; XR or engine-subsystem code goes in `Platform/` or higher.

- Keep it small and stable. A change here rebuilds the whole framework, and an API change affects every module.
- Same-layer dependency allowed in `Tools/LayerRules.json`: `DrsBaseUtilities -> DrsGlobals`.
- `DrsGlobalsTests` is the example test module for the whole framework. Keep it in line with the Tests section of the root CLAUDE.md.
- The framework version lives in `DrsVersion.h` and must match `VersionName` in the plugins' `.uplugin` files.
