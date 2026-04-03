param(
    [string]$ProjectRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$DocumentsRoot = "C:\Users\aravi\OneDrive\Documents\Paradox Interactive\Europa Universalis V",
    [string]$Ck3DocumentsRoot = "C:\Users\aravi\OneDrive\Documents\Paradox Interactive\Crusader Kings III",
    [string]$Ck3GamePath = "C:\Program Files (x86)\Steam\steamapps\common\Crusader Kings III",
    [string]$Eu5GamePath = "C:\Program Files (x86)\Steam\steamapps\common\Europa Universalis V\game",
    [string]$ModFolderName = "ck3_to_eu5_validation_sample",
    [string]$SupportedGameVersion = "1.1.10",
    [string]$PlaysetName = "CK3EU5 Validation",
    [bool]$ActivatePlayset = $true,
    [bool]$CleanupOldValidationMods = $true,
    [bool]$MinimalGovernmentSetup = $false,
    [bool]$ValidationForceMonarchy = $false,
    [int]$ValidationCountryOffset = 0,
    [int]$ValidationCountryLimit = 0
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function To-ForwardSlashes([string]$PathValue)
{
    return ($PathValue -replace "\\", "/")
}

function Write-Utf8NoBom([string]$PathValue, [string]$Content)
{
    $encoding = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($PathValue, $Content, $encoding)
}

function Remove-OldValidationMods([string]$DocumentsPath, [string]$KeepFolderName)
{
    $modsRoot = Join-Path $DocumentsPath "mod"
    if (-not (Test-Path $modsRoot))
    {
        return
    }

    Get-ChildItem -LiteralPath $modsRoot -Directory -Filter "ck3_to_eu5_validation_sample*" |
        Where-Object { $_.Name -ne $KeepFolderName } |
        ForEach-Object {
            try
            {
                Remove-Item -LiteralPath $_.FullName -Recurse -Force
            }
            catch
            {
                throw "Failed to remove stale validation mod '$($_.FullName)'. Close EU5 and the crash reporter, then rerun deployment."
            }
        }

    $remaining = Get-ChildItem -LiteralPath $modsRoot -Directory -Filter "ck3_to_eu5_validation_sample*" |
        Where-Object { $_.Name -ne $KeepFolderName }
    if ($remaining)
    {
        $names = ($remaining | ForEach-Object { $_.Name }) -join ", "
        throw "Stale validation mods still present after cleanup: $names"
    }
}

$buildDir = Join-Path $ProjectRoot "build_nf"
$exePath = Join-Path $buildDir "ck3_to_eu5.exe"
if (-not (Test-Path $exePath))
{
    cmake --build $buildDir --target ck3_to_eu5 --config Debug | Out-Host
}

if (-not (Test-Path $exePath))
{
    throw "Converter executable not found at $exePath"
}

$modPath = Join-Path $DocumentsRoot ("mod\" + $ModFolderName)
$runtimeConfigPath = Join-Path $buildDir "sample_validation_runtime.cfg"
$reportPath = Join-Path $buildDir "eu5_validation_deploy_report.txt"
$playsetsPath = Join-Path $DocumentsRoot "playsets.json"
$playsetsBackupPath = Join-Path $DocumentsRoot "playsets.ck3eu5_validation_backup.json"
$resolvedSupportedGameVersion = $SupportedGameVersion
$defaultValidationCk3Input = Join-Path $Ck3DocumentsRoot "save games\Vanilla_probe.ck3"
$fallbackValidationInput = Join-Path $ProjectRoot "examples\sample_ck3_world.pds"
$validationCk3Input = if (Test-Path $defaultValidationCk3Input) { $defaultValidationCk3Input } else { $fallbackValidationInput }
$generatedProvinceMappings = Join-Path $ProjectRoot "data\generated\map_correspondence\augmented_mappings.csv"
$validationProvinceMappings = if (Test-Path $generatedProvinceMappings) { $generatedProvinceMappings } else { Join-Path $ProjectRoot "data\configurables\province_mappings.csv" }
$generatedLiveLocationFramework = Join-Path $ProjectRoot "data\generated\eu5_live\location_framework.csv"
$validationLocationFramework = if (Test-Path $generatedLiveLocationFramework) { $generatedLiveLocationFramework } else { Join-Path $ProjectRoot "data\configurables\location_framework.csv" }
$generatedLiveColors = Join-Path $ProjectRoot "data\generated\eu5_live\country_colors.csv"
$validationCountryColors = if (Test-Path $generatedLiveColors) { $generatedLiveColors } else { Join-Path $ProjectRoot "data\configurables\country_colors.csv" }

if ($CleanupOldValidationMods)
{
    Remove-OldValidationMods -DocumentsPath $DocumentsRoot -KeepFolderName $ModFolderName
}

$configLines = @(
    "ck3_input = $(To-ForwardSlashes $validationCk3Input)"
    "ck3_game_path = $(To-ForwardSlashes $Ck3GamePath)"
    "output_mod_path = $(To-ForwardSlashes $modPath)"
    "eu5_game_path = $(To-ForwardSlashes $Eu5GamePath)"
    "location_framework = $(To-ForwardSlashes $validationLocationFramework)"
    "province_mappings = $(To-ForwardSlashes $validationProvinceMappings)"
    "title_mappings = $(To-ForwardSlashes (Join-Path $ProjectRoot 'data\configurables\title_mappings.csv'))"
    "culture_mappings = $(To-ForwardSlashes (Join-Path $ProjectRoot 'data\configurables\culture_mappings.csv'))"
    "religion_mappings = $(To-ForwardSlashes (Join-Path $ProjectRoot 'data\configurables\religion_mappings.csv'))"
    "government_mappings = $(To-ForwardSlashes (Join-Path $ProjectRoot 'data\configurables\government_mappings.csv'))"
    "country_colors = $(To-ForwardSlashes $validationCountryColors)"
    ""
    "mod_name = CK3 to EU5 Validation Sample"
    "mod_id = $ModFolderName"
    "mod_version = 0.1.0"
    "supported_game_version = $resolvedSupportedGameVersion"
    ""
    "verbose_logging = yes"
    "write_debug_snapshots = yes"
    "minimal_government_setup = $(if ($MinimalGovernmentSetup) { 'yes' } else { 'no' })"
    "validation_force_monarchy = $(if ($ValidationForceMonarchy) { 'yes' } else { 'no' })"
    "auto_normalize_raw_ck3 = yes"
    "prefer_subject_realms = yes"
    "minimum_subject_counties = 2"
    "default_technology_level = 1"
    "default_gold = 100"
)

if ($ValidationCountryOffset -gt 0)
{
    $configLines += "validation_country_offset = $ValidationCountryOffset"
}
if ($ValidationCountryLimit -gt 0)
{
    $configLines += "validation_country_limit = $ValidationCountryLimit"
}

Write-Utf8NoBom $runtimeConfigPath (($configLines -join "`r`n") + "`r`n")

& $exePath $runtimeConfigPath
if ($LASTEXITCODE -ne 0 -and $LASTEXITCODE -ne 2)
{
    throw "Converter run failed with exit code $LASTEXITCODE"
}

if (-not (Test-Path $playsetsBackupPath) -and (Test-Path $playsetsPath))
{
    Copy-Item -LiteralPath $playsetsPath -Destination $playsetsBackupPath
}

if (Test-Path $playsetsPath)
{
    $playsetsData = Get-Content -LiteralPath $playsetsPath -Raw | ConvertFrom-Json
}
else
{
    $playsetsData = [pscustomobject]@{
        file_version = "1.0.0"
        playsets = @()
    }
}

$playsets = @($playsetsData.playsets)
$referencePlayset = $playsets | Where-Object { $_.PSObject.Properties.Name -contains 'isActive' -and $_.isActive } | Select-Object -First 1
if (-not $referencePlayset)
{
    $referencePlayset = $playsets | Select-Object -First 1
}
$referenceDlc = @()
if ($referencePlayset -and $referencePlayset.PSObject.Properties.Name -contains 'DLC' -and $referencePlayset.DLC)
{
    $referenceDlc = @($referencePlayset.DLC)
}

if ($ActivatePlayset)
{
    foreach ($existingPlayset in $playsets)
    {
        if ($existingPlayset.PSObject.Properties.Name -contains 'isActive')
        {
            $existingPlayset.isActive = $false
        }
    }
}

$validationPlayset = $playsets | Where-Object { $_.name -eq $PlaysetName } | Select-Object -First 1
if (-not $validationPlayset)
{
    $validationPlayset = [pscustomobject]@{
        name = $PlaysetName
        isAutomaticallySorted = $true
        orderedListMods = @()
        DLC = $referenceDlc
    }
    if ($ActivatePlayset)
    {
        $validationPlayset | Add-Member -NotePropertyName isActive -NotePropertyValue $true
    }
    $playsets = @($playsets) + $validationPlayset
}
elseif ($ActivatePlayset)
{
    if ($validationPlayset.PSObject.Properties.Name -contains 'isActive')
    {
        $validationPlayset.isActive = $true
    }
    else
    {
        $validationPlayset | Add-Member -NotePropertyName isActive -NotePropertyValue $true
    }
}

$validationPlayset.isAutomaticallySorted = $true
$validationPlayset.orderedListMods = @([pscustomobject]@{
    path = (To-ForwardSlashes $modPath) + "/"
    isEnabled = $true
})
if ($referenceDlc.Count -gt 0)
{
    $validationPlayset.DLC = $referenceDlc
}

$playsetsData.playsets = $playsets
Write-Utf8NoBom $playsetsPath (($playsetsData | ConvertTo-Json -Depth 10) + "`r`n")

$reportLines = @(
    "CK3 to EU5 EU5 validation deployment complete.",
    "Generated mod path: $modPath",
    "Runtime config: $runtimeConfigPath",
    "CK3 input: $validationCk3Input",
    "CK3 game path: $Ck3GamePath",
    "Province mappings: $validationProvinceMappings",
    "Location framework: $validationLocationFramework",
    "Country colors: $validationCountryColors",
    "Supported game version: $resolvedSupportedGameVersion",
    "Playset name: $PlaysetName",
    "Playset activated: $ActivatePlayset",
    "Minimal government setup: $MinimalGovernmentSetup",
    "Validation force monarchy: $ValidationForceMonarchy",
    "Validation country offset: $ValidationCountryOffset",
    "Validation country limit: $ValidationCountryLimit",
    "Playsets file: $playsetsPath",
    "Playsets backup: $playsetsBackupPath",
    "",
    "Launch EU5 in debug mode, enter a game with this playset active, then inspect:",
    " - $DocumentsRoot\logs\debug.log",
    " - $DocumentsRoot\logs\game_tests.log",
    " - $DocumentsRoot\logs\error.log",
    "",
    "Expected validation tokens:",
    " - CK3EU5_VALIDATE_ON_GAME_START",
    " - CK3EU5_VALIDATE_WAR_PRESENT",
    " - CK3EU5_VALIDATE_WAR_PARTICIPANT",
    " - CK3EU5_VALIDATE_COUNTRY_<TAG>_START",
    " - CK3EU5_VALIDATE_ARMY_PRESENT_<TAG>",
    " - CK3EU5_VALIDATE_NAVY_PRESENT_<TAG>"
)
Write-Utf8NoBom $reportPath (($reportLines -join "`r`n") + "`r`n")

Write-Host "Validation mod deployed to $modPath"
Write-Host "Validation playset prepared: $PlaysetName"
Write-Host "Deployment report: $reportPath"
