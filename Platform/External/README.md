# External vendor plugins

Private vendor plugin forks (MetaXR, Pico) are added here as git submodules.
Only `DrsXrPlatform` and other Platform-layer code may depend on them, and they
must not depend on any DrsFramework module.

These submodules are private. Clones without access to them will fail to fetch them.

| Folder | Repository | Branch |
|---|---|---|
| `MetaXR` | [DrStevenWright/MetaXR](https://github.com/DrStevenWright/MetaXR) (private) | `drs` |

## Vendor repositories

Each vendor repository keeps two lines of history:

- `main` holds Meta's (or the vendor's) releases only, one commit per release, tagged with the version (`v1.209.0`). Binaries, Intermediate and source `.fbx` files are left out, and files are stored in plain git, not LFS.
- `drs` is the DrsFramework fork. The submodule here tracks it. New releases are merged into it from `main` with a merge commit, so these repositories don't use squash-only merging.

To upgrade, import the new release onto `main` and tag it, merge the tag into `drs`, then update the submodule pointer here.
