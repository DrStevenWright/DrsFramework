# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

DrsFramework is an XR framework for Unreal Engine 5.8, built as a set of layered plugins. It targets Meta Quest (over Link and standalone Android), with Pico as a cross-platform test case. It never builds on its own: a host project includes it as a git submodule at `Plugins/DrsFramework`. Development and tests happen in [DrsTestProject](https://github.com/DrStevenWright/DrsTestProject); [DrsProject](https://github.com/DrStevenWright/DrsProject) is the minimal starter.

`DrsCore` is the entry point. Its `.uplugin` lists every other framework plugin, so a project only enables `DrsCore`. **When you add a plugin, also add it to `Game/DrsCore/DrsCore.uplugin`.**

The framework must not depend on any host project or on the host's dev-only plugins (`Plugins/Dev/` in DrsTestProject, e.g. MCP tooling).

## Commands

Check the layer rules (run from this repo's root; exits 1 on a violation). Run it after any change to a `.Build.cs` or `.uplugin` dependency:

```
powershell -ExecutionPolicy Bypass -File Tools/CheckLayers.ps1
```

Build from the **host project's** root. Bootstrap fetches submodules, finds the engine from the `.uproject`'s `EngineAssociation` (launcher install or a registered source build), generates project files and, with `-Build`, builds `<Project>Editor` Win64 Development:

```
powershell -ExecutionPolicy Bypass -File Plugins/DrsFramework/Tools/Bootstrap.ps1 -Build
```

Options: `-EngineDir <path>`, `-SkipSubmodules`, `-ProjectFile <path>`.

Run the tests through the host project. They are UE Automation tests named `Drs.*`. `RunTests` takes a name prefix, so pass a full test name to run a single test (e.g. `Drs.Globals.Version`):

```
UnrealEditor-Cmd.exe <host>.uproject -ExecCmds="Automation RunTests Drs; Quit" -unattended -nullrhi -nosplash
```

## Tests

Tests use `IMPLEMENT_SIMPLE_AUTOMATION_TEST` with `EditorContext | ProductFilter`, inside `#if WITH_DEV_AUTOMATION_TESTS`, and are named `Drs.<Plugin>.<Area>`. They live in a `DeveloperTool` module whose name ends in `Tests`, inside the plugin they test; `Base/DrsGlobals/Source/DrsGlobalsTests` is the example to follow.

## Layers

From lowest to highest. A module may depend only on lower layers. `Tools/CheckLayers.ps1` enforces this using `Tools/LayerRules.json`, where the longest matching folder path decides a plugin's layer.

| Layer | Folder | Contents |
|---|---|---|
| External | `Platform/External/` | Private vendor plugin forks (MetaXR, Pico) as submodules. Only Platform code may use them, and they must not depend on any Drs module. Clones without access won't fetch them. |
| Base | `Base/` | `DrsGlobals`: logging (`LogDrs`, `DRS_LOG`), framework version (`DrsVersion.h`), `DrsBaseUtilities`, `DrsGlobalsTests` |
| Common | `Common/` | `DrsUtilities`: independent modules (`DrsMathUtilities`, `DrsGraphicsUtilities`) |
| Platform | `Platform/` | `DrsXrPlatform`: XR platform subsystem over OpenXR. Its `DrsXrRuntime` module (Win64, `EarliestPossible`) sets `XR_RUNTIME_JSON` to pick the Meta OpenXR runtime before OpenXR starts; `-DrsKeepXrRuntime` disables this. |
| GameSupport | `GameSupport/` | `DrsXrGameSupport` (tracked placeholder hands), and `DrsInteraction`, `DrsLocomotion`, `DrsSpawning`, which build on it |
| Game | `Game/` | `DrsCore`: game classes (GameMode/State, PlayerController/State, Pawn) and the place where otherwise independent GameSupport plugins talk to each other |

The checker also enforces these rules:
- A same-layer dependency must be listed in `AllowedSameLayer` (format `"A -> B"`), and the module graph must have no cycles.
- A module that uses another framework plugin's module must list that plugin in its own `.uplugin`.
- A `.uplugin` must not list a plugin from a higher layer.
- A `*Tests` module may depend on any module in its own plugin.

Dependencies on engine modules are not checked.

## Conventions

- **Code style:** follow [`Docs/CodingConventions.md`](Docs/CodingConventions.md), which covers naming, the file header, and logging. In short: Epic's standard with the `Drs` prefix, except that arguments and locals are camelCase with no `b`, and every code file starts with the MIT copyright line.
- **Git:** use Conventional Commits for branch names (`feat/…`, `fix/…`, `chore/…`, `docs/…`, `refactor/…`, `test/…`, `ci/…`), commit subjects and PR titles (`feat(scope): summary`). Mark breaking API changes with `!` or a `BREAKING CHANGE:` footer, because host projects must be updated for them.
- **Working from a host project:** the submodule is often on a detached HEAD, so check out a branch before committing. PR against this repo's `main`, then update the submodule pointer in the host project on its own branch.
