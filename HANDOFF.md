# HANDOFF.md

## Adopted x64 size reduction 2026-10-25

Formal EXE now 1,197,056 bytes (was 3,286,528); SHA and raw paired measurements
are in SELF_CHECK. Only unused x64 MFC registration was excluded; ordinary
controls, parser support, static runtime, /O2 and security are retained. Canonical
Release build, isolated actual-EXE analysis/report/metrics/dialog/normal-close,
Debug workflow and console/error regressions pass. Seven alternating performance
pairs stay within baseline-derived limits. Actual clean removed 37 compiler
files, retaining the formal EXE hash; nested fixtures/help/idempotence pass.
Visual/DPI/clean-PC and Win32 acceptance are not implied. User authorized the
isolated test-script fallback. Shared progress records publication; the following
pilot checkpoint is historical, not the current adoption state.

## Size pilot checkpoint 2026-10-24

IN PROGRESS, not a completed release. Added the standard-control MFC exclusion,
separate core regression target (not in production), and exact-path build closure
instead of the old broad process-name kill. Release baseline/candidate and Debug
compile pass; core totals/error fixtures pass. Isolated candidate is 1,197,568
bytes versus 3,286,528, preserving /O2/static runtime/security. Formal EXE remains
unchanged. SELF_CHECK records hashes and missing desktop/performance gates;
Computer Use native pipe is unavailable. Do not publish or promote until those
checks, canonical build and clean-preservation verification are complete.

## Clean Safety Follow-up 2026-10-20

- Replaced named-directory purges with a standalone compiler-file allowlist; canonical and legacy entries share the same helper and support non-destructive help.
- PASS: shared verifier `-CleanBatch scripts/clean_mfc.bat -CleanActualProject -CheckNestedDelivery -CheckReparseRejection`: 17 fixture build sentinels plus source, help/no-deletion, object removal, repeat-clean and junction rejection before deletion.
- PASS: 1 actual protected build files unchanged (SHA-256).
- Application source/project resources are unchanged from verified baseline `fdbd98dfd5f0ddb784a764d48c822a8b9f1d9ca6`. The Release x64 build already passed earlier in this migration; it was not rebuilt or version-bumped just for this clean-only follow-up. This is not a new UI/hardware acceptance claim.
- NOT RUN: new functional/device/full visual/DPI/Debug acceptance; no hardware writes or user-instance closure.
- Final commit/publication evidence: workspace `#READ_ME_FIRST#/PROJECT_LAYOUT_MIGRATION.md`.

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
