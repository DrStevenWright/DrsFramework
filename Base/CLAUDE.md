# Base layer

The lowest framework layer. Everything else may depend on it, and it depends on no framework module, only on engine modules (`Core`, `CoreUObject`).

**Belongs here:** framework-wide plumbing that every module needs and that has no XR, gameplay or platform knowledge. Today that is the `DrsBase` plugin, with one runtime module, also `DrsBase`: the `LogDrs` category and the `DRS_LOG` macros (`DrsLog.h`: `DRS_LOG`, `DRS_CLOG`, their `_ONCE` variants and `DRS_LOG_GLOBAL`, all taking a plain format literal), the framework version (`DrsVersion.h`), and generic helpers: `Drs::GetDebugName` (`DrsObjectUtils.h`), `Drs::EnumToString` (`DrsEnumToString.h`), `TDrsVotableState` (`DrsVotableState.h`) and `Drs::AsNative` for Blueprint-to-native delegates (`DrsDelegateUtils.h`).

**Doesn't belong here:** anything that only some modules need. Maths, graphics and similar helpers go in `Common/`; XR or engine-subsystem code goes in `Platform/` or higher.

- Keep it small and stable. A change here rebuilds the whole framework, and an API change affects every module.
- Add new helpers to the `DrsBase` module rather than a new module: a module here costs every consumer a dependency, and the helpers are small.
- **`DrsBase` has no startup code** (`FDefaultModuleImpl`, static data only). That is what lets the `EarliestPossible` modules in `DrsXrPlatform` (`DrsXrArbiter`, `DrsXrRuntime`) use it: the OS loads `DrsBase`'s DLL as an import of theirs, before the module manager loads `DrsBase` at its own phase. Keep it that way; anything that needs startup work belongs in another module.
- `DrsBaseTests` is the example test module for the whole framework. Keep it in line with the Tests section of the root CLAUDE.md.
- The framework version lives in `DrsVersion.h` and must match `VersionName` in the plugins' `.uplugin` files.
