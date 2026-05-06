# SELF_CHECK.md

Simple Stack Heap Calculator Self-Check

Use PASS / FAIL for every item.
If an item fails, record the reason and the files that still need changes.

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
- Verify only `build\SimpleStackHeapCalculator.exe` is kept when it existed before clean.

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
