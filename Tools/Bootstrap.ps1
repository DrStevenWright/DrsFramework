# Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
<#
.SYNOPSIS
    Prepares a project that uses DrsFramework: fetches submodules, generates project files, and optionally builds.

.DESCRIPTION
    Run from a project that has DrsFramework at Plugins/DrsFramework. The script:
      1. Runs 'git submodule update --init --recursive' in the project (this also fetches the vendor
         submodules in Platform/External, if you have access to them).
      2. Finds the engine from the project's EngineAssociation (a launcher version such as "5.8",
         or a registered source build GUID), unless -EngineDir is given.
      3. Generates IDE project files.
      4. With -Build, builds the project's Editor target (Win64, Development).

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File Plugins/DrsFramework/Tools/Bootstrap.ps1 -Build
#>
[CmdletBinding()]
param(
    [string]$ProjectFile,
    [string]$EngineDir,
    [switch]$Build,
    [switch]$SkipSubmodules
)

$ErrorActionPreference = 'Stop'

if (-not $ProjectFile) {
    # Tools -> DrsFramework -> Plugins -> project root
    $projectRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..\..')
    $found = @(Get-ChildItem $projectRoot -Filter *.uproject -File)
    if ($found.Count -ne 1) { throw "Expected one .uproject in '$projectRoot', found $($found.Count). Pass -ProjectFile." }
    $ProjectFile = $found[0].FullName
}
$ProjectFile = (Resolve-Path $ProjectFile).Path
$projectRoot = Split-Path -Parent $ProjectFile
$projectName = [IO.Path]::GetFileNameWithoutExtension($ProjectFile)
Write-Host "Project: $ProjectFile"

if (-not $SkipSubmodules) {
    Write-Host "Updating submodules..."
    git -C $projectRoot submodule update --init --recursive
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "Submodule update failed. Private vendor submodules need access to their repositories."
    }
}

function Find-Engine([string]$Association) {
    if ($Association -match '^\d+\.\d+$') {
        $manifest = Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'
        if (Test-Path $manifest) {
            $install = (Get-Content $manifest -Raw | ConvertFrom-Json).InstallationList |
                Where-Object { $_.AppName -eq "UE_$Association" } | Select-Object -First 1
            if ($install) { return $install.InstallLocation }
        }
        # The launcher also writes the install folder to the registry; the manifest can be empty.
        $key = Get-ItemProperty "HKLM:\SOFTWARE\EpicGames\Unreal Engine\$Association" -ErrorAction SilentlyContinue
        if ($key -and $key.InstalledDirectory -and (Test-Path $key.InstalledDirectory)) { return $key.InstalledDirectory }
        $default = Join-Path $env:ProgramFiles "Epic Games\UE_$Association"
        if (Test-Path (Join-Path $default 'Engine\Build\Build.version')) { return $default }
        throw "Launcher engine $Association was not found. Pass -EngineDir."
    }
    $builds = Get-ItemProperty 'HKCU:\Software\Epic Games\Unreal Engine\Builds' -ErrorAction SilentlyContinue
    if ($builds -and $builds.PSObject.Properties[$Association]) { return $builds.$Association }
    throw "Engine '$Association' is not registered. Pass -EngineDir."
}

if (-not $EngineDir) {
    $association = (Get-Content $ProjectFile -Raw | ConvertFrom-Json).EngineAssociation
    $EngineDir = Find-Engine $association
}
$EngineDir = (Resolve-Path $EngineDir).Path
Write-Host "Engine:  $EngineDir"

$ubt = Join-Path $EngineDir 'Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe'
if (-not (Test-Path $ubt)) { throw "UnrealBuildTool not found at '$ubt'." }

Write-Host "Generating project files..."
& $ubt -projectfiles "-project=$ProjectFile" -game -progress
if ($LASTEXITCODE -ne 0) { throw "Project file generation failed ($LASTEXITCODE)." }

if ($Build) {
    Write-Host "Building ${projectName}Editor Win64 Development..."
    & (Join-Path $EngineDir 'Engine\Build\BatchFiles\Build.bat') "${projectName}Editor" Win64 Development "-Project=$ProjectFile" -WaitMutex
    if ($LASTEXITCODE -ne 0) { throw "Build failed ($LASTEXITCODE)." }
}

Write-Host "Done."
