# Phase 01 Handoff (MCH Extension)

## Completed Items
- 1.1 Added mode selector and persisted mode state (`Stereo`, `Multichannel Renderer`) in APVTS.
- 1.2 Added layout preset model and selector for `Stereo`, `5.1`, `7.1`, `7.1.2`, `7.1.4`, `9.1.4`, `9.1.6`.
- 1.3 Added channel map display that reflects the selected layout input order.
- 1.4 Added dynamic pair-slot UI that only shows slots required by the selected layout.
- 1.5 Added per-pair APVTS parameters for 8 stable slots: `Enable`, `Mute`, `Solo`, `Gain`, `Pan`, `Wet/Dry`.
- 1.6 Added per-slot placeholder `Load/Replace IR` controls and persisted placeholder slot path state (`pair01..pair08` IR path keys on APVTS state tree).
- 1.7 Added active pair/path count display (`Active pairs` and `Convolution paths`) before MCH DSP integration.
- 1.8 Updated tracker and created this handoff note.

## Files Created/Modified
- `Source/LayoutPreset.h` - new layout preset/channel map/pair-slot data model (Phase 1 state/UI model).
- `Source/Parameters.h` - added MCH parameter IDs and per-slot helper accessors.
- `Source/Parameters.cpp` - added mode/layout APVTS params and per-slot APVTS params for 8 slots.
- `Source/PluginEditor.h` - added MCH UI declarations, slot row model, attachments, and helper methods.
- `Source/PluginEditor.cpp` - implemented mode/layout controls, channel map UI, dynamic slot visibility, per-slot placeholders, and active pair/path count.
- `Tests/LayoutPresetTests.h` - new unit tests for layout preset names/counts and key 9.1.6 mappings.
- `Tests/ParameterTests.h` - extended state/default/range coverage for new Phase 1 parameters.
- `Tests/TestMain.cpp` - registered `LayoutPresetTests`.
- `tasks_mch.md` - marked Phase 1 tasks as `✅ COMPLETED` and phase title complete.

## Verification Performed
- Build:
  - Command(s): `cmake --build build --config Release --target BinauralSpeakerRoomTests`
  - Result: Succeeded (target build completed without compile errors).
- Tests:
  - Command(s): `ctest --test-dir build -C Release --output-on-failure`
  - Result: Completed without reported failures in terminal output.
- Manual checks:
  - Confirmed Phase 1 PRD scope implemented without replacing stereo DSP path.
  - Confirmed MCH UI is state-driven and slot visibility follows selected layout.

## Known Issues / Deferred Items
- MCH DSP, multichannel bus acceptance, and per-slot quad IR validation/loading are intentionally deferred to later phases.
- Placeholder slot IR state currently stores selected file path only; no quad validation/convolution hookup in this phase.

## Next-Phase Prerequisites
- Implement Phase 2 bus layout support (up to preset-aligned input channels, fixed stereo output).
- Add direct/dry pair rendering including `C/LFE` special dry routing.
- Add Phase 2 tests for layout/dry routing behavior.

## Scope Guard For Next Chat
- Already completed: all Phase 1 UI/layout/state scaffolding and parameter persistence.
- In scope next phase: multichannel input bus support and direct/dry rendering (no convolution yet).
- Explicitly out of scope: multi-IR loading/validation per slot and full MCH convolution engine.
