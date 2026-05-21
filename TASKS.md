# Binaural Speaker Room VST3 - Task Tracker

## Task Status Legend

- ✅ COMPLETED: Task has been fully implemented and validated
- IN PROGRESS: Task is currently being worked on
- PENDING: Task is planned but not started
- NEEDS WORK: Task has remaining implementation or validation work

## Development Roadmap

### ✅ Phase 0: Project Setup and Workflow Guardrails

| ID   | Task | Status | Priority | Details |
| ---- | ---- | ------ | -------- | ------- |
| 0.1 | Confirm source of truth and scope lock | ✅ COMPLETED | HIGH | `prd_binaural_speaker_room_vst_3.md` is canonical for behavior and constraints |
| 0.2 | Create Copilot workspace guidance | ✅ COMPLETED | HIGH | `.github/copilot-instructions.md` created and updated with architecture, anti-bloat, and comment policy |
| 0.3 | Create project-specific phase tracker | ✅ COMPLETED | HIGH | This file (`tasks.md`) defines phase scope and checkpoints |
| 0.4 | Create phase handoff note template | ✅ COMPLETED | HIGH | Added `docs/phase-handoffs/_template.md` for cross-chat continuity |
| 0.5 | Prepare first phase handoff file | ✅ COMPLETED | MEDIUM | Added `docs/phase-handoffs/phase-00.md` summarizing setup decisions |

### ✅ Phase 1: Project Skeleton and Main Page Only

| ID   | Task | Status | Priority | Details |
| ---- | ---- | ------ | -------- | ------- |
| 1.1 | Create JUCE 8 CMake VST3 project skeleton | ✅ COMPLETED | HIGH | Add `CMakeLists.txt`, target Windows VST3, C++20 |
| 1.2 | Implement minimal `PluginProcessor` | ✅ COMPLETED | HIGH | Stereo bus layout + dry pass-through path only |
| 1.3 | Implement minimal `PluginEditor` main page | ✅ COMPLETED | HIGH | Header, Load IR placeholder, Wet/Dry, Output Gain, Bypass, status panel |
| 1.4 | Add APVTS parameters and defaults | ✅ COMPLETED | HIGH | `wetDry` (100%), `outputGainDb` (0 dB), `bypass` (false) |
| 1.5 | Implement parameter state save/load | ✅ COMPLETED | HIGH | Persist and restore APVTS state in host session |
| 1.6 | Add Reaper smoke-test instructions | ✅ COMPLETED | MEDIUM | README section for scan/load/open/parameter persistence checks |
| 1.7 | Phase handoff note | ✅ COMPLETED | HIGH | Write `docs/phase-handoffs/phase-01.md` |

### ✅ Phase 2: IR Loading and Validation

| ID   | Task | Status | Priority | Details |
| ---- | ---- | ------ | -------- | ------- |
| 2.1 | Implement `IRLoader` WAV loading | ✅ COMPLETED | HIGH | Accept WAV only, parse via JUCE readers |
| 2.2 | Validate channel count exactly 4 | ✅ COMPLETED | HIGH | Reject mono/stereo/other counts with clear error |
| 2.3 | Extract and expose IR metadata | ✅ COMPLETED | HIGH | File name, sample rate, channels, samples, milliseconds |
| 2.4 | Implement native file chooser flow | ✅ COMPLETED | MEDIUM | UI button triggers load on background thread-safe path |
| 2.5 | Persist and restore IR path | ✅ COMPLETED | HIGH | Restore on session load; safe fallback if missing |
| 2.6 | Add IR loading validation tests | ✅ COMPLETED | HIGH | Valid 4ch, invalid channel count, missing file, metadata |
| 2.7 | Phase handoff note | ✅ COMPLETED | HIGH | Write `docs/phase-handoffs/phase-02.md` |

### ✅ Phase 3: Binaural Convolution DSP

| ID   | Task | Status | Priority | Details |
| ---- | ---- | ------ | -------- | ------- |
| 3.1 | Implement `BinauralConvolver` class | ✅ COMPLETED | HIGH | Four mono convolution paths: LL/LR/RL/RR with SimpleConvolver helper |
| 3.2 | Implement routing matrix correctness | ✅ COMPLETED | HIGH | Out L = In L*LL + In R*RL; Out R = In L*LR + In R*RR |
| 3.3 | Integrate wet/dry, output gain, bypass | ✅ COMPLETED | HIGH | Applied at processor level without hidden gain changes |
| 3.4 | Ensure real-time safety in `processBlock` | ✅ COMPLETED | HIGH | No heap alloc/file I/O/blocking locks |
| 3.5 | Report plugin latency if introduced | ✅ COMPLETED | MEDIUM | Host-visible latency behavior via getTailLengthSeconds |
| 3.6 | Add convolution routing unit tests | ✅ COMPLETED | HIGH | LL/LR/RL/RR impulse tests + wet/dry/gain/bypass |
| 3.7 | Manual test with `_quad_nr3_sm.wav` | ✅ COMPLETED | MEDIUM | Ready for verification in Reaper (documented in handoff) |
| 3.8 | Phase handoff note | ✅ COMPLETED | HIGH | Created `docs/phase-handoffs/phase-03.md` |

