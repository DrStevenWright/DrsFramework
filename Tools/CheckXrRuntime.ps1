# Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
<#
.SYNOPSIS
    Starts the project with -game, reports which OpenXR runtime and vendor XR plugin DRS selected, then closes it.

.DESCRIPTION
    Run from a project that has DrsFramework at Plugins/DrsFramework, after building its Editor target. The script:
      1. Reports which known runtimes are running (SteamVR counts only while vrmonitor.exe, its status window, runs).
      2. Starts the editor binary with -game in a small window, logging to Saved/Logs/XrRuntimeCheck.log.
      3. Waits for the arbiter's runtime selection line, collects the DRS XR log lines for a few more seconds, and closes the
         process it started.
      4. With -Expect, exits with 1 unless the selected runtime is the one named ("None" when none is selected).

    No headset is needed: the runtime is selected before OpenXR starts. Without a headset OpenXR then fails to start,
    which is expected.

.EXAMPLE
    # With SteamVR open: SteamVR should win over the Oculus runtime, even when that is running too.
    powershell -ExecutionPolicy Bypass -File Plugins/DrsFramework/Tools/CheckXrRuntime.ps1 -Expect SteamVR

.EXAMPLE
    # With SteamVR closed and Meta's Link software running, Oculus should win. With neither running, or with -noHMD, no
    # runtime should be selected.
    powershell -ExecutionPolicy Bypass -File Plugins/DrsFramework/Tools/CheckXrRuntime.ps1 -Expect Oculus
    powershell -ExecutionPolicy Bypass -File Plugins/DrsFramework/Tools/CheckXrRuntime.ps1 -Expect None
    powershell -ExecutionPolicy Bypass -File Plugins/DrsFramework/Tools/CheckXrRuntime.ps1 -Expect None -ExtraArgs '-noHMD'
#>
[CmdletBinding()]
param(
    [string]$ProjectFile,
    [string]$EngineDir,
    # The runtime name that should be selected (as in RuntimePriority), or None.
    [string]$Expect,
    # More command-line arguments for the game, e.g. '-noHMD' or '-XrPlugin=Pico'.
    [string]$ExtraArgs = '',
    [int]$TimeoutSeconds = 120,
    [int]$SettleSeconds = 10
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

function Find-Engine([string]$Association) {
    if ($Association -match '^\d+\.\d+$') {
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
    $EngineDir = Find-Engine (Get-Content $ProjectFile -Raw | ConvertFrom-Json).EngineAssociation
}
$editor = Join-Path $EngineDir 'Engine\Binaries\Win64\UnrealEditor.exe'
if (-not (Test-Path $editor)) { throw "UnrealEditor.exe not found at '$editor'." }

Write-Host 'Running before launch:'
foreach ($name in 'vrmonitor', 'vrserver', 'oculus-platform-runtime', 'PICO Connect', 'Business Streaming') {
    $state = if (Get-Process -Name $name -ErrorAction SilentlyContinue) { 'running' } else { '-' }
    Write-Host ("  {0,-26} {1}" -f "$name.exe", $state)
}

$logName = 'XrRuntimeCheck.log'
$logPath = Join-Path $projectRoot "Saved\Logs\$logName"
Remove-Item $logPath -ErrorAction SilentlyContinue

$arguments = "`"$ProjectFile`" -game -windowed -ResX=640 -ResY=360 -nosplash -log=$logName $ExtraArgs"
Write-Host "Starting: UnrealEditor.exe $arguments"
$process = Start-Process -FilePath $editor -ArgumentList $arguments -PassThru

$selectionPattern = 'LogDrsXrRuntime: (Selected the|-noHMD given|None of the)'
$deadline = (Get-Date).AddSeconds($TimeoutSeconds)
$selectedAt = $null
try {
    while ((Get-Date) -lt $deadline -and -not $process.HasExited) {
        Start-Sleep -Milliseconds 500
        if (-not $selectedAt -and (Test-Path $logPath) -and (Select-String -Path $logPath -Pattern $selectionPattern -Quiet)) {
            $selectedAt = Get-Date
            $deadline = $selectedAt.AddSeconds($SettleSeconds) # Let the arbiter and the HMD setup log too.
        }
    }
}
finally {
    if (-not $process.HasExited) {
        Stop-Process -Id $process.Id -Force
    }
}

if (-not (Test-Path $logPath)) { throw "No log was written to '$logPath'." }
Write-Host ''
Write-Host "DRS XR log lines ($logPath):"
Select-String -Path $logPath -Pattern 'LogDrsXr(Runtime|Arbiter|Platform)' | ForEach-Object {
    Write-Host ('  ' + ($_.Line -replace '^\[[^\]]*\]\[[^\]]*\]', ''))
}

if (-not $selectedAt) {
    Write-Warning "The arbiter logged no runtime selection within $TimeoutSeconds s."
    exit 1
}

$selectedLine = Select-String -Path $logPath -Pattern 'LogDrsXrRuntime: Selected the (\S+) OpenXR runtime' | Select-Object -First 1
$selected = if ($selectedLine) { $selectedLine.Matches[0].Groups[1].Value } else { 'None' }
Write-Host ''
Write-Host "Selected runtime: $selected"

if ($Expect) {
    if ($selected -ieq $Expect) {
        Write-Host "PASS: expected $Expect." -ForegroundColor Green
    }
    else {
        Write-Host "FAIL: expected $Expect, got $selected." -ForegroundColor Red
        exit 1
    }
}
