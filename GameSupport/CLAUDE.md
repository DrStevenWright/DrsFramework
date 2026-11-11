# GameSupport layer

Reusable gameplay building blocks: components and world subsystems that a game assembles. Modules here may depend on Base, Common and Platform.

**Belongs here:** one feature per plugin, usable on its own. Today:
- `DrsXrGame`: shared XR gameplay pieces. It contains `UDrsHandMeshComponent` (a `USkeletalMeshComponent`) with `EDrsHand` (in `DrsHand.h`). The hand mesh defaults to the engine's `SkeletalCube`, scaled to a palm-sized box, with the plugin's own `/DrsXrGame/M_DrsHandPlaceholder` material: engine materials such as `BasicShapeMaterial` aren't flagged "Used with Skeletal Mesh" and would render with the default material in cooked builds. Any replacement material needs that flag and a `Color` vector parameter for the per-hand tint.
- `DrsInteraction`: interaction (grabbing, pointing, using). Empty module for now.
- `DrsLocomotion`: movement and teleporting. Empty module for now.
- `DrsSpawning`: spawning. Empty module for now.

**Doesn't belong here:** code that wires several features together, or game classes such as pawns and game modes. That goes in `Game/DrsCore`.

- **Feature plugins don't depend on each other.** The only same-layer dependencies allowed are on `DrsXrGame`: `DrsInteraction`, `DrsLocomotion` and `DrsSpawning -> DrsXrGame`. If two features need to work together, connect them in `DrsCore`. Don't add an `AllowedSameLayer` entry between features without discussing it first.
- Shared types that several features need (like `EDrsHand`) go in `DrsXrGame`.
- Components use `ClassGroup = (Drs)` and `meta = (BlueprintSpawnableComponent)`. UFUNCTION/UPROPERTY categories are `Drs|<Feature>`.
- World subsystems override `DoesSupportWorldType` to choose which world types they run in.
- Say in each class's doc comment whether it holds replicated state ("Local-only: holds no replicated state").
