param(
    [string]$Image = "xiashj/logue-sdk:latest"
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$sdk = Join-Path $root "logue-sdk"
$platform = Join-Path $sdk "platform"
$source = Join-Path $root "units\delfx\graindrift"
$dest = Join-Path $sdk "platform\drumlogue\taureon-graindrift"
$template = Join-Path $sdk "platform\drumlogue\dummy-delfx"

if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
    throw "Docker was not found in PATH. Install Docker Desktop or add docker.exe to PATH."
}

if (-not (Test-Path -LiteralPath $platform -PathType Container)) {
    throw "Could not find SDK platform directory: $platform"
}

if (-not (Test-Path -LiteralPath $source -PathType Container)) {
    throw "Could not find GrainDrift source directory: $source"
}

if (-not (Test-Path -LiteralPath $dest -PathType Container)) {
    if (-not (Test-Path -LiteralPath $template -PathType Container)) {
        throw "Could not find drumlogue delfx template: $template"
    }
    Copy-Item -Recurse -LiteralPath $template -Destination $dest
}

Copy-Item -LiteralPath `
    (Join-Path $source "config.mk"), `
    (Join-Path $source "header.c"), `
    (Join-Path $source "unit.cc"), `
    (Join-Path $source "graindrift.h"), `
    (Join-Path $source "Makefile") `
    -Destination $dest -Force

docker run --rm `
    -v "${platform}:/workspace" `
    -h logue-sdk `
    $Image `
    /app/cmd_entry build drumlogue/taureon-graindrift
