# GameSupport layer

Reusable gameplay building blocks: components and world subsystems that a game assembles. Modules here may depend on Base, Common and Platform.

**Belongs here:** one feature per plugin, usable on its own. Today:
- `DrsXrGameSupport`: shared XR gameplay pieces. It contains `UDrsXrGameSupportSubsystem` (per world: is XR active for this game?) and `UDrsHandPlaceholderComponent` with `EDrsHand`.
- `DrsInteraction`: `UDrsInteractorComponent` (placeholder).
- `DrsLocomotion`: `UDrsLocomotionComponent` (placeholder).
- `DrsSpawning`: `UDrsSpawningSubsystem` (placeholder).

**Doesn't belong here:** code that wires several features together, or game classes such as pawns and game modes. That goes in `Game/DrsCore`.

- **Feature plugins don't depend on each other.** The only same-layer dependencies allowed are on `DrsXrGameSupport`: `DrsInteraction`, `DrsLocomotion` and `DrsSpawning -> DrsXrGameSupport`. If two features need to work together, connect them in `DrsCore`. Don't add an `AllowedSameLayer` entry between features without discussing it first.
- Shared types that several features need (like `EDrsHand`) go in `DrsXrGameSupport`.
- Components use `ClassGroup = (Drs)` and `meta = (BlueprintSpawnableComponent)`. UFUNCTION/UPROPERTY categories are `Drs|<Feature>`.
- World subsystems override `DoesSupportWorldType` to choose which world types they run in.
- Say in each class's doc comment whether it holds replicated state ("Local-only: holds no replicated state").
