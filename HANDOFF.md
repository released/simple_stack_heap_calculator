# HANDOFF.md

## Layout Audit 2026-10-20

- Screenshot moved to `docs/images/keil_v6.jpg` with no overwrite and SHA-256 verification; README uses a relative link instead of an old repository URL.
- Added compatible `scripts/build_release_mfc.bat` and `scripts/clean_mfc.bat` aliases. Original entries and application behavior remain unchanged.
- PASS: `scripts/build.ps1 -Configuration Release -Platform x64 -NoRun`; Release x64, 0 warnings/errors. No user instance was force-closed and no device was connected for the audit.
- PASS: actual `scripts/clean_mfc.bat`; 1 existing protected build files unchanged by SHA-256.
- PASS: isolated clean sentinels/idempotence (9 protected build files plus source).
- NOT RUN: fresh UI/device/parser functional acceptance; this is a directory/script-alias change, not a claim that every product checklist passed.
- Commit/push status is recorded in workspace `#READ_ME_FIRST#/PROJECT_LAYOUT_MIGRATION.md`.

## Date / Workspace

- Date: 2026-05-06
- Workspace: `simple_stack_heap_calculator`

## Current Authority

- Active implementation baseline: `AGENTS.md`
- User-facing project overview: `README.md`
- Verification checklist: `SELF_CHECK.md`

## Implemented State

1. Core product baseline
- MFC EXE created with static MFC / CRT build settings
- UI supports map-file browse, detected format display, manual parser fallback, RAM limit entry, parse action, log output, report output, and stack/heap recommendation table

2. Map detection and parser architecture
- Weighted map-format detector implemented
- Parser interface and parser factory split by compiler family
- Automatic detection is used before manual fallback selection

3. Parser coverage
- `KEIL`, `GCC`, and `IAR` implemented
- `RL78` and `RH850` provide partial parsing
- `GHS` currently provides detection only

4. Build / resource baseline
- Custom application icon is wired into the EXE resource
- Build version and build date are generated during build
- Build scripts and clean script are present under `scripts\`

5. Portable config baseline
- The tool now creates `build\SimpleStackHeapCalculator.ini` automatically
- Current persisted fields:
  - last map path
  - RAM limit text
- Manual parser now always defaults to `KEIL` on startup
- If auto-detection disagrees with manual selection, the detected checkbox is highlighted with `[Auto]`

## Main Files

- `src\ui\main_view.*`
- `src\core\analysis_engine.*`
- `src\detector\map_format_detector.*`
- `src\parser\*.cpp`
- `scripts\build.ps1`
- `scripts\build_release.bat`
- `scripts\clean_build.bat`
- `SimpleStackHeapCalculator.vcxproj`

## Build Verification

- Command: `powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build.ps1 -Configuration Release -Platform x64 -NoRun`
- Result: success
- Output: `build\SimpleStackHeapCalculator.exe`
- Config output: `build\SimpleStackHeapCalculator.ini`
- PDB output: `build\obj\x64\Release\SimpleStackHeapCalculator.pdb`
- Smoke launch: rerun, process stayed alive after startup
- Config creation: verified, missing INI is auto-created on startup
- Config persistence smoke test: verified with seeded values for map path / RAM limit

## Open Issues / Risks

- `RL78` and `RH850` are still partial parsers and need broader real-world map validation
- `GHS` detection exists, but full parse flow is not implemented

## Document Sync

- `AGENTS.md`: updated
- `README.md`: updated
- `SELF_CHECK.md`: updated
- `HANDOFF.md`: updated
