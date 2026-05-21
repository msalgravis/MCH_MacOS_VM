# Phase 01 Handoff (macOS MCH Enablement)

## Completed Items
- 1.1 Audited the current JUCE/CMake plugin target for macOS compatibility.
- 1.2 Added macOS build configuration defaults in CMake.
- 1.3 Enabled the required macOS plugin formats from the locked Phase 0 decision.
- 1.4 Defined expected macOS output artifact locations.
- 1.5 Added the Phase 1 handoff note.

## What Changed
- `CMakeLists.txt`
  - Added macOS-only cache variables for deployment target and architecture selection.
  - Set default macOS deployment target to `12.0`.
  - Set default macOS architectures to `arm64;x86_64`.
  - Switched plugin formats to `VST3;AU` on macOS while keeping Windows on `VST3`.
- `README.md`
  - Added macOS configure guidance and expected artifact output paths for `VST3` and `AU` bundles.

## Files Modified
- `CMakeLists.txt`
- `README.md`
- `macos_mch_tasks.md`
- `docs/phase-handoffs-macos/phase-01.md`

## Verification Performed
- Windows regression check:
  - Command: `cmake --build build --config Release`
  - Result: Passed after one transient file-lock retry; CMake reconfigured successfully and the Release plugin/tests built.
- Windows validation check:
  - Command: `powershell -NoProfile -ExecutionPolicy Bypass -File tools\pluginval\run_strict.ps1`
  - Result: Passed; pluginval strictness level 10 completed with `SUCCESS` against the current Release VST3.
- Source-level diagnostics:
  - `CMakeLists.txt` reports no editor diagnostics after the change.

## Known Issues / Deferred Items
- No native macOS configure/build has been executed in this Windows workspace.
- AU bundle signing and notarization workflow remain out of scope for this phase.
- Cross-platform source compatibility and Apple Clang fixes remain Phase 2 work.

## Next-Phase Prerequisites
- Run the first native macOS configure with Xcode and capture any Apple Clang or JUCE/AU-specific issues.
- Audit source for Windows-only assumptions before attempting host validation.