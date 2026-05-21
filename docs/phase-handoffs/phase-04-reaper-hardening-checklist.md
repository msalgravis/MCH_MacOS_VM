# Phase 4 Reaper Hardening Checklist

Purpose: complete TASKS item 4.5 with repeatable manual host validation.

Scope covered:
- buffer sizes under playback
- host sample rates: 44.1 kHz, 48 kHz, 96 kHz
- project close/reopen state reload
- rapid bypass toggling
- rapid wet/dry automation
- live IR load while transport is running

Out of scope:
- subjective tonal evaluation
- packaging and installer validation
- CPU optimization tuning

## Preconditions

1. Build latest plugin:

```powershell
cmake --build build --config Release
```

2. Confirm Reaper scan path contains built VST3 location.
3. Use the provided fixture IR file in repo root: _quad_nr3_sm.wav.
4. Start with a new Reaper project at 24-bit float mix engine defaults.
5. Create one stereo track with a stable audio source (pink noise or looped speech/music).
6. Insert BinauralSpeakerRoom on that track.

## Pass Criteria Summary

A run passes if all test cases below pass with:
- no crash
- no audio dropout/stall attributable to plugin
- no unexpected silence unless bypassed/muted by user
- diagnostics consistent with the current plugin state
- no automatic gain reduction or limiting behavior

## Test Case Matrix

### TC-01 Baseline load and no-IR safe state

Steps:
1. Open plugin UI immediately after insert.
2. Do not load an IR.
3. Play audio for at least 20 seconds.

Expected:
- status shows no-IR state.
- warning indicates dry pass-through safe state.
- audio passes through (dry behavior), no crash.

### TC-02 Valid IR load and diagnostics

Steps:
1. Click Load IR and choose _quad_nr3_sm.wav.
2. Keep transport running for 20 seconds.

Expected:
- status changes to IR loaded.
- IR metadata line shows file, channels, and length.
- host sample rate and IR sample rate fields are visible.
- no crash or UI freeze.

### TC-03 Buffer size sweep during playback

Run this sequence while plugin remains inserted and active:
- 64
- 128
- 256
- 512
- 1024

Steps per buffer size:
1. Set Reaper audio buffer size.
2. Play for 15 seconds.
3. Move Wet/Dry and Output Gain during playback.

Expected per size:
- no crash, no hard lock.
- controls remain responsive.
- meter movement remains plausible (input and output active).
- clip indicator only lights when level is hot; no auto gain cut.

### TC-04 Sample-rate sweep and mismatch visibility

Run this sequence (reopen audio device if needed):
- 44.1 kHz
- 48 kHz
- 96 kHz

Steps per sample rate:
1. Set project/device sample rate.
2. Play at least 15 seconds.
3. With IR loaded, observe diagnostics.

Expected per rate:
- plugin continues processing without crash.
- host sample-rate field reflects current host rate.
- IR sample-rate field remains tied to loaded IR.
- when rates differ, warning area indicates mismatch.

### TC-05 Rapid bypass toggling

Steps:
1. Keep transport running.
2. Toggle Bypass manually 20-30 times over ~15 seconds.

Expected:
- no crash or lockup.
- no parameter desync in UI.
- audio switches between bypassed and active behavior.

### TC-06 Rapid Wet/Dry automation

Steps:
1. Write automation for Wet/Dry with repeated fast ramps between 0% and 100%.
2. Loop 30 seconds of playback.

Expected:
- no crash.
- automation follows expected motion.
- no meter freeze.
- no hidden normalization behavior introduced.

### TC-07 Live IR load while transport running

Steps:
1. Start playback.
2. Load a valid 4-channel IR while transport is active.
3. Repeat load one more time with same file.

Expected:
- no crash.
- diagnostics update after load.
- plugin remains responsive.

### TC-08 Project save/reopen state restore

Steps:
1. With IR loaded and non-default control values, save project.
2. Close Reaper project.
3. Reopen the same project.

Expected:
- parameter values restore.
- plugin attempts to restore saved IR path.
- if file exists: IR loaded state returns.
- if file missing: warning clearly states missing saved IR file and plugin remains in safe dry behavior.

## Failure Classification

Mark each failure with one category:
- Crash: process/host crash, plugin unload, or fatal assertion.
- Audio Glitch: repeatable severe artifact or dropout attributable to plugin.
- State Regression: incorrect restore, stale status, wrong warning.
- UI Regression: frozen controls, stale meters, incorrect indicator behavior.

## Run Log Template

Copy this section for each run:

- Date:
- Reaper version:
- Audio driver/device:
- Buffer sizes tested:
- Sample rates tested:
- Fixture IR path:
- TC-01: PASS/FAIL
- TC-02: PASS/FAIL
- TC-03: PASS/FAIL (note failing size if any)
- TC-04: PASS/FAIL (note failing sample rate if any)
- TC-05: PASS/FAIL
- TC-06: PASS/FAIL
- TC-07: PASS/FAIL
- TC-08: PASS/FAIL
- Notes:
- Blocking issues:

## Completion Rule For TASKS 4.5

Set TASKS item 4.5 to COMPLETED only when:
1. All test cases pass, or
2. Remaining failures are documented as non-blocking and accepted for current milestone.
