# MCH Plugin Reaper Testing Checklist

## Pre-Test Setup
- [ ] Plugin is in `build/BinauralSpeakerRoom_artefacts/Release/VST3/Sonarworks VMPRO MCH.vst3`
- [ ] Have quad IR WAV files ready (4-channel, 48kHz recommended)
- [ ] Test audio files for 5.1, 7.1, 7.1.4 layouts available

## Phase 1: Stereo Mode Verification (Regression Check)
- [ ] Load plugin on stereo track
- [ ] "Load Stereo IR" button visible and working
- [ ] Load existing stereo IR
- [ ] Audio processes correctly
- [ ] No crashes or errors

## Phase 2: MCH Mode - UI Appearance
- [ ] Switch "Mode" dropdown to "Multichannel Renderer"
- [ ] "Load Stereo IR" button changes text
- [ ] Select "5.1" layout from "Layout" dropdown
- [ ] Pair slots appear (should show 3 rows for 5.1)
- [ ] Pair labels show: "L/R", "C/LFE", "Ls/Rs"
- [ ] Each slot shows Enable, Mute, Solo, Gain, Pan, Wet/Dry controls
- [ ] Status labels show "No IR loaded"
- [ ] Load buttons show "Load IR"

## Phase 3: MCH Mode - IR Loading per Slot
- [ ] Click "Load IR" button on Pair 1 (L/R)
- [ ] File chooser opens
- [ ] Select 4-channel quad IR WAV
- [ ] Button changes to "Replace IR"
- [ ] Status label shows "Loaded"
- [ ] Repeat for other slots
- [ ] Switching layouts hides/shows correct slots

## Phase 4: MCH Rendering Test (5.1 Layout)
- [ ] Create Reaper track with 6-channel input (5.1 surround)
- [ ] Route test audio to QUAD plugin
- [ ] Verify all 3 pair slots have quad IRs loaded
- [ ] Enable all pair slots (Enable toggle ON)
- [ ] Monitor output stereo mix
- [ ] Audio should process without clicks/pops
- [ ] Check DSP load indicator (if visible)
- [ ] No crashes or audio glitches

## Phase 5: MCH Rendering Test (7.1 Layout)
- [ ] Change layout to "7.1"
- [ ] Verify 4 pair slots now visible
- [ ] Load quad IRs for all 4 slots
- [ ] Create 8-channel test track
- [ ] Verify correct channel mapping (check copilot-instructions.md if unsure)
- [ ] Process audio
- [ ] Verify stereo output
- [ ] No artifacts or crashes

## Phase 6: MCH Rendering Test (7.1.4 Layout)
- [ ] Change layout to "7.1.4"
- [ ] Verify pair slots match layout
- [ ] Create 12-channel (7.1.4) test track
- [ ] Load quad IRs for all active slots
- [ ] Process multi-minute audio
- [ ] Monitor for stability
- [ ] Check CPU usage is reasonable

## Phase 7: Edge Cases & Stability
- [ ] Switch layouts while audio is playing
- [ ] Toggle Enable/Mute/Solo on slots
- [ ] Change Gain, Pan, Wet/Dry during playback
- [ ] Replace IR on active slot
- [ ] Save and reload Reaper session
- [ ] Close and reopen plugin window
- [ ] Test with no IRs loaded (dry only)

## Issues to Document
- [ ] Any crashes or hangs (include repro steps)
- [ ] Unexpected parameter values
- [ ] UI elements not updating correctly
- [ ] Audio glitches or pops
- [ ] Performance issues
- [ ] Channel routing incorrect (how does it sound wrong?)

## Success Criteria
- ✅ Stereo mode still works perfectly
- ✅ MCH mode UI appears/updates correctly
- ✅ Each quad IR file loads per slot
- ✅ Audio processes without crashes
- ✅ Stereo output sounds correct
- ✅ Multiple layouts work (5.1, 7.1, 7.1.4)
- ✅ Can save/reload session with multiple IRs

---

**Note**: If you encounter crashes, note:
1. What layout/slot count you were using
2. What action caused the crash (loading IR, switching layout, etc.)
3. Any audio processing at the time
4. Reaper console output if available

Use this info to debug if issues arise.
