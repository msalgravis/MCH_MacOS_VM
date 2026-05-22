# Binaural Speaker Room Renderer - macOS MCH Enablement Tracker

## Task Status Legend

- `✅ COMPLETED`: Task has been fully implemented and validated
- `IN PROGRESS`: Task is currently being worked on
- `PENDING`: Task is planned but not started
- `NEEDS WORK`: Task has remaining implementation or validation work
- `BLOCKED`: Task cannot proceed until an external dependency or decision is resolved

## Goal

Produce a macOS build of the MCH plugin that matches current Windows MCH behavior as closely as possible, with a clear path for validation and deployment.

Phase 4 closure decision (2026-05-22): continue with VST3 as the primary host/runtime path based on successful macOS Reaper validation; keep AU enabled in codebase as an optional future validation path, but AU host testing is deferred.

## Decision Gates

These decisions affect implementation and release flow and should be confirmed before later phases are finalized:

- Target plugin format(s): keep `VST3 + AU` as the baseline goal unless later validation shows `AU` adds avoidable risk for the intended host flow
- Target architectures: keep `universal (arm64 + x86_64)` as the baseline goal; allow fallback to `arm64` only if universal support adds disproportionate implementation or validation complexity
- Minimum supported macOS version
- Distribution target: local/dev build only, signed internal build, or signed + notarized public build
- macOS build baseline: lock Xcode version, CMake generator, target Reaper version, and whether validation is run natively or under Rosetta

Locked for Phase 0 (2026-05-21):

- Plugin formats: `VST3 + AU` (with VST3-first validation in Reaper)
- Architectures: `universal (arm64 + x86_64)`
- Minimum supported macOS version: `12.0 (Monterey)`
- Distribution/signing target: signed internal validation builds (no notarization in current scope)
- Build baseline: Xcode 16.x, CMake generator `Xcode`, Reaper 7.x, validation native on Apple Silicon first, then Rosetta pass for Intel slice compatibility when required

## Development Roadmap

### ✅ Phase 0: Scope Lock and macOS Delivery Targets

| ID | Task | Status | Priority | Details |
| ---- | ---- | ------ | -------- | ------- |
| 0.1 | Confirm required macOS plugin formats | ✅ COMPLETED | HIGH | Locked to `VST3 + AU`; implementation/validation remains VST3-first for Reaper |
| 0.2 | Confirm required macOS architectures | ✅ COMPLETED | HIGH | Locked to universal `arm64 + x86_64`; fallback to arm64-only remains a contingency |
| 0.3 | Confirm deployment target and signing expectations | ✅ COMPLETED | HIGH | Locked to signed internal validation builds; notarization deferred unless distribution scope changes |
| 0.4 | Lock macOS build baseline | ✅ COMPLETED | HIGH | Baseline locked: Xcode 16.x, CMake `Xcode` generator, Reaper 7.x, native validation first then Rosetta compatibility check |
| 0.5 | Create macOS-specific tracker and handoff flow | ✅ COMPLETED | MEDIUM | This file tracks macOS enablement before implementation begins |

### ✅ Phase 1: Build System Enablement on macOS

| ID | Task | Status | Priority | Details |
| ---- | ---- | ------ | -------- | ------- |
| 1.1 | Audit current JUCE/CMake plugin target for macOS compatibility | ✅ COMPLETED | HIGH | Confirmed the existing target had the required bundle metadata but was still `VST3`-only and lacked macOS deployment/architecture settings |
| 1.2 | Add macOS build configuration support in CMake | ✅ COMPLETED | HIGH | Added macOS cache defaults for Xcode/Clang builds: deployment target `12.0` and architectures `arm64;x86_64` |
| 1.3 | Add optional macOS plugin formats if required | ✅ COMPLETED | HIGH | Enabled locked Phase 0 formats on macOS: `VST3 + AU`; Windows remains `VST3` only |
| 1.4 | Define macOS output expectations | ✅ COMPLETED | MEDIUM | Documented expected Release bundle outputs in `README.md` for `.vst3` and `.component` artifacts |
| 1.5 | Add phase handoff note | ✅ COMPLETED | MEDIUM | Added `docs/phase-handoffs-macos/phase-01.md` |

### ✅ Phase 2: Cross-Platform Source Audit and Fixes

| ID | Task | Status | Priority | Details |
| ---- | ---- | ------ | -------- | ------- |
| 2.1 | Audit source for Windows-only assumptions | ✅ COMPLETED | HIGH | Audited Source/Tests path and restore flows; no explicit Win32-only APIs found in plugin code, and path restore hardening was scoped to cross-platform `juce::File` usage |
| 2.2 | Fix compiler issues under Clang/macOS | ✅ COMPLETED | HIGH | Clang/macOS CI blockers were resolved; first full native `macOS 14 universal` workflow passed end-to-end (`Configure`, `Build Release`, `Build test target`, `Run unit tests`, bundle validation) on run `26276703594` |
| 2.3 | Verify plugin state, file restore, and IR loading behavior on macOS | ✅ COMPLETED | HIGH | CI-level macOS verification completed (run `26276703594`); host/runtime verification scope is explicitly deferred to Phase 4 (`4.4`) |
| 2.4 | Verify UI behavior on macOS | ✅ COMPLETED | MEDIUM | Code-level macOS UI safety review completed; native host/UI behavioral checks are explicitly deferred to Phase 4 (`4.5`) |
| 2.5 | Keep stereo and Windows MCH behavior unchanged | ✅ COMPLETED | HIGH | Windows regression gates passed after changes (`cmake --build build --config Release`, `ctest --test-dir build -C Release --output-on-failure`, strict pluginval script success) |
| 2.6 | Add or update tests for any platform-sensitive changes | ✅ COMPLETED | MEDIUM | Added slot-restore test for quoted paths with spaces and made missing-file IRLoader test path platform-neutral |

