param(
    [string]$DocumentsRoot = "C:\Users\aravi\OneDrive\Documents\Paradox Interactive\Europa Universalis V"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$debugLog = Join-Path $DocumentsRoot "logs\debug.log"
$testLog = Join-Path $DocumentsRoot "logs\game_tests.log"
$errorLog = Join-Path $DocumentsRoot "logs\error.log"

if (-not (Test-Path $debugLog))
{
    throw "Missing debug log: $debugLog"
}
if (-not (Test-Path $testLog))
{
    throw "Missing test log: $testLog"
}
if (-not (Test-Path $errorLog))
{
    throw "Missing error log: $errorLog"
}

$debugText = Get-Content -LiteralPath $debugLog -Raw
$testText = Get-Content -LiteralPath $testLog -Raw
$errorText = Get-Content -LiteralPath $errorLog -Raw

$requiredPatterns = @(
    "CK3EU5_VALIDATE_ON_GAME_START",
    "CK3EU5_VALIDATE_WAR_PRESENT",
    "CK3EU5_VALIDATE_COUNTRY_",
    "CK3EU5_VALIDATE_ARMY_PRESENT_",
    "CK3EU5_VALIDATE_WAR_LINK_"
)

$optionalPatterns = @(
    "CK3EU5_VALIDATE_NAVY_PRESENT_",
    "CK3EU5_VALIDATE_WAR_PARTICIPANT"
)

$missingErrors = @()
$unexpectedErrors = @()

foreach ($pattern in $requiredPatterns)
{
    if (($debugText -notmatch [regex]::Escape($pattern)) -and ($testText -notmatch [regex]::Escape($pattern)))
    {
        $missingErrors += $pattern
    }
}

foreach ($pattern in @("CK3EU5_VALIDATE_ARMY_MISSING_", "CK3EU5_VALIDATE_NAVY_MISSING_"))
{
    if ($errorText -match [regex]::Escape($pattern))
    {
        $unexpectedErrors += $pattern
    }
}

foreach ($pattern in @("CK3EU5_VALIDATE_WAR_LINK_MISSING_"))
{
    if ($errorText -match [regex]::Escape($pattern))
    {
        $unexpectedErrors += $pattern
    }
}

Write-Host "CK3EU5 validation log summary"
Write-Host "Debug log: $debugLog"
Write-Host "Test log:  $testLog"
Write-Host "Error log: $errorLog"
Write-Host ""

foreach ($pattern in $requiredPatterns)
{
    $present = ($debugText -match [regex]::Escape($pattern)) -or ($testText -match [regex]::Escape($pattern))
    Write-Host ("Required [{0}] {1}" -f ($(if ($present) { "OK" } else { "MISS" })), $pattern)
}

foreach ($pattern in $optionalPatterns)
{
    $present = ($debugText -match [regex]::Escape($pattern)) -or ($testText -match [regex]::Escape($pattern))
    Write-Host ("Optional [{0}] {1}" -f ($(if ($present) { "OK" } else { "MISS" })), $pattern)
}

foreach ($pattern in @("CK3EU5_VALIDATE_ARMY_MISSING_", "CK3EU5_VALIDATE_NAVY_MISSING_", "CK3EU5_VALIDATE_WAR_LINK_MISSING_"))
{
    $present = $errorText -match [regex]::Escape($pattern)
    Write-Host ("Error    [{0}] {1}" -f ($(if ($present) { "FOUND" } else { "CLEAR" })), $pattern)
}

if ($missingErrors.Count -gt 0 -or $unexpectedErrors.Count -gt 0)
{
    throw "Validation log check failed. Missing required markers: $($missingErrors -join ', '). Error markers present: $($unexpectedErrors -join ', ')"
}

Write-Host ""
Write-Host "Validation log check passed."
