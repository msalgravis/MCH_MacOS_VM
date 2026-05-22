# BinauralSpeakerRoom

Windows JUCE 8 VST3 plugin for Reaper that applies a 4-channel binaural speaker-room impulse response to stereo input. The canonical behavior and constraints live in `prd_binaural_speaker_room_vst_3.md`, especially sections 2, 5, 7, and 9.

## Current MVP

Implemented in the current repo state:

- Stereo VST3 effect built with C++20, CMake, JUCE 8, and MSVC
- 4-channel WAV IR loading and validation
- Binaural convolution routing using the PRD matrix
- Wet/Dry, Output Gain, and Bypass controls
- State restore for parameters and saved IR path
- Diagnostics panel with IR status, warnings, host sample rate, IR sample rate, meters, and clip warning
- Safe fallback to dry pass-through when no valid IR is available

For expected user behavior, see the Reaper flows in `prd_binaural_speaker_room_vst_3.md` section 5 and the testing requirements in section 9.

## Prerequisites

- Windows 10 or newer
- CMake 3.22 or newer
- Visual Studio 2022 Build Tools or Visual Studio 2022 with MSVC x64 tools
- Internet access during first configure so CMake can fetch JUCE 8.0.2 via `FetchContent`
- Reaper for host validation

## Build

