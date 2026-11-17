# Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
<#
.SYNOPSIS
    Checks that DrsFramework module and plugin dependencies follow the layer rules.

.DESCRIPTION
    Reads Tools/LayerRules.json, every .uplugin and every *.Build.cs under the framework root, and fails on:
      - a dependency on a module in a higher layer
      - a same-layer dependency that the layer's SameLayer policy does not allow and AllowedSameLayer does not list.
        SameLayer is 'Plugin' (the default: dependencies within a plugin are allowed), 'Any' or 'None'.
        *Tests modules may always depend on modules in their own plugin.
      - an AllowedSameLayer entry for a dependency that the layer's policy already allows
      - a dependency cycle between framework modules, or between framework plugins
      - a layer with more plugins than its MaxPlugins allows
      - a module that depends on a module in another framework plugin that its .uplugin does not list
      - a .uplugin that lists a framework plugin in a higher layer
    Dependencies on engine or other non-framework modules are ignored.
    Works with Windows PowerShell 5.1 and PowerShell 7.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File Tools/CheckLayers.ps1
#>
[CmdletBinding()]
param(
    [string]$FrameworkRoot,
    [string]$RulesFile
)

$ErrorActionPreference = 'Stop'
# Windows PowerShell 5.1 does not set $PSScriptRoot for parameter defaults, so default here.
if (-not $FrameworkRoot) { $FrameworkRoot = Split-Path -Parent $PSScriptRoot }
if (-not $RulesFile) { $RulesFile = Join-Path $PSScriptRoot 'LayerRules.json' }
$FrameworkRoot = (Resolve-Path $FrameworkRoot).Path
$rules = Get-Content $RulesFile -Raw | ConvertFrom-Json
$errors = New-Object System.Collections.Generic.List[string]

