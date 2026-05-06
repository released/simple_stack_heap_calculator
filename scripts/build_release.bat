@echo off
setlocal

set "ROOT=%~dp0.."
set "EXE=%ROOT%\build\SimpleStackHeapCalculator.exe"
set "LEGACY_PDB=%ROOT%\build\SimpleStackHeapCalculator.pdb"

pushd "%ROOT%"
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$proc = Get-Process -Name 'SimpleStackHeapCalculator' -ErrorAction SilentlyContinue;" ^
  "if ($proc) { $proc | Stop-Process -Force }"
if exist "%LEGACY_PDB%" del /f /q "%LEGACY_PDB%" >nul 2>nul
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$vswhere = Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Microsoft Visual Studio\Installer\vswhere.exe';" ^
  "if (!(Test-Path $vswhere)) { Write-Error 'vswhere not found. Please install Visual Studio.'; exit 1 };" ^
  "$msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1;" ^
  "if (!$msbuild) { Write-Error 'MSBuild not found.'; exit 1 };" ^
  "& $msbuild 'SimpleStackHeapCalculator.sln' /p:Configuration=Release /p:Platform=x64 /m;" ^
  "exit $LASTEXITCODE"
if errorlevel 1 (
  set "RC=%ERRORLEVEL%"
  popd
  endlocal
  exit /b %RC%
)

echo Build complete: build\SimpleStackHeapCalculator.exe
if not exist "%EXE%" (
  echo Build succeeded but executable not found: "%EXE%"
  popd
  endlocal
  exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$proc = Get-Process -Name 'SimpleStackHeapCalculator' -ErrorAction SilentlyContinue;" ^
  "if ($proc) { $proc | Stop-Process -Force; Start-Sleep -Milliseconds 300 };" ^
  "Start-Process -FilePath '%EXE%'"

popd
endlocal
exit /b 0
