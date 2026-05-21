# Binaural Speaker Room Renderer - Multichannel Extension Task Tracker

## Task Status Legend

- ✅ COMPLETED: Task has been fully implemented and validated
- IN PROGRESS: Task is currently being worked on
- PENDING: Task is planned but not started
- NEEDS WORK: Task has remaining implementation or validation work

## Development Roadmap

### ✅ Phase 0: Extension Scope Lock and Planning

| ID   | Task | Status | Priority | Details |
| ---- | ---- | ------ | -------- | ------- |
| 0.1 | Confirm stereo baseline is completed and stable | ✅ COMPLETED | HIGH | Original stereo roadmap in `tasks.md` is fully completed |
| 0.2 | Confirm extension source of truth | ✅ COMPLETED | HIGH | `prd_multichannel_binaural_renderer_extension.md` is canonical for MCH work |
| 0.3 | Update Copilot workspace instructions for extension | ✅ COMPLETED | HIGH | `.github/copilot-instructions.md` updated for dual-tracker and MCH workflow |
| 0.4 | Create extension phase tracker | ✅ COMPLETED | HIGH | This file (`tasks_mch.md`) created from the existing `tasks.md` structure |
| 0.5 | Create MCH handoff folder and template | ✅ COMPLETED | MEDIUM | Added `docs/phase-handoffs-mch/_template.md` |
| 0.6 | Create initial MCH handoff note | ✅ COMPLETED | MEDIUM | Added `docs/phase-handoffs-mch/phase-00.md` |

### ✅ Phase 1: MCH UI and Layout State (No New DSP)

| ID   | Task | Status | Priority | Details |
| ---- | ---- | ------ | -------- | ------- |
| 1.1 | Add mode selector with Stereo and MCH modes | ✅ COMPLETED | HIGH | UI component and APVTS binding present; isMchModeSelected() implemented |
| 1.2 | Add layout preset model and selector | ✅ COMPLETED | HIGH | All 7 presets defined; layoutSelector APVTS binding present; getSelectedLayoutIndex() implemented |
| 1.3 | Add channel map display component | ✅ COMPLETED | HIGH | Channel order shown via LayoutPreset::channelOrder; updated during layout change |
| 1.4 | Add dynamic visible pair-slot UI | ✅ COMPLETED | HIGH | Pair slots shown/hidden based on layout; updateMchUi() implements visibility logic |
| 1.5 | Add per-pair APVTS parameters for 8 stable slots | ✅ COMPLETED | HIGH | All 64 parameters created (8 slots × 8 params each); APVTS attachments working |
| 1.6 | Add placeholder per-slot Load/Replace IR controls | ✅ COMPLETED | MEDIUM | UI buttons created; showSlotLoadDialog() and getSlotLoadButtonText() implemented |
| 1.7 | Add active pairs and path count display | ✅ COMPLETED | MEDIUM | DSP statistics available via getMchActivePairCount() etc; timer updates UI |
| 1.8 | Add phase handoff note | ✅ COMPLETED | HIGH | See `docs/phase-handoffs-mch/phase-01.md` |
| 1.9 | Implement MCH UI wiring in PluginEditor | ✅ COMPLETED | HIGH | All 7 UI methods implemented: initialisePairSlots, updateMchUi, isMchModeSelected, getSelectedLayoutIndex, showSlotLoadDialog, getSlotStatusText, getSlotLoadButtonText. Pluginval strict passed (2026-05-19) |

### ✅ Phase 2: Multichannel Bus Layout and Direct/Dry Rendering

| ID   | Task | Status | Priority | Details |
| ---- | ---- | ------ | -------- | ------- |
| 2.1 | Add multichannel input bus support up to 16ch | ✅ COMPLETED | HIGH | Accept preset-aligned channel counts |
| 2.2 | Keep output fixed stereo in all modes | ✅ COMPLETED | HIGH | Validate stereo output contract remains unchanged |
| 2.3 | Implement dry routing for normal A/B pairs | ✅ COMPLETED | HIGH | Dry A -> out L, Dry B -> out R |
| 2.4 | Implement C/LFE dry special-case | ✅ COMPLETED | HIGH | C and LFE route equally to L/R |
| 2.5 | Add global and pair wet/dry structure | ✅ COMPLETED | MEDIUM | Effective wet model staged pre-convolution |
| 2.6 | Add tests for layout mapping and dry routing | ✅ COMPLETED | HIGH | Include C/LFE behavior tests |
| 2.7 | Add phase handoff note | ✅ COMPLETED | HIGH | Write `docs/phase-handoffs-mch/phase-02.md` |

