param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectRoot
)

$ProjectRoot = $ProjectRoot.Trim().Trim('"')
$counterPath = Join-Path $ProjectRoot "version.counter"
$headerPath = Join-Path $ProjectRoot "src\generated\build_version.h"

$counter = 0
if (Test-Path $counterPath) {
    $raw = (Get-Content $counterPath -Raw).Trim()
    if ($raw -match '^\d+$') {
        $counter = [int]$raw
    }
}

$counter++
Set-Content -Path $counterPath -Value $counter -NoNewline

$now = Get-Date
$versionText = "1.0.$counter"
$dateText = $now.ToString("yyyy-MM-dd HH:mm:ss")

$content = @"
#pragma once

namespace buildinfo {

static constexpr int kVersionMajor = 1;
static constexpr int kVersionMinor = 0;
static constexpr int kVersionBuild = $counter;
static constexpr wchar_t kBuildVersionText[] = L"$versionText";
static constexpr wchar_t kBuildDateText[] = L"$dateText";

}  // namespace buildinfo
"@

Set-Content -Path $headerPath -Value $content -NoNewline
