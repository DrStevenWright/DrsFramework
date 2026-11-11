# DrsCore

The framework's entry point. A project enables only this plugin.

- **`DrsCore.uplugin` lists every other framework plugin** so that enabling `DrsCore` enables the whole framework. When you add a framework plugin, add it here too, or projects won't load it.
- **`DrsCore.Build.cs` lists feature modules as private dependencies** (currently `DrsXrPlatform` and `DrsXrGame`; add a feature module when DrsCore starts using it). Its public headers only forward-declare their types, so projects that include `DrsCore` headers don't pull in every feature's headers. Keep it that way: forward-declare in headers and include in `.cpp` files.
- **Each game class has a Blueprint in the plugin's `Content/` folder:** `BP_DrsGameMode`, `BP_DrsGameState`, `BP_DrsPlayerController`, `BP_DrsPlayerState`, `BP_DrsPawn` and `BP_DrsGameInstance` (`/DrsCore/BP_Drs...`). They hold asset defaults that C++ shouldn't hard-code. `BP_DrsGameMode`'s defaults point at the other Blueprints (pawn, controller, game state, player state); the C++ `ADrsGameMode` keeps the C++ classes as its defaults. When you add a game class, add its Blueprint and wire it in.
- **The Blueprints are a project-facing contract.** Host projects' config refers to them by path. For example, DrsTestProject's `DefaultEngine.ini` has `GlobalDefaultGameMode=/DrsCore/BP_DrsGameMode.BP_DrsGameMode_C` and `GameInstanceClass=/DrsCore/BP_DrsGameInstance.BP_DrsGameInstance_C`. Renaming or moving a Blueprint or a game class breaks host projects: mark it as a breaking change, or add a redirector (assets) or `CoreRedirects` entry (classes).

## Game classes

- `ADrsGameMode` sets the framework's default classes (pawn, controller, states). The state classes, the player controller and `UDrsGameInstance` are empty for now. The game instance isn't set by the game mode; a project selects it with `GameInstanceClass` in its config.
- `ADrsPawn` is the XR pawn and the place where features are put together. It has:
  - a floor-level `TrackingOrigin` with an HMD-driven `Camera`;
  - left and right `UMotionControllerComponent`s, each with a `UDrsHandMeshComponent` attached.

  New features are normally added to the pawn as components here, not by making features depend on each other.
- **Tracking origin:** in `BeginPlay`, `ADrsPawn` calls `UDrsXrPlatformSubsystem::SetFloorTrackingOrigin()`. The tracking origin is engine-wide, so only the locally viewed pawn may set it. The pawn isn't possessed yet at `BeginPlay`, so the code checks `GetNetMode() != NM_DedicatedServer` rather than the controller. Keep this in mind when you add other engine-wide XR setup.
