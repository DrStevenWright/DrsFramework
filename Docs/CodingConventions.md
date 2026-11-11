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

### Prefixes

- Use the `Drs` prefix for plugins, modules, types, log categories, repos and projects: `DrsCore`, `ADrsPawn`, `FDrsXrRuntimeModule`, `LogDrsXrPlatform`.
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
- **Third-party and vendor plugins** (including `Platform/External/`) keep their own style. Don't reformat them.
- **Overrides of engine virtuals:** you may rename the arguments in an override, and camelCase is preferred: `virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;`. Epic's `Out` parameters become `out` the same way (`OutHitResult` → `outHitResult`). Code generated by engine macros keeps its macro-generated names.
- **Macro parameters** follow Epic's PascalCase style (`DRS_LOG(CategoryName, Verbosity, Format, ...)`).
- **C# build files** (`.Build.cs`, `.Target.cs`) follow Epic's templates, e.g. the `ReadOnlyTargetRules Target` constructor argument.

## Logging

- Each module declares its own `LogDrs<Module>` category in its public module header (`DECLARE_LOG_CATEGORY_EXTERN`) and defines it in the module's `.cpp`.
- Log with `DRS_LOG(Category, Verbosity, TEXT("..."), ...)`. It is `UE_LOG` with the calling function's name prepended.
- Use `DRS_LOG_GLOBAL(Verbosity, TEXT("..."), ...)` for messages that belong to no single module. It logs to `LogDrs`.
- `UE_LOG` is fine where `DrsGlobals` isn't available, e.g. in `DrsXrRuntime`, which loads at `EarliestPossible`.

## Open items

- [ ] **Check Blueprint pin labels in UE 5.8.** Add one `BlueprintCallable` function and one dynamic multicast delegate with camelCase arguments (e.g. `isActive`, `hitActor`), and confirm the pins show "Is Active" and "Hit Actor". If they don't, revisit the no-Blueprint-exception rule.
- [ ] **Set up IDE naming rules.** Rider/ReSharper ship with Epic's rules and will warn on camelCase arguments and locals. Add custom naming rules for parameters and locals, and commit the shared settings (`.DotSettings`) to each repo.
