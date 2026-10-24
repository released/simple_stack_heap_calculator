# SELF_CHECK.md

## Adopted x64 size reduction 2026-10-25

- Exact formal Release EXE: 1,197,056 bytes (baseline 3,286,528), SHA-256
  `61A797D22C36DE693613C696E9CC73F7B29AD4B190C35C8BCA7B69EC16D7514D`.
  No new required files/DLLs; complete EXE-only delivery saves 2,089,472 bytes.
  Reduction is x64-only; Win32 definitions and /O2 /GL /MT /GS/PE 0x8160 remain.
- PASS: canonical scripts/build.ps1 -Configuration Release -Platform x64 -NoRun,
  zero warnings/errors. Original verified EXE retained outside the project.
- PASS: authorized isolated formal-EXE GUI analysis (GCC 1,120-byte flash result,
  metrics rows), identical baseline/candidate report/control-class shape, owned
  Browse dialog open/cancel and normal exit. Candidate Debug exercised the same
  workflow/dialog with no assertion. Separate console core fixtures also pass.
- PASS scoped performance: two warmups, seven baseline calibration samples,
  seven alternating AB/BA measured pairs, no concurrent compilation. Limits
  fixed from baseline before candidate: delta max(floor, 10% baseline median,
  3*MAD), memory 5%; floors startup 25ms/workflow 2ms/CPU 16ms/memory 1MiB.
  Median baseline -> formal: startup 389.9096 -> 396.6633ms; five GUI analyses
  115.2144 -> 113.8591ms; process CPU 390.625 -> 421.875ms; peak working set
  26,779,648 -> 26,243,072 bytes. All within predeclared observed variability;
  this is not a claim that every sample improves or all environments were tested.
- Raw samples/limits: shared batch ui-pair-cd806708a25b4ae0838ec7bf679bac3d.
- PASS actual clean_mfc.bat via shared preservation verifier: 37 compiler files
  removed, 1 protected actual file SHA unchanged; 17 nested fixture files/source,
  help and idempotence pass. Unchanged junction coverage is reused from below.
- PASS exact build-close loop fixture: two hash-matched test-owned EXEs with the
  same filename in different directories; matching path closed normally, foreign
  copy remained running and was then closed by fixture cleanup. No user process
  was operated. Evaluated x64 define is present and Win32 define absent.
- NOT RUN: visual/pixel/DPI and clean-PC testing, Win32 builds, broad unrelated
  compiler-format acceptance. No hardware or personal INI was used. Existing
  RL78/RH850/GHS parser limitations remain; this does not expand their support.
- Publication follows reviewed explicit paths and the verified existing origin;
  final commit/remote SHA is recorded in shared EXE_SLIMMING_PROGRESS.md.

## Historical size pilot checkpoint 2026-10-24 — before adoption

- Preserved original formal EXE (3,286,528 bytes) in the shared isolated batch
  evidence directory. No personal INI was copied to test fixtures.
- Comparable rebuilt Release x64 baseline: 3,286,528 bytes, SHA-256
  `17AC64981BE25DD287E7930B8E789BE1E1AA7FBC9A45170CEA0BE4A555AAE477`.
- Candidate with `_AFX_NO_MFC_CONTROLS_IN_DIALOGS`: 1,197,568 bytes, SHA-256
  `368E71BC09975BB5508647F34AE33790CD3F0850AB64CDC464BA49E51AC5A3C3`.
  No functional source, optimization/runtime/architecture/security change;
  /O2 /GL /MT /GS and PE 0x8160 retained, VisualManager RTTI absent. Required
  delivery has no added DLL. The bytes alone are not an adoption gate.
- PASS: isolated Release baseline/candidate and candidate Debug x64 compilation
  using MSBuild, /p:PreBuildEventUseInBuild=false and separate OutDir/IntDir;
  no reported warnings/errors. Production EXE and version inputs unchanged.
- PASS: separate tests/SizeRegression.vcxproj + SizeRegression.exe with a new
  test-owned directory: GCC/Keil size/report totals, RAM-limit overflow warning,
  malformed/missing input. This is a console core regression, not a GUI test.
- Build preflight now closes only the exact canonical executable path, normal
  close first and bounded/revalidated PID fallback; same-named copies excluded.
  Runtime closure fixture has NOT RUN yet.
- NOT RUN: actual formal/candidate desktop controls/file-picker/Debug startup,
  paired startup/UI/CPU/memory measurements, canonical build, actual clean and
  publication. Computer Use native pipe failed retries/reset; alternative
  isolated test-script authorization requested. Do not promote this candidate
  or report no performance regression until those affected gates pass.
