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

$targetExe = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\build\SimpleStackHeapCalculator.exe"))
foreach ($running in @(Get-Process -Name "SimpleStackHeapCalculator" -ErrorAction SilentlyContinue)) {
    # Never close a same-named executable from a different checkout or fixture.
    try { $runningPath = $running.Path; $started = $running.StartTime } catch { continue }
    if (-not [string]::Equals($runningPath, $targetExe, [StringComparison]::OrdinalIgnoreCase)) { continue }
    $targetPid = $running.Id
    if ($running.MainWindowHandle -ne [IntPtr]::Zero) { [void]$running.CloseMainWindow() }
    if (-not $running.WaitForExit(5000)) {
        $current = Get-Process -Id $targetPid -ErrorAction SilentlyContinue
        if ($current -and $current.StartTime -eq $started -and
            [string]::Equals($current.Path, $targetExe, [StringComparison]::OrdinalIgnoreCase)) {
            # This read-only map viewer has no unsaved-document close prompt.
            Stop-Process -Id $targetPid -Force -ErrorAction Stop
            if (-not $current.WaitForExit(5000)) { throw "Exact build target PID $targetPid did not exit." }
            Write-Host "Closed exact build target PID $targetPid (forced after normal-close timeout)."
        }
    } else {
        Write-Host "Closed exact build target PID $targetPid normally."
    }
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