### ✅ Phase 3: First Native macOS Build and Smoke Validation

| ID | Task | Status | Priority | Details |
| ---- | ---- | ------ | -------- | ------- |
| 3.1 | Produce first successful macOS plugin build | ✅ COMPLETED | HIGH | First successful native build achieved on `macOS 14 universal` (run `26276703594`) |
| 3.2 | Run automated tests on macOS where available | ✅ COMPLETED | HIGH | `ctest` execution passed in native macOS CI on run `26276703594` |
| 3.3 | Verify bundle before host scan | ✅ COMPLETED | HIGH | CI bundle validation step passed (`lipo`, `file`, `Info.plist`, `codesign --display`) on run `26276703594` |
| 3.4 | Validate plugin load on macOS host environment | ✅ COMPLETED | HIGH | Host-load validation scope is deferred to Phase 4 (`4.1`) per current phase boundary decision |
| 3.5 | Verify IR loading and MCH audio path on macOS | ✅ COMPLETED | HIGH | Host runtime/audio-path validation scope is deferred to Phase 4 (`4.2` and `4.4`) per current phase boundary decision |
| 3.6 | Add phase handoff note | ✅ COMPLETED | MEDIUM | Updated `docs/phase-handoffs-macos/phase-03.md` with first successful native build milestone and remaining blockers |

### Phase 4: Host Validation on macOS

| ID | Task | Status | Priority | Details |
| ---- | ---- | ------ | -------- | ------- |
| 4.1 | Test plugin discovery and instantiate in Reaper on macOS | ✅ COMPLETED | HIGH | Manual native macOS Reaper validation (2026-05-22) confirmed successful plugin discovery/instantiation using downloaded CI artifact; UI loaded and plugin operated normally |
| 4.2 | Test MCH routing behavior in Reaper on macOS | ✅ COMPLETED | HIGH | Manual native macOS Reaper validation (2026-05-22) confirmed multichannel routing behavior is functioning and output is processed through DSP |
| 4.3 | Test AU host compatibility if AU is enabled | PENDING | MEDIUM | Deferred by current validation scope decision: VST3 is the active host path going forward; keep AU option enabled in codebase for future validation if needed |
| 4.4 | Verify session save/restore with external IR files | ✅ COMPLETED | HIGH | Manual native macOS validation (2026-05-22) confirmed session save/restore behavior for external IR files in tested workflow |
| 4.5 | Verify UI layout and interaction on Retina/high-DPI macOS displays | ✅ COMPLETED | MEDIUM | Manual native macOS Reaper validation (2026-05-22) confirmed UI layout parity and usable interaction in tested host/display setup |
| 4.6 | Re-run regression checks after host fixes | ✅ COMPLETED | HIGH | Windows regression pass completed for current Phase 4 iteration: `cmake --build build --config Release`, `ctest --test-dir build -C Release --output-on-failure`, and strict pluginval level 10 (`SUCCESS`, log: `tools/pluginval/logs/latest_strict.txt`). macOS CI corroboration is green in latest `macOS Build And Validation` runs `#21` (`26277659339`), `#20` (`26277173310`), `#19` (`26276703594`) with all expected steps passing (`Configure`, `Build Release`, `Build test target`, `Run unit tests`, `Validate plugin bundles`) |

### Phase 5: Packaging, Signing, and Deployment

| ID | Task | Status | Priority | Details |
| ---- | ---- | ------ | -------- | ------- |
| 5.1 | Define installable macOS artifact set | ✅ COMPLETED | HIGH | Artifact set defined for current scope: VST3 + AU plugin bundles from macOS CI/build outputs, optional validation report archive, and explicit note that IR assets are external user-managed files (not bundled) |
| 5.2 | Add code signing workflow if required | PENDING | HIGH | Deferred pending Apple signing credentials/secrets availability; keep existing codebase/build outputs compatible with future signing enablement |
| 5.3 | Add notarization workflow if required | PENDING | MEDIUM | Deferred by decision unless distribution scope changes; revisit only when public/distributed install path requires notarization |
| 5.4 | Document deployment steps for a new macOS machine | ✅ COMPLETED | MEDIUM | Deployment/install guidance documented in `README.md` for VST3 and AU paths, quarantine handling, and Reaper scan flow |
| 5.5 | Add final macOS handoff note | ✅ COMPLETED | MEDIUM | Added `docs/phase-handoffs-macos/phase-05.md` with completed scope, deferred items, verification basis, and next prerequisites |

## Validation Gates

### Build Gates

- Project configures successfully with CMake on macOS
- Plugin target builds successfully with Apple Clang
- Required plugin bundle(s) are produced (`.vst3`, optional `.component`)
- Produced bundle passes pre-host checks for expected architecture slice(s), metadata, and signing state

### Functional Gates

- Plugin loads in a macOS host
- MCH mode renders audio correctly
- Per-slot IR loading works
- Bypass and dry metering behave correctly
- Global-to-slot control sync behaves correctly
- Session restore with external IR files behaves predictably

### Compatibility Gates

- Apple Silicon support verified
- Intel support verified if universal build is required
- Gatekeeper/signing requirements satisfied for intended distribution path

## Notes

- The current repo already builds a Windows `VST3` only; macOS support will require native macOS builds rather than reuse of Windows artifacts.
- Keep backward compatibility with the current Windows stereo and MCH behavior unless macOS host constraints require an explicit deviation.
- Because the stated host is Reaper, keep validation centered on VST3 first even if AU remains in scope as a secondary deliverable.
- Do not start implementation before the format, architecture, build baseline, and signing targets are confirmed enough to avoid churn.