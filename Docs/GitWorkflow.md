# Git workflow

These rules apply to DrsFramework and to every project built on it, including [DrsProject](https://github.com/DrStevenWright/DrsProject) and [DrsTestProject](https://github.com/DrStevenWright/DrsTestProject). Like [`CodingConventions.md`](CodingConventions.md), they live here because every project includes the framework as a submodule at `Plugins/DrsFramework`.

All names follow [Conventional Commits](https://www.conventionalcommits.org/).

## Types

| Type | Use for |
|---|---|
| `feat` | New behaviour or API for users of the framework or project |
| `fix` | A bug fix |
| `refactor` | A code change that adds no feature and fixes no bug (renames, moves, removing unused code) |
| `docs` | Documentation only, including CLAUDE.md files |
| `test` | Adding or changing tests only |
| `chore` | Upkeep with no change for users: tooling, config, dependency and submodule updates |
| `ci` | CI configuration |
| `perf` | A change made for performance |
| `build` | Build system changes (`.Build.cs`, `.Target.cs`, `.uplugin` dependencies) |

Pick the type from what the change means for users, not from which files it touches.

## Scopes

A scope is optional. Use one when the change sits in one area.

| Scope | Area |
|---|---|
| `globals` | `Base/DrsGlobals` (logging, version, `DrsBaseUtilities`) |
| `utilities` | `Common/DrsUtilities` |
| `xrplatform` | `Platform/DrsXrPlatform`, including `DrsXrRuntime` |
| `xrgame` | `GameSupport/DrsXrGame` |
| `interaction` | `GameSupport/DrsInteraction` |
| `locomotion` | `GameSupport/DrsLocomotion` |
| `spawning` | `GameSupport/DrsSpawning` |
| `core` | `Game/DrsCore` |
| `base`, `common`, `platform`, `gamesupport`, `game` | Several plugins in one layer |
| `deps` | Module and plugin dependencies across the framework |
| `tools` | `Tools/` (`CheckLayers.ps1`, `Bootstrap.ps1`, `LayerRules.json`) |
| `config`, `content` | Host projects: `Config/` and `Content/` |

Leave the scope out for changes that span layers. When you add a plugin, add its scope here.

## Branches

- Name branches `<type>/<short-kebab-description>`, using the same types as commits (see [Types](#types)):

  | Prefix | Example |
  |---|---|
  | `feat/` | `feat/hand-tracking-gestures` |
  | `fix/` | `fix/openxr-runtime-fallback` |
  | `refactor/` | `refactor/core-classes-and-logging` |
  | `docs/` | `docs/git-workflow` |
  | `test/` | `test/log-macros` |
  | `chore/` | `chore/update-drsframework` |
  | `ci/` | `ci/build-on-pr` |
  | `perf/` | `perf/hand-mesh-tick` |
  | `build/` | `build/android-arm64-config` |

  Use the type of the PR's main change. If a branch mixes several, pick the one the PR title will use.
- Don't use the Git Flow prefixes `feature/`, `bugfix/` or `hotfix/`.
- Start a new branch from `main` for each piece of work. Don't reuse a branch after its PR has merged.
- GitHub deletes the branch when its PR merges. Delete your local copy too.

## Commits

- Write the subject as `<type>(<scope>): <imperative summary>`: `feat(core): add UDrsGameInstance`, `fix: fall back when the Meta OpenXR runtime is missing`.
- Commits on a branch don't reach `main` (see [Merging](#merging)), so they can be as small as you like. Their subjects should still follow the format, because they show in the PR.
- Commits made before 2026-11-11 have no type prefix. Leave them as they are.

### Breaking changes

While the framework's major version is 0 (`FrameworkVersionMajor` in `Base/DrsGlobals/Source/DrsGlobals/Public/DrsVersion.h`), don't mark anything as breaking: no `!` after the type and no `BREAKING CHANGE:` footer. Any 0.x change may break host projects. Instead, list what host projects must change in the PR description.

This also applies in host projects, which follow the framework's version. Start marking breaking changes once the framework reaches 1.0.

## Pull requests

- **Title:** same format as a commit subject. With squash merges, the title becomes the commit subject on `main`, so it's the name that matters most.
- **Description:** it becomes the body of the commit on `main`, so write it for someone reading the history later. Include:
  - what changed and why;
  - **Host project updates:** what projects using the framework must change (renamed classes, config paths, macro signatures). Leave the section out if there is nothing to change;
  - **Testing:** what you built and ran (build, `Tools/CheckLayers.ps1`, `Automation RunTests Drs`, PIE, device), and what you didn't test.
- Open framework PRs against DrsFramework `main`, and host PRs against the host's `main`.

## Merging

- **Squash merge only.** Both repos are set up on GitHub to allow only squash merges, using the PR title and description as the commit message, and to delete the head branch after merging.
- One PR becomes one commit on `main`. Branch commits disappear when the branch is deleted.

## Framework changes from a host project

1. In `Plugins/DrsFramework`, check out a new branch from `origin/main`. The submodule is often on a detached HEAD.
2. Commit there and open a PR against DrsFramework `main`.
3. **After that PR merges,** update the submodule in the host project to the squash commit on DrsFramework `main`, on its own branch and PR. Never point a host commit at a framework branch commit: it disappears once the branch is deleted, and the host's pointer would then refer to a commit nobody can fetch.
4. Host changes that depend on the framework change, such as config that uses a new class, go in the same host PR as the pointer update.

## Working with Claude Code

- Claude commits on local branches. It pushes, opens PRs and merges only when asked.
- Claude's commits end with a `Co-Authored-By: Claude …` line, and PRs it opens end with a "Generated with Claude Code" line. Leave them in. When squash merging, check that the co-author line is still in the commit message in GitHub's merge dialog.
