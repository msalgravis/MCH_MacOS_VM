# Project Guidelines

## Scope And Source Of Truth
- Stereo baseline work is completed under `prd_binaural_speaker_room_vst_3.md`.
- For extension work, treat `prd_multichannel_binaural_renderer_extension.md` as the canonical specification.
- The current Windows plugin is functional and is the behavioral reference implementation for all macOS work.
- Keep backward compatibility with the completed stereo behavior unless the extension PRD explicitly changes it.
- The next project is macOS enablement; implement platform support without changing established Windows behavior, DSP results, parameter behavior, state behavior, or user-facing functionality unless the user explicitly approves a deviation.
- The fixture WAV `_quad_nr3_sm.wav` remains valid for stereo regression checks.

## Code Style
- Use C++20 and JUCE 8.x conventions described in the PRD.
- Keep changes modular and scoped (DSP, file I/O, parameters, and UI in separate classes).
- Avoid broad rewrites when a focused patch can satisfy the task.
- Add comments only where they preserve important intent/context (DSP assumptions, routing rationale, threading constraints, host-specific behavior). Keep comments concise and avoid obvious line-by-line narration.
- Keep generated code purpose-driven and simple. Prefer the smallest correct implementation for the current phase; avoid speculative abstractions, feature creep, and unnecessary complexity.

## Prompt Clarification
- If the user's purpose, desired outcome, or requested behavior is unclear or ambiguous in any meaningful way, ask focused follow-up questions before making code changes.
- Do not guess between multiple plausible interpretations when a short clarification would change implementation details, validation strategy, or scope.
- If a macOS-specific constraint, host difference, code-signing requirement, filesystem difference, plugin-format issue, or API limitation forces a behavioral decision, stop and ask the user before changing behavior or accepting a platform-specific deviation.
- After each new user prompt, explicitly check whether the goal is sufficiently clear; if not, pause and ask only the minimum additional questions needed to proceed correctly.

## Architecture
- Keep these boundaries strict:
- `IRLoader`: file parsing/validation and IR metadata only.
- `BinauralConvolver`: convolution/routing DSP only.
- `PluginProcessor`: APVTS parameters, state, and audio-thread orchestration.
- `PluginEditor`: UI only.
- Follow the routing matrix from the PRD: `Out L = In L * LL + In R * RL`, `Out R = In L * LR + In R * RR`.
- For the multichannel extension, keep new logic modular in dedicated classes:
- `LayoutPreset`/layout mapping: preset definitions, channel maps, visible slot rules.
- `SpeakerPairProcessor` (or equivalent): one pair slot DSP and per-slot state interaction.
- `MchBinauralRenderer`: orchestration across all active pair processors and stereo summing.
- `IrSetManager`: IR-set persistence/import-export only.

## Build And Test
- Target platform for MVP: Windows VST3 in Reaper.
- Current known-good baseline: the Windows plugin is functional today; macOS work must preserve parity with that baseline rather than redefining behavior.
- Preferred toolchain: Visual Studio 2022 Build Tools (MSVC) with CMake.
- Standard build flow:
- `cmake -S . -B build`
- `cmake --build build --config Release`
- Automated validator requirement (`pluginval`):
- Always validate the built VST3 after code changes that can affect plugin behavior/loading.
- Validation target path: `build/BinauralSpeakerRoom_artefacts/Release/VST3/Sonarworks VMPRO MCH.vst3`.
- Preferred `pluginval` location: `tools/pluginval/pluginval.exe`.
- If `pluginval` is missing, install it locally by downloading the latest Windows release ZIP from Tracktion/pluginval, extract to `tools/pluginval/`, then run validation.
- Minimum validation command (strict level 10 recommended):
- `tools/pluginval/pluginval.exe --strictness-level 10 --repeat 1 --timeout-ms 120000 "build/BinauralSpeakerRoom_artefacts/Release/VST3/Sonarworks VMPRO MCH.vst3"`
- Treat non-zero `pluginval` exit code as a failed verification and report failing test sections in-chat.
- Keep and reuse validation logs under `tools/pluginval/logs/` for regression comparisons.
- When tests exist, run the test target/executable and prioritize unit coverage for IR loading, routing correctness, and parameter/state behavior.

## Session Learnings (2026-05-19)

## Session Learnings (2026-05-21)

### macOS CI Validation Pipeline
- Native macOS validation is now wired via `.github/workflows/macos-build.yml` and should be treated as the first autonomous gate for Phase 2.2/3.x.
- Current matrix intent:
- `macos-14` with universal `arm64;x86_64`
- `macos-13` with `x86_64`
- CI runs should build plugin targets, run `ctest`, validate bundle metadata (`lipo`, `file`, `Info.plist`, `codesign --display`), and upload artifacts/reports.
- If a new chat starts Phase 3 work, always check latest Actions run status/results first before applying code changes.

