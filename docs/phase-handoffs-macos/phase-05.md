# Phase 05 Handoff (macOS MCH Enablement)

## Scope Status
- Phase 5 was executed for documentation and packaging-definition scope only.
- Completed in this iteration: 5.1, 5.4, 5.5.
- Deferred in this iteration: 5.2 (signing workflow), 5.3 (notarization workflow).

## Completed Items
- 5.1 Defined installable macOS artifact set for current scope:
  - VST3 bundle (`Sonarworks VMPRO MCH.vst3`)
  - AU bundle (`Sonarworks VMPRO MCH.component`) retained as optional/secondary deliverable
  - Optional CI validation report artifact for traceability
  - External IR assets remain user-managed and are not bundled
- 5.4 Documented deployment steps for a new macOS machine in `README.md`:
  - macOS install paths for VST3 and AU
  - quarantine removal commands (`xattr`)
  - Reaper rescan and basic runtime validation flow
- 5.5 Added this final Phase 5 handoff note.

## Deferred Items
- 5.2 `PENDING`: Signing workflow setup deferred pending Apple credentials/secrets availability.
- 5.3 `PENDING`: Notarization workflow deferred by scope decision unless distribution target changes.

## Files Modified
- `macos_mch_tasks.md`
- `README.md`
- `docs/phase-handoffs-macos/phase-05.md`

## Verification Basis
- macOS runtime confidence comes from completed Phase 4 manual validation in Reaper (VST3 path):
  - plugin discovery/load
  - UI layout parity
  - IR loading
  - DSP processing
  - multichannel routing
  - session save/restore with external IR files
- CI support remains available via `.github/workflows/macos-build.yml` for build/test/bundle checks.

## Known Constraints
- Apple signing credentials/secrets are not currently available in scope.
- VST3 is the active forward runtime path; AU remains supported in codebase/artifacts but AU host runtime testing is deferred.

## Next-Phase Prerequisites
1. If internal signed distribution becomes required, provide Apple signing credentials and add 5.2 workflow implementation.
2. If public distribution becomes required, implement 5.3 notarization and stapling pipeline.
3. Continue VST3-first validation/deployment unless host requirements change.
