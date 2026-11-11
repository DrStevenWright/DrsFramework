# Common layer

General-purpose helpers for one area each, such as maths or graphics. Modules here may depend on Base (`DrsGlobals`) and on engine modules only.

**Belongs here:** stateless helpers that know nothing about XR, the platform or gameplay. Today: `DrsUtilities` with `DrsMathUtilities` (`Drs::Math`) and `DrsGraphicsUtilities` (e.g. `DrsDebugColors`).

**Doesn't belong here:** code that needs XR, subsystems or gameplay types. Put it in `Platform/` or `GameSupport/`.

- **Common modules are independent of each other.** `LayerRules.json` allows no same-layer dependencies here, so a maths module must not use a graphics module, for example. If two helpers need each other, merge them or move the shared part down to Base.
- Add a new area as a new module inside `DrsUtilities` (list it in `DrsUtilities.uplugin`), not as a new plugin, unless it needs its own engine plugin dependencies.
- Put new functions in a nested namespace named after the area: `Drs::Math`, `Drs::Graphics`.
