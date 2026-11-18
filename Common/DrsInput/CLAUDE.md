# DrsInput

A wrapper around Enhanced Input: custom triggers and modifiers, input action and mapping context classes, helpers for adding and removing contexts, and the device-agnostic motion controller keys. `DrsInput` is the only module, and it has content (see below).

## What a project must set up

DrsInput only works fully when the host project's config uses its classes. These are project-facing contracts, so changing them is a host-project update:

- **`DefaultInputComponentClass=/Script/DrsInput.DrsEnhancedInputComponent` and `DefaultPlayerInputClass=/Script/DrsInput.DrsEnhancedPlayerInput`** in `DefaultInput.ini`. A pawn's input component is created from the first, and `UDrsMotionControllerInputComponent` can only bind to a `UDrsEnhancedInputComponent`. The second releases inputs that a context change interrupts. With the stock engine classes, the `Drs::Input` helpers silently skip the release, so no input is ever force-released.
- **`IMC_MotionController` in the Enhanced Input `DefaultMappingContexts`**, with `bAddImmediately=False`. OpenXR builds its actions and interaction profile suggestions from that list, so without the entry the runtime never sees the motion controller bindings. `ADrsPawn` (in `DrsCore`) adds the context to the player.

## Device-agnostic motion controller keys

`FMotionControllerHandKeys` (`DrsMotionControllerKeys.h`) defines one custom key per physical input, per hand, such as `Left_Trigger_Press`. The flow is: platform keys (for example Meta Touch) → `IMC_MotionController` → `UDrsInputAction`s → `UDrsMotionControllerInputComponent` → simulated device-agnostic keys → gameplay contexts, which only ever map to those keys.

- **The module registers the keys at startup,** from `StartupModule` (`Drs::Input::RegisterMotionControllerKeys`). It loads in the `PreDefault` phase so the keys exist before any asset that maps them loads. Don't move it to a later phase.
- **Key names are serialised into assets.** Every mapping context that maps one of these keys stores its `FName`. Renaming a key breaks those assets, so treat the names like a file format.
- **An action's `ActionID` must equal its key name.** The input component finds an action by ID, and the key it forwards has the same name. The actions and the global mapping context are in `Content/MotionController/`. Change a key name and its assets together.
- Platform-specific behaviour (controller capabilities, hand tracking, emulated inputs) doesn't belong here. Derive from `UDrsMotionControllerInputComponent` in a higher layer and override `ShouldForwardKey`.

## Conventions

- **Layout:** `Public/DrsInput/` has the module-wide headers, with `MotionController/` for the motion controller area; its `.cpp` files mirror it in `Private/`. See "Source layout" in `Docs/CodingConventions.md`.
- **Triggers and modifiers live in `Private/`.** They are found through reflection, so they don't need exports.
- **Add and remove contexts through the `Drs::Input` helpers (`DrsInputUtils.h`),** not by calling Enhanced Input's local player subsystem directly. The helpers record the reasons each context was added for in the local player's `UDrsInputContextSubsystem` (so they are per player, and independent of any pawn or input component), and release inputs that the change would otherwise leave stuck.
- **Hands are `EDrsHand` and `EDrsHandMask`** from `DrsBase`, never `EControllerHand`.
