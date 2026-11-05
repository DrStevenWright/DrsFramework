# External vendor plugins

Private vendor plugin forks (MetaXR, Pico) are added here as git submodules.
Only `DrsXrPlatform` and other Platform-layer code may depend on them, and they
must not depend on any DrsFramework module.

These submodules are private. Clones without access to them will fail to fetch them.