### ✅ Phase 3: Multiple Quad IR Loading Per Pair

| ID   | Task | Status | Priority | Details |
| ---- | ---- | ------ | -------- | ------- |
| 3.1 | Implement per-slot Load/Replace IR workflow | ✅ COMPLETED | HIGH | One quad WAV per visible pair slot |
| 3.2 | Validate exactly 4 channels per slot IR | ✅ COMPLETED | HIGH | Reject non-quad WAV and show slot-level errors |
| 3.3 | Persist per-slot IR paths and status in state | ✅ COMPLETED | HIGH | `pair01..pair08` state keys and restore behavior |
| 3.4 | Handle missing files and missing slot behavior | ✅ COMPLETED | HIGH | Missing active slots contribute no wet signal and warn |
| 3.5 | Support different slot IR lengths/sample rates | ✅ COMPLETED | MEDIUM | Resample IRs to host sample rate off audio thread |
| 3.6 | Add per-slot IR loading/validation tests | ✅ COMPLETED | HIGH | Valid quad, invalid channel count, missing file, restore |
| 3.7 | Add phase handoff note | ✅ COMPLETED | HIGH | Write `docs/phase-handoffs-mch/phase-03.md` |

### ✅ Phase 4: MCH Binaural Convolution DSP

| ID   | Task | Status | Priority | Details |
| ---- | ---- | ------ | -------- | ------- |
| 4.1 | Implement reusable `SpeakerPairProcessor` | ✅ COMPLETED | HIGH | Four mono paths per pair with quad routing convention |
| 4.2 | Implement `MchBinauralRenderer` orchestration | ✅ COMPLETED | HIGH | Up to 8 active pairs summed to stereo |
| 4.3 | Implement pre-convolution pair pan behavior | ✅ COMPLETED | HIGH | Pan acts on pair inputs before convolution |
| 4.4 | Implement pair wet/dry x global wet model | ✅ COMPLETED | HIGH | effectivePairWet = pairWet * globalWet |
| 4.5 | Implement enable/mute/solo processing logic | ✅ COMPLETED | HIGH | Skip non-audible pairs where practical |
| 4.6 | Preserve gain relationships without normalization | ✅ COMPLETED | HIGH | No per-channel or inter-slot normalization |
| 4.7 | Add routing and behavior DSP tests | ✅ COMPLETED | HIGH | LL/LR/RL/RR, pan A/B, solo/mute, multi-pair sum |
| 4.8 | Add Reaper manual tests for 5.1, 7.1.4, 9.1.6 | IN PROGRESS | HIGH | UI, slot metering, bypass dry metering, and global-to-slot control sync are working and validated (build ✅, pluginval strict ✅). Fixture quad file tested loading into multiple slots. Ready for manual Reaper testing. |
| 4.9 | Add phase handoff note | ✅ COMPLETED | HIGH | Write `docs/phase-handoffs-mch/phase-04.md` |

### ✅ Phase 5: Performance Optimization Pass

| ID   | Task | Status | Priority | Details |
| ---- | ---- | ------ | -------- | ------- |
| 5.1 | Add skip logic for inactive/inaudible pairs | ✅ COMPLETED | HIGH | Disabled, muted, solo-filtered, and no-input pairs avoid work |
| 5.2 | Add skip logic for hard pan A/B cases | ✅ COMPLETED | HIGH | Hard pan now skips opposite-side wet convolution paths |
| 5.3 | Add skip logic for effective wet 0 percent | ✅ COMPLETED | HIGH | Wet paths are bypassed when effective wet is zero |
| 5.4 | Preallocate and reuse temporary buffers | ✅ COMPLETED | HIGH | Pair processor uses preallocated reusable scratch buffers |
| 5.5 | Add active DSP load indicators | ✅ COMPLETED | MEDIUM | UI now shows active pairs/paths plus wet/dry/skipped pair stats |
| 5.6 | Investigate optimization points for code, its running, resource requirements and DSP processes| ✅ COMPLETED | HIGH | Implemented block-level fast paths and reduced redundant buffer work |
| 5.7 | Run performance matrix tests | ✅ COMPLETED | HIGH | Added automated matrix scheduling tests at 44.1/48/96 and 128/256/512 |
| 5.8 | Add phase handoff note | ✅ COMPLETED | HIGH | Wrote `docs/phase-handoffs-mch/phase-05.md` |

