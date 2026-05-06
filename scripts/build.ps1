param(
    [ValidateSet("Debug", "Release")][string]$Configuration = "Release",
    [ValidateSet("Win32", "x64")][string]$Platform = "x64",
    [switch]$NoRun
)

$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    Write-Error "vswhere not found. Please run from a Visual Studio Developer PowerShell or install Visual Studio."
    exit 1
}

$msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
if (-not $msbuild) {
    Write-Error "MSBuild not found."
    exit 1
}

$running = Get-Process -Name "SimpleStackHeapCalculator" -ErrorAction SilentlyContinue
if ($running) {
    $running | Stop-Process -Force
    Start-Sleep -Milliseconds 300
}

$legacyPdb = Join-Path "build" "SimpleStackHeapCalculator.pdb"
if (Test-Path $legacyPdb) {
    Remove-Item -Force $legacyPdb
}

& $msbuild "SimpleStackHeapCalculator.sln" /p:Configuration=$Configuration /p:Platform=$Platform /m
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$exe = Join-Path "build" "SimpleStackHeapCalculator.exe"
if (-not (Test-Path $exe)) {
    Write-Error "Build succeeded but $exe was not found."
    exit 1
}

Write-Host "Built: $exe"

if ($NoRun) {
    Write-Host "Skip launch: -NoRun specified."
    exit 0
}

$resolvedExe = (Resolve-Path $exe).Path
Start-Process -FilePath $resolvedExe | Out-Null
Write-Host "Launched: $exe"
