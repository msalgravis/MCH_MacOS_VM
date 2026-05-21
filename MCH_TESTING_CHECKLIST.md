# MCH Plugin Manual Testing Checklist

**Plugin Version**: Built May 15, 2026  
**Reaper Version**: `[INSERT]`  
**Test Date**: `[INSERT]`  
**Tester**: `[INSERT]`

## Setup
- [ ] Copied `BinauralSpeakerRoom.vst3` to `C:\Program Files\Common Files\VST3\`
- [ ] Reaper rescanned VST3 plugins
- [ ] Plugin appears in Reaper plugin list

---

## Stereo Mode (Baseline - must not break)
- [ ] Insert plugin on **stereo track**
- [ ] Select **Stereo** mode
- [ ] Select **Stereo** layout preset
- [ ] Load stereo quad IR (4-channel WAV)
- [ ] Verify IR loads without error
- [ ] Play audio; verify stereo output (both ears)
- [ ] Toggle Enable on pair slot
- [ ] Toggle Mute on pair slot
- [ ] Adjust Gain / Pan / Wet/Dry controls
- [ ] Adjust Global Wet/Dry
- [ ] Save and reload Reaper project → IR path restores

---

## MCH Mode: 5.1 Layout
- [ ] Create **6-channel Reaper track** (L, R, C, LFE, Ls, Rs)
- [ ] Insert plugin on track
- [ ] Select **MCH Renderer** mode
- [ ] Select **5.1** layout preset
- [ ] Verify UI displays correct 6-channel map
- [ ] Verify UI shows **3 pair slots**: L/R, C/LFE, Ls/Rs
- [ ] Load quad IR for **L/R pair** → Status shows "Loaded"
- [ ] Load quad IR for **C/LFE pair** → Status shows "Loaded"
- [ ] Load quad IR for **Ls/Rs pair** → Status shows "Loaded"
- [ ] Play audio; verify stereo output (all three pairs summed)
- [ ] Toggle each pair **Enable** on/off while playing
- [ ] Toggle each pair **Mute** while playing
- [ ] Test **Solo** on each pair individually
- [ ] Test **Pan** on L/R pair (should be pre-convolution)
- [ ] Test **Pan** on C/LFE pair
- [ ] Test **Pan** on Ls/Rs pair
- [ ] Adjust **Gain** on each pair
- [ ] Adjust **Wet/Dry** on each pair
- [ ] Adjust **Global Wet/Dry** → all pairs respond
- [ ] Save and reload Reaper project → all 3 IR paths restore
- [ ] Test loading IR while **playing** (should update atomically)

---

## MCH Mode: 7.1.4 Layout
- [ ] Create **10-channel Reaper track** (L, R, C, LFE, Ls, Rs, Tfl, Tfr, Trl, Trr)
- [ ] Insert plugin on track
- [ ] Select **MCH Renderer** mode
- [ ] Select **7.1.4** layout preset
- [ ] Verify UI displays correct 10-channel map
- [ ] Verify UI shows **5 pair slots**: L/R, C/LFE, Ls/Rs, Tfl/Tfr, Trl/Trr
- [ ] Load quad IR for all **5 pairs**
- [ ] Play audio; verify stereo output (all 5 pairs summed)
- [ ] Toggle each pair Enable/Mute/Solo while playing
- [ ] Adjust Pan on Tfl/Tfr pair
- [ ] Adjust Pan on Trl/Trr pair
- [ ] Save and reload Reaper project → all 5 IR paths restore

---

## MCH Mode: 9.1.6 Layout (Full CPU Load)
- [ ] Create **16-channel Reaper track**
- [ ] Insert plugin on track
- [ ] Select **MCH Renderer** mode
- [ ] Select **9.1.6** layout preset
- [ ] Verify UI displays correct 16-channel map
- [ ] Verify UI shows **8 pair slots**
- [ ] Load quad IR for all **8 pair slots**
- [ ] Play audio; verify stereo output (all 8 pairs summed)
- [ ] Verify **CPU meter** shows active load
- [ ] Toggle all pairs Enable/Mute/Solo in sequence
- [ ] Adjust **Global Wet/Dry** to 0% → confirm all dry pass-through
- [ ] Adjust **Global Wet/Dry** to 100% → confirm full wet
- [ ] Save and reload Reaper project → all 8 IR paths restore

---

## MCH Mode: Missing/Invalid IR Handling
- [ ] Create 6-channel track; select 5.1 layout
- [ ] Load IR on L/R and C/LFE pairs only (skip Ls/Rs)
- [ ] Play audio; verify **Ls/Rs pair shows "Missing"** warning
- [ ] Verify Ls/Rs pair contributes silence (only L/R and C/LFE audible)
- [ ] Load IR on Ls/Rs pair while playing
- [ ] Verify Ls/Rs pair now contributes to output
- [ ] Attempt to load **non-quad IR** (wrong channel count)
  - [ ] UI should reject and show "Invalid" status
  - [ ] No crash

---

## MCH Mode: Parameter Automation & Playback
- [ ] Create 9.1.6 track with all 8 pairs loaded
- [ ] Play audio
- [ ] Record **Enable** automation on L/R pair
- [ ] Record **Mute** automation on C/LFE pair
- [ ] Record **Solo** automation on Ls/Rs pair
- [ ] Record **Gain** automation on Tfl/Tfr pair
- [ ] Record **Pan** automation on Trl/Trr pair
- [ ] Record **Wet/Dry** automation on one pair
- [ ] Play back automation → all parameters respond correctly
- [ ] Save/reload Reaper project → automation intact

---

## MCH Mode: Multiple Instances
- [ ] Insert plugin on **track 1** (5.1 layout, 3 pairs loaded)
- [ ] Insert plugin on **track 2** (7.1.4 layout, 5 pairs loaded)
- [ ] Play both tracks simultaneously
- [ ] Verify both plugins render independently to stereo (no cross-talk)
- [ ] Save/reload Reaper project → both instances restore independently

---

## General Stability & Edge Cases
- [ ] Plugin does **not crash** on mode switch (Stereo ↔ MCH)
- [ ] Plugin does **not crash** on layout change
- [ ] Plugin does **not crash** on IR load/replace
- [ ] Plugin does **not crash** on parameter automation
- [ ] Plugin does **not crash** on Reaper project save/reload
- [ ] No **VRAM leaks** (monitor plugin memory over long playback)
- [ ] No **CPU spikes** on IR load
- [ ] No **audio dropouts** during IR load while playing
- [ ] UI remains **responsive** during playback

---

## Final Verification
- [ ] All required layouts load without error
- [ ] Stereo mode behavior **unchanged** from original baseline
- [ ] MCH mode delivers stereo output in all cases
- [ ] Per-pair and global controls work as specified
- [ ] State save/restore complete and accurate
- [ ] No critical bugs or crash scenarios encountered

---

**Notes/Issues Found**:
```
[Write any issues, unexpected behavior, or notes here]
```

**Overall Status**: ☐ PASS | ☐ FAIL | ☐ NEEDS FIXES

---