### ✅ Phase 6: Runtime Safety, Optimization, and Redundancy Cleanup

| ID   | Task | Status | Priority | Details |
| ---- | ---- | ------ | -------- | ------- |
| 6.1 | Fix per-slot IR hot-reload detection | ✅ COMPLETED | HIGH | Replaced pointer-identity checks with versioned slot snapshots |
| 6.2 | Make slot IR manager safe for concurrent UI/audio access | ✅ COMPLETED | HIGH | Added immutable per-slot processed IR snapshots with versioned handoff |
| 6.3 | Move all IR convolver reconfiguration off audio thread | ✅ COMPLETED | HIGH | Applied slot IR updates from non-audio paths; removed `processBlock` slot reload sync |
| 6.4 | Enforce no heap allocations in `processBlock` during IR updates | ✅ COMPLETED | HIGH | Slot IR updates now occur outside `processBlock`; active convolver state swapped lock-free |
| 6.5 | Remove redundant clears/copies in stereo convolver hot path | ✅ COMPLETED | MEDIUM | Reduced duplicate wet/path scratch clear/copy operations |
| 6.6 | Cache high-frequency APVTS parameter pointers for MCH pairs | ✅ COMPLETED | MEDIUM | Added cached MCH pair parameter pointers and used them in MCH render loop |
| 6.7 | Reduce non-essential UI refresh work in timer callback | ✅ COMPLETED | LOW | Throttled expensive UI status/layout refreshes while preserving meter cadence |
| 6.8 | Add thread-safety and hot-reload regression tests | ✅ COMPLETED | HIGH | Added slot snapshot/version stability and renderer hot-reload regression tests |
| 6.9 | Run stress/performance validation matrix after fixes | ✅ COMPLETED | HIGH | Re-ran automated matrix scheduling tests at 44.1/48/96 and 128/256/512 |
| 6.10 | Add phase handoff note | ✅ COMPLETED | HIGH | Wrote `docs/phase-handoffs-mch/phase-06.md` |

## Testing Gates

### Unit Tests (Required)

- Layout preset maps and visible slot generation
- Channel index mapping and C/LFE dry special-case
- Pair pan gain behavior and effective wet computation
- Solo/mute/enable audible pair logic
- Per-slot IR validation and state save/load

### DSP Tests (Required)

- Quad routing correctness for every pair processor
- Hard pan A/B pre-convolution behavior
- Global wet and pair wet interaction
- Multi-pair summing correctness
- Different IR lengths and missing-slot behavior

### Integration Tests in Reaper (Required)

- Stereo baseline still works
- 5.1 input track path
- 7.1.4 input track path
- 9.1.6 (16-channel) input track path
- Fixed stereo output confirmation
- Session save/restore with multiple slot IRs

### Performance Tests (Required)

- CPU usage across active pair counts: 1, 3, 6, 8
- CPU usage across sample rates: 44.1, 48, 96 kHz (if feasible)
- CPU usage across buffers: 128, 256, 512 samples
- Confirm no `processBlock` heap allocation/file I/O/long locks

## Multi-Chat Execution Rules

- At phase start: read `tasks_mch.md` and previous MCH handoff note before coding.
- Before implementation in each chat: summarize completed, in-scope, and out-of-scope items.
- At phase end: update `tasks_mch.md` and create/update the matching handoff note.
- Mark completed tasks as `✅ COMPLETED` and completed phase titles with `✅`.
- If tracker and code disagree, treat code + latest handoff as source of truth and update this tracker.

## Notes

- Baseline stereo tracker remains in `tasks.md`; do not overwrite its completed history.
- Source of truth for extension behavior: `prd_multichannel_binaural_renderer_extension.md`.
- Preserve stereo behavior while incrementally adding MCH functionality.
- Keep changes modular and phase-focused to reduce overlap and regression risk.
- Latest iteration handoff: `docs/phase-handoffs-mch/phase-bugfix-2026-05-19.md`.
- **Current status**: MCH UI, slot metering, bypass dry metering, and global control override/sync are implemented and validated (2026-05-20).
- Build: ✅ Release succeeded
- Pluginval Strict (Level 10): ✅ All 40+ test sections passed
- Plugin ready for Phase 4.8 manual Reaper testing with 5.1/7.1.4/9.1.6 layouts and `_quad_nr3_sm.wav` fixture.
