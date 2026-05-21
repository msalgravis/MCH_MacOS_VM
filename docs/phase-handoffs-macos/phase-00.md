# Phase 00 Handoff (macOS MCH Enablement)

## Completed Items
- 0.1 Confirmed required macOS plugin formats: `VST3 + AU`.
- 0.2 Confirmed required macOS architectures: universal `arm64 + x86_64`.
- 0.3 Confirmed deployment/signing target: signed internal validation builds; notarization out of scope for now.
- 0.4 Locked macOS build baseline for implementation start.
- 0.5 macOS-specific tracker exists and is now updated for Phase 0 completion.

## Locked Decisions
- Plugin formats: `VST3 + AU` with VST3-first validation flow in Reaper.
- Architectures: universal `arm64 + x86_64` (arm64-only fallback allowed only if universal becomes disproportionate risk).
- Minimum supported macOS version: `12.0 (Monterey)`.
- Distribution target: internal validation builds, signed as needed for local host/Gatekeeper behavior.
- Build baseline: Xcode 16.x, CMake `Xcode` generator, Reaper 7.x.
- Validation baseline: native Apple Silicon first; Rosetta compatibility pass when Intel slice validation is required.

## Files Created/Modified
- `macos_mch_tasks.md` - marked Phase 0 complete and recorded locked decision gates.
- `docs/phase-handoffs-macos/phase-00.md` - created initial macOS Phase 0 handoff note.

## Verification Performed
- Build:
  - Command(s): Not run in this planning/scope-lock phase.
  - Result: Not applicable.
- Tests:
  - Command(s): Not run in this planning/scope-lock phase.
  - Result: Not applicable.
- Manual checks:
  - Confirmed decision-gate values are captured in tracker before implementation work.
  - Confirmed phase workflow now includes a dedicated macOS handoff note path.

## Known Issues / Deferred Items
- No macOS build attempted yet (expected in Phase 1+).
- Baseline values (Xcode/Reaper minors) may be pinned more strictly when implementation begins on the target machine.

## Next-Phase Prerequisites
- Start Phase 1 with a focused CMake/JUCE audit for macOS target formats, metadata, and architecture/deployment settings.
- Keep behavior parity with current Windows plugin as the reference implementation.