### macOS CI Troubleshooting Flow
- If macOS build fails with generic `xcodebuild`/exit-65 style errors and logs are not directly available, prefer stabilizing CI generator/toolchain first (e.g., Ninja single-config Release) before making broad source changes.
- Keep fixes minimal and parity-safe: no DSP behavior changes while chasing macOS build portability.
- After each CI fix push, re-check run/job states and step-level outcomes to identify whether failure moved past `Configure`, `Build Release`, `ctest`, or bundle validation.

### DSP vs UI Separation
- MCH DSP backend (MchBinauralRenderer, SpeakerPairProcessor, MchSlotIrManager) is **fully implemented and working** independently.
- UI layer (PluginEditor) and DSP layer can be developed/debugged separately.
- Always verify DSP backend with automated tests (pluginval strict, unit tests) before assuming UI issues.

### Parameter State Restoration
- Avoid single-choice `AudioParameterChoice` parameters; they cause NaN comparison failures in pluginval state restoration tests.
- Use `AudioParameterBool` or `AudioParameterFloat` with appropriate ranges as compatibility parameters.
- Actual string/path data can be stored separately in APVTS state properties, not in the parameter value itself.

### UI Wiring for MCH Pair Slots
- Mode selector (Stereo/MCH) and layout preset selector are separate APVTS parameters.
- Pair slot visibility depends on **both** mode AND layout; must check both in `updateMchUi()`.
- Use timer callback (4Hz typical) to reactively update UI when mode or layout changes.
- Create UI components (sliders, buttons, toggles) once in `initialisePairSlots()`, then update visibility/labels in `updateMchUi()`.
- Pair slot row components must include APVTS attachments for each control (Enable, Mute, Solo, Gain, Pan, Wet/Dry).

### Testing Quad IR Loading
- Use `_quad_nr3_sm.wav` fixture file to validate MCH slot IR loading functionality.
- Can programmatically load same quad file into multiple slots to test slot independence.
- Verify slot status API (`getMchSlotStatusText()`, `isMchSlotIRLoaded()`) returns correct values after load.

### Tracker vs Code Reality Check
- Task tracker (`tasks_mch.md`) can diverge from actual code state if previous sessions had reverts or incomplete work.
- **Source of truth**: latest code + most recent handoff note in `docs/phase-handoffs-mch/`.
- Always audit actual codebase (check for empty methods, stub implementations, TODO comments) before trusting tracker claims.
- Update tracker to match code reality, not vice versa.

### Pluginval as Essential Gate
- Pluginval strict (level 10) must pass before releasing code or proceeding to manual Reaper testing.
- Exit code 0 = all tests passed; non-zero = investigation required.
- Log files kept in `tools/pluginval/logs/` enable regression tracking across iterations.
- Pluginval catches parameter state issues, parameter type mismatches, and threading problems that Reaper might later trigger.

## Conventions
- Do not normalize individual IR channels or add hidden gain compensation.
- Preserve relative levels across LL/LR/RL/RR channels.
- In MCH mode, preserve relative levels across pair slots as well (no inter-slot normalization or loudness matching).
- For macOS enablement, prefer platform-compatibility changes over behavior changes; keep output, routing, controls, parameter semantics, state restore behavior, and failure/fallback behavior aligned with the current Windows plugin.
- Keep `processBlock` real-time safe: no file I/O, no heap allocations, and no blocking locks.
- Perform IR loading off the audio thread and apply updates atomically/safely.
- If IR is missing or invalid, use the safe fallback described in the PRD (dry pass-through with clear status/warning).
- Keep output fixed stereo in both stereo and MCH modes.

## Phase Workflow (Multi-Chat Development)
- Treat each phase in the active tracker as an isolated execution unit, but require a handoff check before coding.
- Use `tasks.md` for the completed stereo baseline and `tasks_mch.md` for the multichannel extension.
- At the start of every new chat/phase, first read:
- The active tracker file (`tasks.md` or `tasks_mch.md`) for planned checklist and current phase
- The previous phase handoff note in the matching folder:
- Stereo baseline: `docs/phase-handoffs/phase-XX.md`
- MCH extension: `docs/phase-handoffs-mch/phase-XX.md`
- Before implementing, summarize in-chat:
- What is already completed
- What is in scope for the current phase only
- What is explicitly out of scope to prevent overlap
- At the end of each phase, create/update a short handoff note containing:
- Completed items
- Files created/modified
- Known issues or deferred items
- Verification performed (build/tests/manual checks)
- Next-phase prerequisites
- After each phase execution, update the active tracker file in the same change:
- Mark completed items with `✅ COMPLETED` in the Status column.
- Mark the phase title with a `✅` prefix when all items in that phase are completed.
- Keep non-completed items as `PENDING`, `IN PROGRESS`, or `NEEDS WORK` as appropriate.
- If tracker and code disagree, treat code + latest handoff note as source of truth and update the tracker accordingly.

## References
- Stereo baseline spec: `prd_binaural_speaker_room_vst_3.md`
- Multichannel extension spec: `prd_multichannel_binaural_renderer_extension.md`
- MCH implementation phases: extension PRD section `11`
- MCH testing requirements: extension PRD section `13`
- Copilot prompts for MCH phases: extension PRD section `14.3-14.8`