function Get-RelativePath([string]$Path) {
    return $Path.Substring($FrameworkRoot.Length).TrimStart('\', '/').Replace('\', '/')
}

# Layer lookup: the longest rule path that prefixes the plugin's path.
$layers = @($rules.Layers)
$validPolicies = @('Plugin', 'Any', 'None')
$sameLayerPolicies = @(foreach ($l in $layers) { if ($l.SameLayer) { [string]$l.SameLayer } else { 'Plugin' } })
for ($i = 0; $i -lt $layers.Count; $i++) {
    if ($validPolicies -notcontains $sameLayerPolicies[$i]) {
        $errors.Add("Layer '$($layers[$i].Name)' has SameLayer '$($sameLayerPolicies[$i])'; expected one of: $($validPolicies -join ', ').")
    }
}
function Get-Layer([string]$RelativePath) {
    $best = $null
    for ($i = 0; $i -lt $layers.Count; $i++) {
        $p = $layers[$i].Path.TrimEnd('/') + '/'
        if (($RelativePath + '/').StartsWith($p, [StringComparison]::OrdinalIgnoreCase)) {
            if ($null -eq $best -or $p.Length -gt $best.PathLength) {
                $best = [pscustomobject]@{ Index = $i; Name = $layers[$i].Name; PathLength = $p.Length; SameLayer = $sameLayerPolicies[$i] }
            }
        }
    }
    return $best
}

# MaxPlugins is optional; 0 means no limit.
$maxPlugins = @(foreach ($l in $layers) { if ($l.MaxPlugins) { [int]$l.MaxPlugins } else { 0 } })

# --- Collect plugins and modules ---------------------------------------------
$plugins = @{}   # plugin name -> info
$modules = @{}   # module name -> info

foreach ($file in Get-ChildItem $FrameworkRoot -Recurse -Filter *.uplugin -File) {
    $rel = Get-RelativePath $file.DirectoryName
    $layer = Get-Layer $rel
    if ($null -eq $layer) { $errors.Add("Plugin '$($file.BaseName)' at '$rel' is not inside any layer folder."); continue }
    $json = Get-Content $file.FullName -Raw | ConvertFrom-Json
    $plugins[$file.BaseName] = [pscustomobject]@{
        Name = $file.BaseName; Dir = $file.DirectoryName; Layer = $layer
        PluginDeps = @($json.Plugins | Where-Object { $_ -and $_.Name } | ForEach-Object { $_.Name })
    }
}

for ($i = 0; $i -lt $layers.Count; $i++) {
    if ($maxPlugins[$i] -gt 0) {
        $inLayer = @($plugins.Values | Where-Object { $_.Layer.Index -eq $i } | ForEach-Object { $_.Name } | Sort-Object)
        if ($inLayer.Count -gt $maxPlugins[$i]) {
            $errors.Add("Layer '$($layers[$i].Name)' holds at most $($maxPlugins[$i]) plugin(s) but has $($inLayer.Count): $($inLayer -join ', ').")
        }
    }
}

foreach ($p in $plugins.Values) {
    $sourceDir = Join-Path $p.Dir 'Source'
    if (-not (Test-Path $sourceDir)) { continue }
    foreach ($buildCs in Get-ChildItem $sourceDir -Recurse -Filter *.Build.cs -File) {
        $name = $buildCs.Name -replace '\.Build\.cs$', ''
        $text = Get-Content $buildCs.FullName -Raw
        # Strip comments so commented-out dependencies are ignored.
        $text = [regex]::Replace($text, '/\*.*?\*/', '', 'Singleline')
        $text = [regex]::Replace($text, '//[^\r\n]*', '')
        $deps = New-Object System.Collections.Generic.HashSet[string]
        $pattern = '(?:Public|Private)DependencyModuleNames\s*\.\s*(?:AddRange\s*\(\s*new\s+(?:string\s*)?\[\s*\]\s*\{(?<list>[^}]*)\}|Add\s*\(\s*"(?<one>[^"]+)"\s*\))'
        foreach ($m in [regex]::Matches($text, $pattern)) {
            if ($m.Groups['one'].Success) { [void]$deps.Add($m.Groups['one'].Value) }
            foreach ($q in [regex]::Matches($m.Groups['list'].Value, '"([^"]+)"')) { [void]$deps.Add($q.Groups[1].Value) }
        }
        if ($modules.ContainsKey($name)) { $errors.Add("Module '$name' is defined more than once."); continue }
        $modules[$name] = [pscustomobject]@{ Name = $name; Plugin = $p; Deps = @($deps) }
    }
}

# --- Parse the allowed same-layer list ----------------------------------------
$allowed = New-Object System.Collections.Generic.HashSet[string]
foreach ($entry in @($rules.AllowedSameLayer)) {
    $parts = $entry -split '->'
    if ($parts.Count -ne 2) { $errors.Add("Bad AllowedSameLayer entry '$entry' (expected 'A -> B')."); continue }
    $from = $parts[0].Trim(); $to = $parts[1].Trim()
    foreach ($n in @($from, $to)) { if (-not $modules.ContainsKey($n)) { $errors.Add("AllowedSameLayer entry '$entry' names unknown module '$n'.") } }
    [void]$allowed.Add("$from->$to")
}
$testSuffix = [string]$rules.TestModuleSuffix

# Whether the layer's SameLayer policy (or the test module rule) allows a same-layer dependency, without exceptions.
function Test-PolicyAllows($Mod, $Dep) {
    $samePlugin = $Dep.Plugin.Name -eq $Mod.Plugin.Name
    if ($samePlugin -and $testSuffix -and $Mod.Name.EndsWith($testSuffix)) { return $true }
    switch ($Mod.Plugin.Layer.SameLayer) {
        'Any' { return $true }
        'Plugin' { return $samePlugin }
        default { return $false }
    }
}

foreach ($entry in $allowed) {
    $from, $to = $entry -split '->'
    if (-not ($modules.ContainsKey($from) -and $modules.ContainsKey($to))) { continue }
    $fromMod = $modules[$from]; $toMod = $modules[$to]
    if ($fromMod.Plugin.Layer.Index -ne $toMod.Plugin.Layer.Index) {
        $errors.Add("AllowedSameLayer entry '$from -> $to' names modules in different layers.")
    }
    elseif (Test-PolicyAllows $fromMod $toMod) {
        $errors.Add("AllowedSameLayer entry '$from -> $to' is already allowed by the $($fromMod.Plugin.Layer.Name) layer's SameLayer policy; remove it.")
    }
}

# --- Module dependency rules --------------------------------------------------
foreach ($mod in $modules.Values) {
    foreach ($depName in $mod.Deps) {
        if (-not $modules.ContainsKey($depName)) { continue } # engine or other non-framework module
        $dep = $modules[$depName]
        $from = $mod.Plugin.Layer; $to = $dep.Plugin.Layer
        if ($to.Index -gt $from.Index) {
            $errors.Add("Upward dependency: $($mod.Name) ($($from.Name)) -> $depName ($($to.Name)).")
        }
        elseif ($to.Index -eq $from.Index) {
            if (-not (Test-PolicyAllows $mod $dep) -and -not $allowed.Contains("$($mod.Name)->$depName")) {
                $errors.Add("Same-layer dependency not allowed: $($mod.Name) -> $depName ($($from.Name), SameLayer '$($from.SameLayer)'). Add it to AllowedSameLayer if intended.")
            }
        }
        if ($dep.Plugin.Name -ne $mod.Plugin.Name -and ($mod.Plugin.PluginDeps -notcontains $dep.Plugin.Name)) {
            $errors.Add("Undeclared plugin dependency: module $($mod.Name) uses $depName, but $($mod.Plugin.Name).uplugin does not list plugin $($dep.Plugin.Name).")
        }
    }
}

# --- Plugin dependency rules ----------------------------------------------------
foreach ($p in $plugins.Values) {
    foreach ($depName in $p.PluginDeps) {
        if (-not $plugins.ContainsKey($depName)) { continue }
        $to = $plugins[$depName].Layer
        if ($to.Index -gt $p.Layer.Index) {
            $errors.Add("Upward plugin dependency: $($p.Name).uplugin ($($p.Layer.Name)) lists $depName ($($to.Name)).")
        }
    }
}

# --- Cycle detection (module graph) -----------------------------------------------
$state = @{}  # 1 = visiting, 2 = done
$stack = New-Object System.Collections.Generic.List[string]
function Visit([string]$Name) {
    $state[$Name] = 1; $stack.Add($Name)
    foreach ($d in $modules[$Name].Deps) {
        if (-not $modules.ContainsKey($d)) { continue }
        if ($state[$d] -eq 1) {
            $start = $stack.IndexOf($d)
            $cycle = (@($stack.GetRange($start, $stack.Count - $start)) + $d) -join ' -> '
            $errors.Add("Dependency cycle: $cycle.")
        }
        elseif (-not $state.ContainsKey($d)) { Visit $d }
    }
    $stack.RemoveAt($stack.Count - 1); $state[$Name] = 2
}
foreach ($n in @($modules.Keys | Sort-Object)) { if (-not $state.ContainsKey($n)) { Visit $n } }

# --- Cycle detection (plugin graph) ---------------------------------------------------
# Edges come from .uplugin dependencies and from modules that use another plugin's modules.
# A cycle here can exist even when the module graph is acyclic, and the engine cannot load it.
$pluginEdges = @{}
foreach ($p in $plugins.Values) {
    $set = New-Object System.Collections.Generic.HashSet[string]
    foreach ($d in $p.PluginDeps) { if ($plugins.ContainsKey($d) -and $d -ne $p.Name) { [void]$set.Add($d) } }
    $pluginEdges[$p.Name] = $set
}
foreach ($mod in $modules.Values) {
    foreach ($depName in $mod.Deps) {
        if (-not $modules.ContainsKey($depName)) { continue }
        $depPlugin = $modules[$depName].Plugin.Name
        if ($depPlugin -ne $mod.Plugin.Name) { [void]$pluginEdges[$mod.Plugin.Name].Add($depPlugin) }
    }
}
$pState = @{}
$pStack = New-Object System.Collections.Generic.List[string]
function VisitPlugin([string]$Name) {
    $pState[$Name] = 1; $pStack.Add($Name)
    foreach ($d in @($pluginEdges[$Name] | Sort-Object)) {
        if ($pState[$d] -eq 1) {
            $start = $pStack.IndexOf($d)
            $cycle = (@($pStack.GetRange($start, $pStack.Count - $start)) + $d) -join ' -> '
            $errors.Add("Plugin dependency cycle: $cycle.")
        }
        elseif (-not $pState.ContainsKey($d)) { VisitPlugin $d }
    }
    $pStack.RemoveAt($pStack.Count - 1); $pState[$Name] = 2
}
foreach ($n in @($plugins.Keys | Sort-Object)) { if (-not $pState.ContainsKey($n)) { VisitPlugin $n } }

# --- Report -------------------------------------------------------------------------
Write-Host "Checked $($plugins.Count) plugins and $($modules.Count) modules under $FrameworkRoot"
if ($errors.Count -gt 0) {
    foreach ($e in $errors) { Write-Host "  ERROR: $e" -ForegroundColor Red }
    Write-Host "$($errors.Count) layer rule violation(s)." -ForegroundColor Red
    exit 1
}
Write-Host "All layer rules pass." -ForegroundColor Green
exit 0
