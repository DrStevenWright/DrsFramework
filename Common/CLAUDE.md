# Common layer

General-purpose helpers for one area each, such as maths or graphics. Modules here may depend on Base (`DrsBase`) and on engine modules only.

**Belongs here:** stateless helpers that know nothing about XR, the platform or gameplay. Today: `DrsUtilities` with `DrsMathUtils` (`Drs::Math`) and `DrsGraphicsUtils` (e.g. `DrsDebugColors`, and `UDrsViewFadeLibrary`, a full-screen fade and passthrough holdout drawn by a scene view extension; unlike the other helpers it keeps global fade state).

**Doesn't belong here:** code that needs XR, subsystems or gameplay types. Put it in `Platform/` or `GameSupport/`.

- **Common modules are independent of each other.** `LayerRules.json` sets `"SameLayer": "None"` for Common, so a maths module must not use a graphics module, for example. If two helpers need each other, merge them or move the shared part down to Base.
- Add a new area as a new module inside `DrsUtilities` (list it in `DrsUtilities.uplugin`), not as a new plugin, unless it needs its own engine plugin dependencies.
- Put new functions in a nested namespace named after the area: `Drs::Math`, `Drs::Graphics`.
