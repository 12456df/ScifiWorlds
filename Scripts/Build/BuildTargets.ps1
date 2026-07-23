[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateScript({ Test-Path -LiteralPath $_ -PathType Container })]
    [string]$EngineRoot,

    [ValidateScript({ Test-Path -LiteralPath $_ -PathType Leaf })]
    [string]$ProjectFile = (Join-Path $PSScriptRoot '..\\..\\PolygonScifiWorlds.uproject')
)

$ErrorActionPreference = 'Stop'
$resolvedEngineRoot = (Resolve-Path -LiteralPath $EngineRoot).Path
$resolvedProjectFile = (Resolve-Path -LiteralPath $ProjectFile).Path
$buildBat = Join-Path $resolvedEngineRoot 'Engine\\Build\\BatchFiles\\Build.bat'

if (-not (Test-Path -LiteralPath $buildBat -PathType Leaf)) {
    throw "Build.bat was not found under EngineRoot: $resolvedEngineRoot"
}

$projectRoot = Split-Path -Parent $resolvedProjectFile
$logDirectory = Join-Path $projectRoot 'Saved\\Logs\\Build'
New-Item -ItemType Directory -Force -Path $logDirectory | Out-Null

foreach ($target in @('PolygonScifiWorldsEditor', 'PolygonScifiWorlds', 'PolygonScifiWorldsServer')) {
    $logFile = Join-Path $logDirectory "$target-Development-Win64.log"
    Write-Host "Building $target (Development Win64)..."
    & $buildBat $target Win64 Development "-Project=$resolvedProjectFile" -WaitMutex -NoHotReloadFromIDE *>&1 | Tee-Object -FilePath $logFile

    if ($LASTEXITCODE -ne 0) {
        throw "Build failed for $target. See $logFile"
    }
}
