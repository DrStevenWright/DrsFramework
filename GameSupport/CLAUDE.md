# GameSupport layer

Reusable gameplay building blocks: components and world subsystems that a game assembles. Modules here may depend on Base, Common, Platform and each other.

**Belongs here:** one feature per plugin, usable on its own. Today:
- `DrsXrGame`: shared XR gameplay pieces. It contains `UDrsHandMeshComponent` (a `USkeletalMeshComponent`) which uses `EDrsHand` (from `DrsBase`). The hand mesh defaults to the engine's `SkeletalCube`, scaled to a palm-sized box, with the plugin's own `/DrsXrGame/M_DrsHandPlaceholder` material: engine materials such as `BasicShapeMaterial` aren't flagged "Used with Skeletal Mesh" and would render with the default material in cooked builds. Any replacement material needs that flag and a `Color` vector parameter for the per-hand tint.
- `DrsInteraction`: interaction (grabbing, pointing, using). Empty module for now.
- `DrsLocomotion`: movement and teleporting. Empty module for now.
- `DrsSpawning`: spawning. Empty module for now.

**Doesn't belong here:** code that wires several features together, or game classes such as pawns and game modes. That goes in `Game/DrsCore`.

- **Plugins here may depend on each other** (`"SameLayer": "Any"`), so list every plugin you use in your `.uplugin`. Both the module graph and the plugin graph must stay acyclic, and `Tools/CheckLayers.ps1` checks that. Depend on a plugin when yours genuinely builds on it. If two features only need to react to each other, connect them in `DrsCore` instead of adding a dependency; that keeps this layer from becoming a tangle.
- Shared XR types that several features need go in `DrsXrGame`. Types that every layer needs, such as the hand enums `EDrsHand` and `EDrsHandMask`, live in `DrsBase`.
- Components use `ClassGroup = (DRS)` and `meta = (BlueprintSpawnableComponent)`. UFUNCTION/UPROPERTY categories are `DRS|<Feature>`.
- World subsystems override `DoesSupportWorldType` to choose which world types they run in.
- Say in each class's doc comment whether it holds replicated state ("Local-only: holds no replicated state").
