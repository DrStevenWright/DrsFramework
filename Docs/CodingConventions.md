# Coding conventions

These conventions apply to DrsFramework and to every project built on it, including [DrsProject](https://github.com/DrStevenWright/DrsProject) and [DrsTestProject](https://github.com/DrStevenWright/DrsTestProject). They live here because every project includes the framework as a submodule at `Plugins/DrsFramework`.

Anything not covered below follows [Epic's coding standard](https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine): type prefixes (`U`, `A`, `F`, `T`, `E`, `I`, `S`), PascalCase for types, functions and member variables, and the engine's formatting style.

## File header

Every code file starts with this line:

```cpp
// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
```

This covers `.h`, `.cpp`, `.inl`, `.Build.cs`, `.Target.cs` and any other `.cs` files. In PowerShell scripts (`.ps1`), use `#` instead of `//`. Data and config files (`.uplugin`, `.uproject`, `.ini`, `.json`, `.md`) don't need it. Vendor plugins in `Platform/External/` keep their own licences.

## Naming

### Spelling of DRS

The framework is called DRS. Write **DRS** in prose and display text, and `Drs` only where an identifier needs Unreal's camel-case.

| Where | Spelling | Example |
|---|---|---|
| Reflection categories and `ClassGroup` | DRS | `Category = "DRS|Input"`, `ClassGroup = (DRS)` |
| Plugin `FriendlyName`, `Description` and `Category` | DRS | `"DRS Input"` |
| Settings page category and other authored `DisplayName` metas | DRS | `CategoryName = TEXT("DRS")`, `"DRS Hand Mesh"` |
| Comments and docs | DRS | "the DRS framework" |
| Type names, files, namespaces | `Drs` | `UDrsInputAction`, `Drs::Input` |
| Plugin, module, repo and log category names | `Drs` | `DrsInput`, `LogDrsInput` |
| Macros | `DRS_` | `DRS_LOG` |

Names the editor derives from identifiers (such as "Drs Pawn" in class pickers) stay as they are. Don't sweep comments: use DRS in files you touch anyway and in new text. `Tools/CheckNaming.ps1` checks the categories, class groups and plugin descriptors.

### Prefixes

- Use the `Drs` prefix for plugins, modules, log categories, repos and projects: `DrsCore`, `FDrsXrArbiterModule`, `LogDrsXrPlatform`.
- **Prefix reflected types** (`UCLASS`, `USTRUCT`, `UENUM`, including `UINTERFACE`): `ADrsPawn`, `UDrsInputAction`, `EDrsHand`. Their names are global and UHT can't put them in a namespace, so the prefix is what keeps them from clashing with an engine or project type, which fails at load. This matters most for names that copy the engine's own pattern, such as `UInputTrigger…`.
- **Plain C++ types don't need the prefix.** Put them in a `Drs` namespace, nested one level by area (`Drs::Input`, `Drs::XrPlatform`), and keep Epic's letter prefix: `Drs::Input::FMotionInputKey` rather than `FDrsMotionInputKey`. This includes the free functions that belong with them (`Drs::Input::GetMotionControllerKeysForHand`). Module classes (`FDrsXrArbiterModule`) are the exception and keep the prefix, because they appear in logs and crash stacks.
- **Using namespaces:** don't put `using` declarations or directives at global scope in a header, because they leak into every file that includes it. Inside a header, write the namespace out (`Drs::XrArbiter::FXrSelection`), or define the code inside its own namespace block. In a `.cpp`, either put the implementation inside the namespace block, or list `using` declarations for the types you need straight after the includes (`using Drs::XrArbiter::FXrSelection;`). Prefer declarations to `using namespace`, so each file shows what it depends on.
- Use `DRS_` only for macros: `DRS_LOG`, `DRS_LOG_GLOBAL`.
- Put free functions and constants in the `Drs` namespace, nested by area where useful: `Drs::GetDebugName`, `Drs::Math::ToYawOnly`, `Drs::XrRuntime::FindOculusRuntimeJson`.

### Variables: members versus everything else

We differ from Epic's standard on one point. **Function arguments and local variables use camelCase**; members keep Epic's PascalCase.

| Kind | Convention | Example |
|---|---|---|
| Member variables (class or struct, `UPROPERTY` or plain C++) | PascalCase; bools get `b` | `TrackingOrigin`, `bHideWhenUntracked`, `uint8 bReplicateMovement : 1` |
| Function arguments | camelCase, no `b` | `SetHand(EDrsHand hand)`, `SetActive(bool isActive)` |
| Local variables (including `static` and `constexpr` locals) | camelCase, no `b` | `const bool isTracked = ...;`, `constexpr float goldenRatioConjugate = ...;` |
| Namespace-scope and file-scope constants | PascalCase | `Drs::FrameworkVersionMajor`, `RuntimeJsonVariable` |
| Bool-returning query functions | PascalCase question | `IsXrActive()`, `CanFire()`, `HasTarget()` |

Rules that go with the table:

- **camelCase bools must start with a question word:** `is`, `has`, `can`, `should`, `was`, `does`, `needs` and so on. This does the job the `b` prefix does for members: it says "this is a yes/no value". `isVisible` is fine, but bare `visible` or `enabled` is not.
- **Members always use Epic style, including the `b`,** whether or not they are reflected. That way a name doesn't change when someone later adds `UPROPERTY()`, and our members match the engine members they sit beside in derived classes.
- **There is no Blueprint exception for arguments.** `BlueprintCallable` functions and dynamic delegates also use camelCase arguments without `b`. The editor builds pin labels with `FName::NameToDisplayString`, which turns `isActive` into "Is Active", the same result the `b`-stripping gives for `bIsActive`. (See the open items below.)
- Functions that return bool to report success, rather than answer a question, may keep a verb name: `bool SetFloorTrackingOrigin()`. Document what the return value means.
- **IDE warnings:** Rider's built-in Unreal naming rules flag camelCase arguments and locals. The repos don't ship naming settings, so turn those warnings off in your own Rider settings.

### Why

- UnrealBuildTool treats variable shadowing as an error (`ShadowVariableWarningLevel`; MSVC C4456–C4459, clang `-Wshadow`). A local or argument with the same name as a member fails the build.
- Engine base classes such as `AActor`, `UActorComponent`, `APawn` and `UPrimitiveComponent` declare hundreds of PascalCase members that every derived class inherits. With PascalCase arguments, names like `Owner`, `Instigator`, `Hand` or `Health` can clash, sometimes only after an engine upgrade adds a member. camelCase arguments and locals can never clash with them.
- In Epic's style, bool locals and bool members look the same (`bHidden`). Dropping the `b` outside members keeps the case-based separation working for bools too.

### No Hungarian notation

Names describe what a value means, not its type, scope or storage. The case of a name already tells members apart from arguments and locals. So don't use:

- **type prefixes:** `p`, `n`, `i`, `f`, `sz`, `str`, `arr` (`pActor`, `nCount`, `fSpeed`);
- **scope prefixes:** `m_`, `g_`, `s_`, `k` (`m_Health`, `kMaxHands`);
- **prefixes that only avoid a member's name:** `In`, `New`, `The`, `My`. Because members are PascalCase, an argument can use the member's name in camelCase: `SetHand(EDrsHand hand) { Hand = hand; }`;
- **`io`/`InOut`:** a parameter that is read and then changed is still an output, so use `out`.

Exceptions:

- Epic's type prefixes on type names (`U`, `A`, `F`, `T`, `E`, `I`, `S`).
- `b` on bool members.
- `out` on reference or pointer parameters the function writes to, including ones it also reads: `bool TryGetHit(FHitResult& outHit)`, `void Normalize(FVector& outVector)`. This marks intent, not type: C++ has no `out` keyword, so the prefix shows in the function body which values are being produced. Parameters without `out` are inputs.
- Engine globals you use but don't define (`GEngine`, `GIsEditor`). Don't add new `G`-prefixed globals; use a subsystem or a namespaced constant instead.

### Exceptions

- **Engine code:** anything changed in the engine itself, or prepared for an upstream contribution to Epic, follows Epic's standard exactly. Don't mix styles in engine files.
- **Third-party and vendor plugins** (such as the MetaXR fork in host projects) keep their own style. Don't reformat them.
- **Overrides of engine virtuals:** you may rename the arguments in an override, and camelCase is preferred: `virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;`. Epic's `Out` parameters become `out` the same way (`OutHitResult` → `outHitResult`). Code generated by engine macros keeps its macro-generated names.
- **Macro-bound arguments:** when an engine macro refers to a function argument by name, keep the engine's exact name for that argument. Renaming it breaks every use of the macro with "undeclared identifier". Examples: `OutLifetimeProps` in `GetLifetimeReplicatedProps` (used by `DOREPLIFETIME`, `DOREPLIFETIME_CONDITION`, `RESET_REPLIFETIME` and so on), and `ChangedPropertyTracker` in `PreReplication` (used by `DOREPLIFETIME_ACTIVE_OVERRIDE`). Other arguments in the same signature still follow our rules. Copy the name from the engine declaration and say why in a short comment, so nobody "fixes" it later. Don't work around it with a renamed parameter plus an engine-named local reference.

  ```cpp
  void ADrsPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const // Name required by DOREPLIFETIME
  {
  	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
  	DOREPLIFETIME(ADrsPawn, TrackingOrigin);
  }
  ```
- **Macro parameters** follow Epic's PascalCase style (`DRS_LOG(CategoryName, Verbosity, Format, ...)`).
- **C# build files** (`.Build.cs`, `.Target.cs`) follow Epic's templates, e.g. the `ReadOnlyTargetRules Target` constructor argument.

## Source layout

Each module's `Source/<Module>/` has a `Public/` and a `Private/` folder.

- **Public headers go in `Public/<Module>/`**, named after the module, so every include says where it comes from: `#include "DrsInput/DrsInputSettings.h"`. Headers used only inside the module go in `Private/`.
- **Split by area, not by kind of type.** When an area reaches about six to eight files, give it a folder (such as `MotionController/` in DrsInput) under both `Public/<Module>/` and `Private/`, with the same name in each, so a header and its `.cpp` sit in the same place. Go at most two levels deep, and keep module-wide files at the top. Don't create `Classes/` or `Structs/` folders.
- **If a module has several areas that share little, split it into modules** instead of adding folders. The layer checker then covers the dependencies between them.
- **Include order:** three groups, separated by one blank line each:
  1. The include that must come first: the `.cpp`'s own header, or `CoreMinimal.h` in a header.
  2. Every other include, sorted alphabetically by path with no blank lines. Don't group DRS and engine headers separately: the module-name folder already keeps a module's headers together. Keep `#if` blocks that contain includes at the end of this group.
  3. The include that must come last: the `.generated.h`, or `UE_INLINE_GENERATED_CPP_BY_NAME` in a `.cpp`.

  Skip a group that has nothing in it. Files whose includes must keep a fixed order, such as those wrapped in `AllowWindowsPlatformTypes.h` and `HideWindowsPlatformTypes.h`, are the exception: sort around the wrapped block and leave its contents alone.
- Tests mirror the folders of the module they test.

## Reflection categories

`Category` on `UPROPERTY` and `UFUNCTION` decides where an item appears in the Details panel and the Blueprint palette.

- **Format: `DRS|<Feature>[|<Area>]`**, for example `DRS|Input`, `DRS|Input|Triggers`, `DRS|XR Platform|HMD`. The root is always `DRS` (never `Drs`): DRS is the framework's name, and `Drs` is only for identifiers. The editor compares categories without regard to case but shows the first spelling it sees, so mixed spellings make the same group look different from run to run.
- **The second level is the feature in plain words**, written as it should display (spaces allowed, as in the engine's own `Hand Tracking`). Don't use a module or class name, and don't use the word "System". Use a third level only when a feature has distinct areas, and never go deeper than three levels.
- **Don't put the layer in the category.** `DRS|GameSupport|...` and `DRS|Common|...` describe our structure, not something users look for.
- **Use the same category for a feature's properties and functions**, so the Details panel and the palette agree.
- **Use a sub-category such as `DRS|Input|Advanced` or `DRS|Input|Debug`** for items most users don't need, together with `AdvancedDisplay` for properties.
- **`UDeveloperSettings` classes** set `CategoryName = TEXT("DRS")` in their constructor, so that every page is grouped under DRS. Don't override `GetCategoryName()` or `GetContainerName()`: the editor container comes from `Config = EditorSettings`.
  - Give the class a `DisplayName` of the feature alone (`"Input"`, `"XR Arbiter"`). Don't add DRS: the editor already shows the page as "DRS - Input".
  - `SectionName` is only the page's internal ID. It defaults to the class name and is never displayed, so leave it out unless code needs to open the page by ID.
  - Give the properties of a settings class `Category = NoCategory`: the page is already the scope, and `NoCategory` shows them without a group header. Don't leave `Category` off, because UHT then uses the class name and the editor shows a "Drs Input Settings" group. Use a real category only for a group that is distinct from the rest of the page, for example `"XR Vendor Plugin"` on the one arbiter option in the editor settings.
- **`ClassGroup = (DRS)` on components is separate** (it controls the Add Component menu). Set both.
- Vendor plugins keep their own categories.

## Tooltips on reflected types

Comments on `UCLASS`, `USTRUCT`, `UENUM` (and its values), `UPROPERTY` and `UFUNCTION` become tooltips in the editor, and a settings class's comment is the description of its page. Write them for the person who sees the tooltip, a designer or developer using the feature in the editor or Blueprint, not for someone reading the code.

- **Say what it does and when to change it:** the effect, the unit, the default or special value (`None` means ...), and anything that needs a restart.
- **Leave out how it works:** which code reads it, when it is read from config, why the class exists, and API or extension names (`XR_EXT_user_presence`, `LOCAL_FLOOR`) or which runtimes support it. Say what the user sees instead ("where the device doesn't report it"). Put that in the `.cpp`, in a comment inside the function, or in the plugin's `CLAUDE.md`, because anything directly above the declaration shows up as the tooltip.
- **On a settings class, describe the scope, not the contents.** Each property has its own tooltip, so the class comment says which area the page covers and who it applies to (the project or just you). Don't list or paraphrase the settings, or the comment goes stale when one is added.
- **Keep them short and write in full sentences.** If a tooltip needs a paragraph, the setting is probably doing too much.

## Logging

- Each module declares its own `LogDrs<Module>` category in its public module header (`DECLARE_LOG_CATEGORY_EXTERN`) and defines it in the module's `.cpp`.
- That `.cpp` also holds `IMPLEMENT_MODULE`. Use `FDefaultModuleImpl` unless the module has real startup or shutdown logic (as `DrsXrArbiter` does); don't add a module class just to log that the module started or shut down.
- Log with `DRS_LOG(Category, Verbosity, "...", ...)`. It is `UE_LOG` that applies `TEXT()` to the format itself, so pass a plain string literal: `DRS_LOG(LogDrsCore, Log, "No HMD (%d)", value);`. It adds no prefix, so write messages that make sense on their own.
- `DRS_CLOG(Condition, Category, Verbosity, "...", ...)` is the same for `UE_CLOG`.
- `DRS_LOG_ONCE` and `DRS_CLOG_ONCE` take the same arguments and log only the first time that call site is reached (for `DRS_CLOG_ONCE`, the first time its condition is true). They are thread-safe. Use them for messages in per-frame or per-call code that would otherwise flood the log.
- Use `DRS_LOG_GLOBAL(Verbosity, "...", ...)` for messages that belong to no single module. It logs to `LogDrs`.
- All `DRS_` log macros are single statements, so they work in an unbraced `if`/`else`.
- Use the `DRS_` macros everywhere, including modules that load at `EarliestPossible` (see `Base/CLAUDE.md`).

## Open items

- [ ] **Check Blueprint pin labels in UE 5.8.** Add one `BlueprintCallable` function and one dynamic multicast delegate with camelCase arguments (e.g. `isActive`, `hitActor`), and confirm the pins show "Is Active" and "Hit Actor". If they don't, revisit the no-Blueprint-exception rule.
