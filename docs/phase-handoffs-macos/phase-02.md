# Phase 02 Handoff (macOS MCH Enablement)

## Completed Items
- 2.1 Audited plugin source for Windows-only assumptions in IR/state/UI-related paths.
- 2.2 Applied cross-platform-safe path restore handling and resolved a JUCE API portability issue.
- 2.5 Preserved Windows stereo and MCH behavior after changes.
- 2.6 Added platform-sensitive unit test coverage for restore-path handling.

## What Changed
- Source path/state restore hardening:
  - Added normalization for persisted paths (trim whitespace, strip wrapping quotes, expand `~/`, resolve relative paths from current working directory).
  - Applied file-only checks (`existsAsFile`) during restore to avoid directory false-positives.
  - Normalized restored stereo IR path persisted in APVTS state.
- Compiler portability fix:
  - Replaced instance-style absolute-path check with JUCE-compatible static call (`juce::File::isAbsolutePath(path)`).
- Test updates:
  - Added MCH slot restore test that validates quoted path + spaces behavior.
  - Made IRLoader missing-file test path platform-neutral.

## Files Modified
- `Source/PluginProcessor.cpp`
- `Source/MchSlotIrManager.cpp`
- `Tests/MchSlotIrManagerTests.h`
- `Tests/IRLoaderTests.h`
- `macos_mch_tasks.md`
- `docs/phase-handoffs-macos/phase-02.md`

## Verification Performed
- Build regression (Windows):
  - `cmake --build build --config Release`
  - Result: Passed.
- Unit tests:
  - `ctest --test-dir build -C Release --output-on-failure`
  - Result: `100% tests passed, 0 tests failed`.
- Plugin validation:
  - `powershell -NoProfile -ExecutionPolicy Bypass -File tools\pluginval\run_strict.ps1`
  - Result: `SUCCESS` (strictness level 10).

## Known Issues / Deferred Items
- 2.2 remains `NEEDS WORK` until first native Apple Clang configure/build is completed.
- 2.3 remains `NEEDS WORK` until state restore and IR load behavior are validated directly on macOS hosts.
- 2.4 remains `NEEDS WORK` until file chooser, layout, and meter behavior are verified on macOS/Retina.
- CMake Tools extension build/test commands in this Windows workspace returned "Unable to configure the project"; validation was completed via the existing workspace build/test commands instead.

## Next-Phase Prerequisites
- Run first native macOS configure/build with Xcode generator and capture Apple Clang diagnostics.
- Validate VST3 (and AU if needed) load/scan in macOS host environment.
- Execute macOS-specific state restore/UI checks (paths with spaces, moved/missing IR files, Retina layout behavior).

## Addendum (2026-05-22)
- Phase 2 is now closed as complete in the tracker after native macOS CI reached green baseline (`26276703594`, commit `ec158a5`).
- Remaining host-runtime checks originally tracked under 2.3/2.4 are intentionally deferred to Phase 4 tasks:
  - `2.3` host/runtime portion -> `4.4`
  - `2.4` host/runtime portion -> `4.5`
- Decision rationale: keep Phase 2 scoped to cross-platform source/compiler correctness and CI-verifiable behavior, with host-DAW/runtime checks consolidated in Phase 4.
