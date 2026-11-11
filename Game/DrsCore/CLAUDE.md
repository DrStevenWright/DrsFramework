# DrsCore

The framework's entry point. A project enables only this plugin.

- **`DrsCore.uplugin` lists every other framework plugin** so that enabling `DrsCore` enables the whole framework. When you add a framework plugin, add it here too, or projects won't load it.
- **`DrsCore.Build.cs` lists feature modules as private dependencies** (`DrsXrPlatform`, `DrsXrGameSupport`, `DrsInteraction`, `DrsLocomotion`, `DrsSpawning`). Its public headers only forward-declare their types, so projects that include `DrsCore` headers don't pull in every feature's headers. Keep it that way: forward-declare in headers and include in `.cpp` files.
- **Host projects' config refers to these classes by path.** For example, DrsTestProject's `DefaultEngine.ini` has `GlobalDefaultGameMode=/Script/DrsCore.DrsGameMode`. Renaming or moving a game class breaks host projects: mark it as a breaking change, or add a `CoreRedirects` entry.

## Game classes

- `ADrsGameMode` sets the framework's default classes (pawn, controller, states). The state classes and the player controller are empty for now.
- `ADrsPawn` is the XR pawn and the place where features are put together. It has:
  - a floor-level `TrackingOrigin` with an HMD-driven `Camera`;
  - left and right `UMotionControllerComponent`s, each with a `UDrsHandPlaceholderComponent` attached;
  - a `UDrsInteractorComponent` and a `UDrsLocomotionComponent`.

  New features are normally added to the pawn as components here, not by making features depend on each other.
- **Tracking origin:** in `BeginPlay`, `ADrsPawn` calls `UDrsXrPlatformSubsystem::SetFloorTrackingOrigin()`. The tracking origin is engine-wide, so only the locally viewed pawn may set it. The pawn isn't possessed yet at `BeginPlay`, so the code checks `GetNetMode() != NM_DedicatedServer` rather than the controller. Keep this in mind when you add other engine-wide XR setup.
