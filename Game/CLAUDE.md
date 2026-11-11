# Game layer

The top framework layer and the only one a project enables directly. Modules here may depend on every lower layer.

**Belongs here:** the game classes (GameMode, GameState, PlayerController, PlayerState, Pawn, GameInstance), and the code that connects GameSupport features that don't know about each other. Today this layer has one plugin, `DrsCore` (see its own CLAUDE.md).

**Doesn't belong here:** features that could stand alone. Put them in `GameSupport/` and let `DrsCore` use them. Keep this layer thin: it assembles features and doesn't implement them.

- Nothing in the framework may depend on this layer. Only host projects use it.
- Don't add another Game-layer plugin without a reason. Same-layer dependencies would need an `AllowedSameLayer` entry, and every project enables only `DrsCore`.
