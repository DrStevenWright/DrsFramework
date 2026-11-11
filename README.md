# DrsFramework

An XR framework for Unreal Engine 5.8, built as a set of layered plugins.
`DrsCore` is the entry point: enabling it in a project enables the whole framework.

Targets: Meta Quest (Link and standalone Android), with Pico as a cross-platform test case.

## Layers

Each layer may depend only on the layers below it. Dependencies within a layer must be
acyclic and listed in [`Tools/LayerRules.json`](Tools/LayerRules.json).

| Layer | Folder | Plugins |
|---|---|---|
| Game | `Game/` | `DrsCore`: game classes (GameMode, GameState, PlayerState, PlayerController, Pawn, GameInstance) and communication between otherwise independent plugins |
| GameSupport | `GameSupport/` | `DrsXrGame`, `DrsInteraction`, `DrsLocomotion`, `DrsSpawning` |
| Platform | `Platform/` | `DrsXrPlatform`; vendor plugin forks in `Platform/External/` |
| Common | `Common/` | `DrsUtilities` (independent modules such as `DrsMathUtilities`, `DrsGraphicsUtilities`) |
| Base | `Base/` | `DrsGlobals` (`DrsGlobals`, `DrsBaseUtilities`, `DrsGlobalsTests`) |

## Naming

- Plugins, modules, types, repos and projects use the `Drs` prefix: `DrsCore`, `ADrsPawn`, `LogDrsXrPlatform`.
- `DRS_` is used only for macros, e.g. `DRS_LOG(LogDrsCore, Log, TEXT("..."))`.

## Using it in a project

DrsFramework is added to a project as a submodule at `Plugins/DrsFramework`:

```
git clone --recurse-submodules https://github.com/DrStevenWright/DrsTestProject.git
cd DrsTestProject
powershell -ExecutionPolicy Bypass -File Plugins/DrsFramework/Tools/Bootstrap.ps1 -Build
```

Projects: [DrsTestProject](https://github.com/DrStevenWright/DrsTestProject) (development and tests),
[DrsProject](https://github.com/DrStevenWright/DrsProject) (minimal starter).

## Checking the layer rules

```
powershell -ExecutionPolicy Bypass -File Tools/CheckLayers.ps1
```

## Licence

MIT. See [LICENSE](LICENSE). The vendor plugins in `Platform/External/` keep their own licences.