- Evidence: workspace #READ_ME_FIRST#/EXE_SLIMMING_PROGRESS.md and its recorded
  isolated 20261024 batch directory. No native commit/push performed yet.

Simple Stack Heap Calculator Self-Check

Use PASS / FAIL for every item.
If an item fails, record the reason and the files that still need changes.

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

==================================================
Section 1 - Build / Output Baseline
==================================================

[BLD-01] Release build
- Run `scripts\build.ps1 -Configuration Release -Platform x64`
- Verify the build succeeds.

Result: PASS / FAIL

[BLD-02] EXE output
- Verify `build\SimpleStackHeapCalculator.exe` is generated.
- Verify `build\SimpleStackHeapCalculator.ini` is created automatically after first launch when missing.

Result: PASS / FAIL

[BLD-03] Build script behavior
- Verify `scripts\build.ps1` closes a running `SimpleStackHeapCalculator.exe` before build.
- Verify it launches the EXE unless `-NoRun` is used.
- Verify `scripts\build_release.bat` closes the running EXE before build and relaunches after success.

Result: PASS / FAIL

[BLD-04] Clean script behavior
- Run `scripts\clean_build.bat`.
- Verify build artifacts are removed.
- Verify EXE, portable INI, required DLLs, data, logs, evidence, and unknown delivery files remain unchanged; only allowlisted compiler intermediates are removed.

Result: PASS / FAIL

==================================================
Section 2 - Runtime / Deployment Baseline
==================================================

[RUN-01] Native runtime expectation
- Verify the tool starts on the target Windows machine without Python runtime.

Result: PASS / FAIL

[RUN-02] Startup responsiveness
- Verify the main window opens normally.
- Verify no heavy startup task blocks the first visible window longer than necessary.

Result: PASS / FAIL

==================================================
Section 3 - UI / Packaging Baseline
==================================================

[UI-01] App icon consistency
- Verify File Explorer icon matches the running tool icon.
- Verify title bar icon matches the EXE icon.
- Verify taskbar icon matches the EXE icon.

Result: PASS / FAIL

[UI-02] Main layout sanity
- Verify the main window shows:
  - map path edit
  - browse button
  - detected format summary
  - manual parser checkboxes
  - RAM limit input
  - parse button
  - log area
  - analysis area
  - bottom stack/heap recommendation table
- Verify labels do not overlap or clip in the default window size.

Result: PASS / FAIL

==================================================
Section 4 - Config / Portability Baseline
==================================================

[CFG-01] Portable config location
- Verify settings are stored in `build\SimpleStackHeapCalculator.ini`.

Result: PASS / FAIL

[CFG-02] Default config creation
- Delete `build\SimpleStackHeapCalculator.ini`.
- Launch the tool once.
- Verify a new default INI is created automatically.

Result: PASS / FAIL

[CFG-03] Persisted fields
- Change map path and RAM limit text.
- Close and reopen the tool.
- Verify those values are restored.

Result: PASS / FAIL

[CFG-04] Manual parser default
- Launch the tool.
- Verify manual parser defaults to `KEIL`.

Result: PASS / FAIL

==================================================
Section 5 - Parser / Analysis Baseline
==================================================

[PARSE-01] Detection flow
- Load a map file.
- Verify the tool detects format from content and does not rely only on file extension.

Result: PASS / FAIL

[PARSE-02] Manual fallback flow
- Use a map with low-confidence or unsupported detection.
- Verify manual parser selection remains available as fallback.

Result: PASS / FAIL

[PARSE-03] Detection highlight
- Load a map whose detected compiler differs from the manual parser.
- Verify the detected compiler checkbox is highlighted with `[Auto]`.
- Switch manual parser to match the detected compiler.
- Verify the highlight is removed.

Result: PASS / FAIL

[PARSE-04] Report output
- Verify analysis report includes:
  - `Code`
  - `RO-data`
  - `RW-data`
  - `ZI-data`
  - `Total Flash Estimate`
  - `Total RAM Static`
  - `Stack Reserved`
  - `Heap Reserved`
  - `Estimated RAM Required`
  - `RAM Remaining`

Result: PASS / FAIL

==================================================
Section 6 - Documentation Sync
==================================================

[DOC-01] Required docs exist
- Verify these files exist:
  - `AGENTS.md`
  - `HANDOFF.md`
  - `README.md`
  - `SELF_CHECK.md`

Result: PASS / FAIL

[DOC-02] Behavior sync
- If parser coverage, build flow, or UI behavior changed, verify the docs were updated in the same change.

Result: PASS / FAIL

==================================================
Final Summary
==================================================

- PASS count:
- FAIL count:

If any item fails:
- list failed items
- explain why
- state what remains to be fixed
