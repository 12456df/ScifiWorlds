[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateScript({ Test-Path -LiteralPath $_ -PathType Container })]
    [string]$EngineRoot,

    [Parameter(Mandatory)]
    [string]$ArchiveDirectory,

    [ValidateScript({ Test-Path -LiteralPath $_ -PathType Leaf })]
    [string]$ProjectFile = (Join-Path $PSScriptRoot '..\\..\\PolygonScifiWorlds.uproject'),

    [string]$Map = '/Game/PolygonSciFiWorlds/Maps/Demo_BlackMarket'
)

$ErrorActionPreference = 'Stop'
$resolvedEngineRoot = (Resolve-Path -LiteralPath $EngineRoot).Path
$resolvedProjectFile = (Resolve-Path -LiteralPath $ProjectFile).Path
$runUat = Join-Path $resolvedEngineRoot 'Engine\\Build\\BatchFiles\\RunUAT.bat'

if (-not (Test-Path -LiteralPath $runUat -PathType Leaf)) {
    throw "RunUAT.bat was not found under EngineRoot: $resolvedEngineRoot"
}

$resolvedArchiveDirectory = [System.IO.Path]::GetFullPath($ArchiveDirectory)
$projectRoot = Split-Path -Parent $resolvedProjectFile
$normalizedProjectRoot = $projectRoot.TrimEnd('\', '/')
$normalizedArchiveDirectory = $resolvedArchiveDirectory.TrimEnd('\', '/')

if ([string]::IsNullOrWhiteSpace($Map) -or -not $Map.StartsWith('/Game/', [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Map must be a project long package name beginning with '/Game/': $Map"
}

$relativeMapPath = $Map.Substring('/Game/'.Length).Replace('/', '\')
$mapFile = Join-Path $projectRoot (Join-Path 'Content' "$relativeMapPath.umap")

if (-not (Test-Path -LiteralPath $mapFile -PathType Leaf)) {
    throw "Map was not found in the project Content directory: $Map ($mapFile)"
}

if ($normalizedArchiveDirectory.Equals($normalizedProjectRoot, [System.StringComparison]::OrdinalIgnoreCase) -or
    $normalizedArchiveDirectory.StartsWith("$normalizedProjectRoot\", [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "ArchiveDirectory must be outside the project repository: $normalizedArchiveDirectory"
}

New-Item -ItemType Directory -Force -Path $resolvedArchiveDirectory | Out-Null

& $runUat BuildCookRun "-project=$resolvedProjectFile" -noP4 -server -noclient -serverplatform=Win64 -serverconfig=Development -build -cook -stage -pak -archive "-archivedirectory=$resolvedArchiveDirectory" "-map=$Map" -utf8output

if ($LASTEXITCODE -ne 0) {
    throw "Server Cook/Stage failed. Inspect the AutomationTool log and archive directory: $resolvedArchiveDirectory"
}