Configure from the repository root:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
```

Build the Release VST3:

```powershell
cmake --build build --config Release
```

For macOS Phase 1 enablement, configure from the repository root with the Xcode generator:

```bash
cmake -S . -B build-macos -G Xcode
```

The macOS target now defaults to:

- `VST3` and `AU` plugin formats
- `12.0` as the minimum deployment target
- `arm64;x86_64` as the default universal architecture set

These defaults can be overridden at configure time if a later phase needs a narrower local build, for example:

```bash
cmake -S . -B build-macos -G Xcode -DBSR_MACOS_ARCHITECTURES=arm64
```

The build does not auto-install into the system VST3 folder. Copy the generated `.vst3` bundle from the repo artifact directory into your preferred Reaper scan path.

Current output locations after a successful Release build:

- `build/BinauralSpeakerRoom_artefacts/Release/VST3/BinauralSpeakerRoom.vst3`
- `build/BinauralSpeakerRoom_artefacts/Release/VST3/Sonarworks Internal VMPRO STEREO FX.vst3`

Expected macOS output locations after a successful Release build from `build-macos`:

- `build-macos/BinauralSpeakerRoom_artefacts/Release/VST3/Sonarworks VMPRO MCH.vst3`
- `build-macos/BinauralSpeakerRoom_artefacts/Release/AU/Sonarworks VMPRO MCH.component`

The duplicate names come from the JUCE target name and product name metadata. Reaper may display the plugin using the product name.

## Run Unit Tests

Build the standalone test target:

```powershell
cmake --build build --config Release --target BinauralSpeakerRoomTests
```

Run the registered CTest suite:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

The current test harness covers IR loading/validation, convolution routing behavior, and parameter defaults/state round-tripping.

## Reaper Install And Scan

Recommended local install flow:

1. Build the Release configuration.
2. Copy the produced `.vst3` bundle into your user or common VST3 folder.
3. On Windows, the standard scan paths are:
   - `%CommonProgramFiles%\VST3`
   - `%LOCALAPPDATA%\Programs\Common\VST3`
4. In Reaper, open `Options > Preferences > Plug-ins > VST`.
5. Ensure the folder containing the built plugin is listed in the VST plug-in paths.
6. Run `Re-scan` or `Clear cache/re-scan` if the plugin does not appear.
7. Insert the plugin on a stereo track. It may show up as `BinauralSpeakerRoom` or `Sonarworks Internal VMPRO STEREO FX` depending on host metadata display.

## Basic Validation In Reaper

1. Insert the plugin on a stereo track.
2. Confirm the initial state matches the PRD flow: no IR loaded, dry audio passes through, Wet/Dry defaults to 100%, Output Gain defaults to 0 dB, and Bypass is off.
3. Click `Load IR...` and select a valid 4-channel WAV such as `_quad_nr3_sm.wav`.
4. Confirm the status area reports a loaded IR and audio switches from dry pass-through to binaural output.
5. Save the Reaper project, close it, and reopen it.
6. Confirm parameters restore and the IR reloads when the file still exists, or a warning is shown with dry fallback if it does not.

These steps correspond to the PRD flows in sections 5.1, 5.2, and 5.4.

## Troubleshooting

### CMake configure fails before JUCE is available

- Confirm internet access is available during the first configure.
- Delete `build/CMakeCache.txt` and rerun configure if the JUCE fetch was interrupted.
- Verify you are using a supported generator such as `Visual Studio 17 2022` with `-A x64`.

### Build succeeds but Reaper cannot find the plugin

- Verify the `.vst3` bundle exists under `build/BinauralSpeakerRoom_artefacts/Release/VST3`.
- Copy the bundle to a folder that Reaper scans, or add the build output folder to Reaper's VST paths.
- Run `Clear cache/re-scan` in Reaper if the plugin was previously missing or changed name.

### Plugin loads but passes dry audio only

- This is expected when no IR is loaded or when the saved IR file is missing or invalid.
- Load a valid 4-channel WAV. Mono, stereo, and non-WAV files are rejected by design.
- Check the diagnostics panel for the exact warning text.

### Saved session reopens without the expected IR

- Confirm the original IR file still exists at the saved path.
- If the file moved, reload it manually. The plugin should warn and fall back to dry audio instead of failing silently.

### Output is too loud or clipping

- The plugin does not normalize individual IR channels or apply hidden gain compensation.
- Use `Output Gain` as the final trim control and watch the clip warning indicator.
- If comparing against speakers, follow the manual A/B guidance in PRD section 5.3.

## Validation Status

Validated in this repository so far:

- Windows Release build through CMake and MSVC
- Standalone CMake/CTest unit-test target for IR loading, convolution routing, and parameter state coverage
- Manual Reaper loading and reload checks from earlier phases

Not yet wired into the top-level CMake build:

- Automated host-side Reaper validation
- Optional Steinberg VST3 validator integration

## Optional CI

An optional GitHub Actions workflow can build the Release VST3 on Windows to catch configure/build regressions. It should be treated as a build gate, not as full plugin validation, because Reaper-host checks and a test executable are not yet automated.

This repository now also includes a native macOS CI workflow at `.github/workflows/macos-build.yml`.

What the macOS workflow does automatically:

- Builds on `macos-14` as a universal target (`arm64;x86_64`)
- Builds on `macos-13` as an Intel target (`x86_64`)
- Builds plugin and unit test targets with Xcode generator
- Runs CTest on macOS runners
- Verifies produced `.vst3` and `.component` bundles exist
- Captures architecture and bundle metadata checks (`lipo`, `file`, `Info.plist`, `codesign --display`)
- Uploads plugin artifacts and validation reports

This allows direct native macOS build and compatibility checks without a local macOS device. Interactive host checks (for example Reaper UI behavior and Retina UX) still require a manual macOS validation pass.

## macOS Artifact And Deployment (Phase 5)

Current packaging scope for macOS uses a VST3-first runtime path while keeping AU available in deliverables.

Installable artifact set (current scope):

- Primary plugin bundle: `Sonarworks VMPRO MCH.vst3`
- Optional/secondary plugin bundle: `Sonarworks VMPRO MCH.component` (AU)
- Optional validation bundle: macOS CI validation reports artifact (for engineering traceability)
- IR assets: external user-managed files (not bundled into plugin artifacts)

Expected macOS artifact locations (local native build from `build-macos`):

- `build-macos/BinauralSpeakerRoom_artefacts/Release/VST3/Sonarworks VMPRO MCH.vst3`
- `build-macos/BinauralSpeakerRoom_artefacts/Release/AU/Sonarworks VMPRO MCH.component`

Expected macOS artifact locations (downloaded GitHub Actions artifact):

- `tools/github-actions/macos-plugins-macos-14-run<run-id>/VST3/Sonarworks VMPRO MCH.vst3`
- `tools/github-actions/macos-plugins-macos-14-run<run-id>/AU/Sonarworks VMPRO MCH.component`

Deploy to a new macOS machine:

1. Copy `Sonarworks VMPRO MCH.vst3` to `~/Library/Audio/Plug-Ins/VST3`.
2. If AU is required, copy `Sonarworks VMPRO MCH.component` to `~/Library/Audio/Plug-Ins/Components`.
3. If files were downloaded from the internet, clear quarantine attributes:

```bash
xattr -dr com.apple.quarantine "~/Library/Audio/Plug-Ins/VST3/Sonarworks VMPRO MCH.vst3"
xattr -dr com.apple.quarantine "~/Library/Audio/Plug-Ins/Components/Sonarworks VMPRO MCH.component"
```

4. In Reaper, open `Preferences > Plug-ins > VST` and run a rescan.
5. Validate runtime by loading a known IR (for example `_quad_nr3_sm.wav`) and confirming DSP/routing behavior.

Current signing/notarization note:

- Internal-signing and notarization workflows are intentionally deferred in current scope until Apple credentials/secrets are available.
- Keep AU enabled in the codebase and artifacts, even though VST3 is the active host/runtime path.

## MVP Release Checklist

- Confirm `cmake -S . -B build -G "Visual Studio 17 2022" -A x64` completes from a clean checkout
- Confirm `cmake --build build --config Release` succeeds
- Confirm the `.vst3` bundle is produced under `build/BinauralSpeakerRoom_artefacts/Release/VST3`
- Run the required integration checks from PRD section 9.2 in Reaper
- Verify the plugin handles missing or invalid IR files with warnings and dry fallback
- Note the exact plugin name shown in Reaper for release notes
- Record any known warnings or limitations with the release artifact

## Repository References

- Main specification: `prd_binaural_speaker_room_vst_3.md`
- Phase tracker: `TASKS.md`
- Phase handoffs: `docs/phase-handoffs/`
