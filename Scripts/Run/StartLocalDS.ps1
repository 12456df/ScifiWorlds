[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateScript({ Test-Path -LiteralPath $_ -PathType Leaf })]
    [string]$ServerExecutable,

    [string]$Map = '/Game/PolygonSciFiWorlds/Maps/Demo_BlackMarket',

    [ValidateRange(1, 65535)]
    [int]$Port = 7777,

    [ValidateRange(1, 120)]
    [int]$StartupTimeoutSeconds = 30,

    [switch]$WaitForPort
)

$ErrorActionPreference = 'Stop'
$resolvedServerExecutable = (Resolve-Path -LiteralPath $ServerExecutable).Path

function Get-ChildProcessIds {
    param(
        [Parameter(Mandatory)]
        [int]$ParentProcessId
    )

    $childIds = @(Get-CimInstance Win32_Process -Filter "ParentProcessId = $ParentProcessId" -ErrorAction SilentlyContinue |
        Select-Object -ExpandProperty ProcessId)

    foreach ($childId in $childIds) {
        $childId
        Get-ChildProcessIds -ParentProcessId $childId
    }
}

function Stop-ProcessTree {
    param(
        [Parameter(Mandatory)]
        [int]$RootProcessId
    )

    $childIds = @(Get-ChildProcessIds -ParentProcessId $RootProcessId)
    foreach ($childId in ($childIds | Sort-Object -Descending -Unique)) {
        Stop-Process -Id $childId -ErrorAction SilentlyContinue
    }

    Stop-Process -Id $RootProcessId -ErrorAction SilentlyContinue
}

function Get-ListeningUdpEndpoint {
    param(
        [Parameter(Mandatory)]
        [int]$LocalPort
    )

    Get-NetUDPEndpoint -LocalPort $LocalPort -ErrorAction SilentlyContinue |
        Select-Object -First 1
}

if (Get-ListeningUdpEndpoint -LocalPort $Port) {
    throw "Port $Port is already in use. Dedicated Server was not started."
}

$logDirectory = Join-Path (Split-Path -Parent $resolvedServerExecutable) 'Logs'
New-Item -ItemType Directory -Force -Path $logDirectory | Out-Null
$logFile = Join-Path $logDirectory "PolygonScifiWorldsServer-$Port.log"
$arguments = @($Map, "-port=$Port", '-log', '-unattended', '-NoSound', "-abslog=$logFile")
$process = Start-Process -FilePath $resolvedServerExecutable -ArgumentList $arguments -PassThru -WindowStyle Hidden

if (-not $WaitForPort) {
    $process
    return
}

$deadline = (Get-Date).AddSeconds($StartupTimeoutSeconds)
do {
    Start-Sleep -Milliseconds 500

    $endpoint = Get-ListeningUdpEndpoint -LocalPort $Port
    if ($endpoint) {
        [PSCustomObject]@{
            LauncherProcessId = $process.Id
            ServerProcessId = $endpoint.OwningProcess
            Protocol = 'UDP'
            Port = $Port
            LogFile = $logFile
        }
        return
    }

    if ($process.HasExited) {
        throw "Dedicated Server launcher exited with code $($process.ExitCode). See $logFile"
    }
} while ((Get-Date) -lt $deadline)

Stop-ProcessTree -RootProcessId $process.Id
throw "Dedicated Server did not listen on port $Port within $StartupTimeoutSeconds seconds. See $logFile"