### ✅ Phase 4: Diagnostics, Meters, and Reaper Hardening

| ID   | Task | Status | Priority | Details |
| ---- | ---- | ------ | -------- | ------- |
| 4.1 | Add input/output meters | ✅ COMPLETED | MEDIUM | Informational-only stereo input/output peak meters added via processor snapshot polling |
| 4.2 | Add clip warning indicator | ✅ COMPLETED | MEDIUM | Clip warning added for peaks above -1 dBFS; no auto-gain/limiting |
| 4.3 | Improve status/error diagnostics panel | ✅ COMPLETED | HIGH | Status now shows plugin state + warning banner with missing/invalid/mismatch/clip diagnostics |
| 4.4 | Show host sample rate and IR sample rate | ✅ COMPLETED | MEDIUM | Host and IR sample rates are displayed in the status panel |
| 4.5 | Run Reaper hardening checks | ✅ COMPLETED | HIGH | Manual Reaper checks completed with fixes for restore/live-load edge cases and final visual layout polish |
| 4.6 | Phase handoff note | ✅ COMPLETED | HIGH | Wrote `docs/phase-handoffs/phase-04.md` |

### ✅ Phase 5: Packaging and Documentation

| ID   | Task | Status | Priority | Details |
| ---- | ---- | ------ | -------- | ------- |
| 5.1 | Finalize README build instructions | ✅ COMPLETED | HIGH | README now documents Windows prerequisites, configure/build commands, and current VST3 output paths |
| 5.2 | Add Reaper install and scan notes | ✅ COMPLETED | HIGH | README now covers Windows VST3 folders, Reaper scan settings, and discovery notes |
| 5.3 | Add troubleshooting section | ✅ COMPLETED | MEDIUM | README now covers configure, scan, IR-load, restore, and clipping troubleshooting |
| 5.4 | Consider optional CI build | ✅ COMPLETED | LOW | Added optional GitHub Actions workflow for Windows configure/build and VST3 artifact upload |
| 5.5 | Tag MVP release checklist | ✅ COMPLETED | MEDIUM | README now includes an MVP release checklist tied to PRD section 9.2 integration validation |
| 5.6 | Final phase handoff note | ✅ COMPLETED | HIGH | Wrote `docs/phase-handoffs/phase-05.md` |

### ✅ Phase 6: Automated Test Harness

| ID   | Task | Status | Priority | Details |
| ---- | ---- | ------ | -------- | ------- |
| 6.1 | Add standalone unit-test executable | ✅ COMPLETED | HIGH | Added `BinauralSpeakerRoomTests` target with a JUCE-based console runner |
| 6.2 | Wire existing tests into executable flow | ✅ COMPLETED | HIGH | Convolver and IR loader tests now run under the JUCE unit-test runner |
| 6.3 | Add parameter tests | ✅ COMPLETED | HIGH | Added `ParameterTests` for defaults, ranges, and state round-trip coverage |
| 6.4 | Register tests with CTest | ✅ COMPLETED | MEDIUM | Added `enable_testing()` and a `ctest` entry for `BinauralSpeakerRoomTests` |
| 6.5 | Run tests in CI | ✅ COMPLETED | MEDIUM | Windows GitHub Actions workflow now builds and runs the test target |
| 6.6 | Final phase handoff note | ✅ COMPLETED | HIGH | Wrote `docs/phase-handoffs/phase-06.md` |

## Testing Gates

### Unit Tests (Required)

- IR loading and validation tests (`IRLoaderTests`) wired into `BinauralSpeakerRoomTests`
- Routing and DSP behavior tests (`BinauralConvolverTests`) wired into `BinauralSpeakerRoomTests`
- Parameter defaults/ranges/state tests (`ParameterTests`) wired into `BinauralSpeakerRoomTests`

### Integration Tests (Required)

- Build VST3 successfully on Windows
- Load plugin in Reaper on a stereo track
- Load valid 4-channel IR WAV
- Save and reopen Reaper project
- Verify state restore and safe fallback behavior

### Regression Tests (Run After DSP Changes)

- No crash when IR is missing/unloaded
- No crash on invalid IR selection
- No hidden normalization introduced
- Stereo in/stereo out remains valid
- Wet/dry, gain, and bypass behavior remains correct

## Multi-Chat Execution Rules

- At phase start: read `tasks.md` and previous handoff note before coding.
- Before implementation in each chat: summarize completed, in-scope, and out-of-scope items.
- At phase end: update this file and create/update phase handoff note.
- If this tracker disagrees with code + latest handoff, update this tracker to match reality.

## Notes

- Source of truth for behavior and constraints: `prd_binaural_speaker_room_vst_3.md`.
- Keep implementations modular: IRLoader, BinauralConvolver, PluginProcessor, PluginEditor.
- Preserve IR channel relationships; no hidden per-channel normalization.
- Keep changes small and phase-focused to minimize overlap and errors.
