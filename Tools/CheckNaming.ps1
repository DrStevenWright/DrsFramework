# Copyright (c) 2026 Steven Wright. Licensed under the MIT License.
<#
.SYNOPSIS
    Checks that DrsFramework writes DRS (not Drs) in categories, class groups and plugin descriptors.

.DESCRIPTION
    Scans the framework (skipping Platform/External, Intermediate, Binaries and Saved) and fails on:
      - a reflection Category whose root is not DRS, such as Category = "Drs|Input"
      - ClassGroup = (Drs)
      - a settings class that returns or sets a category other than DRS (GetCategoryName, CategoryName)
      - a .uplugin FriendlyName, Description or Category that says Drs
    Identifiers, repo URLs and comments are not checked. See "Spelling of DRS" in Docs/CodingConventions.md.
    Works with Windows PowerShell 5.1 and PowerShell 7.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File Tools/CheckNaming.ps1
#>
[CmdletBinding()]
param(
    [string]$FrameworkRoot
)

$ErrorActionPreference = 'Stop'
if (-not $FrameworkRoot) { $FrameworkRoot = Split-Path -Parent $PSScriptRoot }
$FrameworkRoot = (Resolve-Path $FrameworkRoot).Path
$errors = New-Object System.Collections.Generic.List[string]

function Get-RelativePath([string]$Path) {
    return $Path.Substring($FrameworkRoot.Length).TrimStart('\', '/').Replace('\', '/')
}

$skip = '/(Platform/External|Intermediate|Binaries|Saved)/'
$files = Get-ChildItem -Path $FrameworkRoot -Recurse -File -Include *.h, *.cpp, *.uplugin |
    Where-Object { ('/' + (Get-RelativePath $_.FullName)) -notmatch $skip }

$codeChecks = @(
    @{ Pattern = 'Category\s*=\s*"(?!DRS\b)[Dd][Rr][Ss]\|'; Message = 'Category root must be DRS' },
    @{ Pattern = 'ClassGroup\s*=\s*\((?!DRS\))[Dd][Rr][Ss]\)'; Message = 'ClassGroup must be (DRS)' },
    @{ Pattern = '(GetCategoryName\(\)|CategoryName\s*=).*TEXT\("(?!DRS"\))'; Message = 'Settings category must be DRS' }
)
$pluginChecks = @(
    @{ Pattern = '"(FriendlyName|Category)"\s*:\s*"(?!DRS\b)[Dd][Rr][Ss]'; Message = 'FriendlyName and Category must start with DRS' },
    @{ Pattern = '"Description"\s*:\s*"[^"]*\bDrs\b(?!\w)'; Message = 'Description must say DRS' }
)

foreach ($file in $files) {
    $rel = Get-RelativePath $file.FullName
    $checks = if ($file.Extension -eq '.uplugin') { $pluginChecks } else { $codeChecks }
    $lineNo = 0
    foreach ($line in (Get-Content $file.FullName)) {
        $lineNo++
        foreach ($check in $checks) {
            if ($line -cmatch $check.Pattern) {
                $errors.Add("${rel}:${lineNo}: $($check.Message): $($line.Trim())")
            }
        }
    }
}

if ($errors.Count -gt 0) {
    $errors | ForEach-Object { Write-Host $_ }
    Write-Host "$($errors.Count) naming violation(s)."
    exit 1
}
Write-Host "Checked $($files.Count) files under $FrameworkRoot"
Write-Host 'All naming rules pass.'
