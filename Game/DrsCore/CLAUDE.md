# DrsCore

The framework's entry point. A project enables only this plugin.

- **`DrsCore.uplugin` lists every other framework plugin** so that enabling `DrsCore` enables the whole framework. When you add a framework plugin, add it here too, or projects won't load it.
- **`DrsCore.Build.cs` lists feature modules as private dependencies** (currently `DrsInput`, `DrsXrPlatform` and `DrsXrGame`; add a feature module when DrsCore starts using it). Its public headers only forward-declare their types, so projects that include `DrsCore` headers don't pull in every feature's headers. Keep it that way: forward-declare in headers and include in `.cpp` files.
- **Each game class has a Blueprint in the plugin's `Content/` folder:** `BP_DrsGameMode`, `BP_DrsGameState`, `BP_DrsPlayerController`, `BP_DrsPlayerState`, `BP_DrsPawn` and `BP_DrsGameInstance` (`/DrsCore/BP_Drs...`). They hold asset defaults that C++ shouldn't hard-code. `BP_DrsGameMode`'s defaults point at the other Blueprints (pawn, controller, game state, player state); the C++ `ADrsGameMode` keeps the C++ classes as its defaults. When you add a game class, add its Blueprint and wire it in.
- **The Blueprints are a project-facing contract.** Host projects' config refers to them by path. For example, DrsTestProject's `DefaultEngine.ini` has `GlobalDefaultGameMode=/DrsCore/BP_DrsGameMode.BP_DrsGameMode_C` and `GameInstanceClass=/DrsCore/BP_DrsGameInstance.BP_DrsGameInstance_C`. Renaming or moving a Blueprint or a game class breaks host projects: add a redirector (assets) or `CoreRedirects` entry (classes), or list the change under host-project updates in the PR description (see the Git convention in the root CLAUDE.md).

## Game classes

- `ADrsGameMode` sets the framework's default classes (pawn, controller, states). The state classes, the player controller and `UDrsGameInstance` are empty for now. The game instance isn't set by the game mode; a project selects it with `GameInstanceClass` in its config.
- `ADrsPawn` is the XR pawn and the place where features are put together. It has:
  - a floor-level `TrackingOrigin` with an HMD-driven `Camera`;
  - left and right `UMotionControllerComponent`s, each with a `UDrsHandMeshComponent` attached;
  - left and right `UDrsMotionControllerInputComponent`s, which forward each hand's motion controller input as device-agnostic keys. The pawn adds its `MotionControllerContext` (`IMC_MotionController`, an asset default on `BP_DrsPawn`) to the controlling local player at the highest priority, and binds the components, in `SetupPlayerInputComponent` (the engine calls it only for a locally controlled pawn, once the controller is known). It undoes both in `UnPossessed` and `EndPlay`.
  - **A project must also list `IMC_MotionController` in `DefaultMappingContexts`** (`EnhancedInputDeveloperSettings`, with `bAddImmediately=False`). OpenXR builds its actions and interaction profile suggestions from those contexts, so without the entry the runtime never sees the motion controller bindings.
  - **The pawn's input component is created from the project's `DefaultInputComponentClass`,** so a project must set it to `/Script/DrsInput.DrsEnhancedInputComponent` (and `DefaultPlayerInputClass` to `/Script/DrsInput.DrsEnhancedPlayerInput`) in `DefaultInput.ini`. Otherwise the bindings are skipped and an error is logged.

  New features are normally added to the pawn as components here, not by making features depend on each other.
- **HMD setup:** in `BeginPlay`, `ADrsPawn` calls `UDrsXrPlatformLibrary::SetupHmd()`, which enables the HMD and stereo, sets the floor tracking origin and checks the XR selection (`FXrSelection::Validate`). These are engine-wide, so only a client may set them up. The pawn isn't possessed yet at `BeginPlay`, so the code checks `!IsNetMode(NM_DedicatedServer)` rather than the controller (outside the editor it reduces to `IsRunningDedicatedServer()`). Keep this in mind when you add other engine-wide XR setup.
