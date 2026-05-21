#pragma once

#include <JuceHeader.h>

#include "../Source/MchSlotIrManager.h"

class MchSlotIrManagerTests : public juce::UnitTest
{
public:
    MchSlotIrManagerTests() : juce::UnitTest("MchSlotIrManager", "MCH") {}

    void runTest() override
    {
        beginTest("Loads valid quad IR per slot");
        testValidQuadLoad();

        beginTest("Rejects non-quad IR per slot");
        testRejectsStereo();

        beginTest("Restore missing file reports error state");
        testMissingFileRestore();

        beginTest("Restore accepts quoted saved path with spaces");
        testRestoreQuotedPathWithSpaces();

        beginTest("Resamples loaded IR to host sample rate");
        testResampling();

        beginTest("Snapshot version updates and old snapshot remains stable");
        testSnapshotVersionAndStability();

        beginTest("Loads fixture quad file into multiple slots");
        testFixtureQuadMultiSlot();
    }

private:
    static bool writeTestWav(const juce::File& file, int channels, double sampleRate, int numSamples)
    {
        auto stream = file.createOutputStream();
        if (stream == nullptr)
            return false;

        juce::WavAudioFormat format;
        auto writer = std::unique_ptr<juce::AudioFormatWriter>(
            format.createWriterFor(stream.get(), sampleRate, static_cast<unsigned int>(channels), 24, {}, 0));
        if (writer == nullptr)
            return false;

        stream.release();

        juce::AudioBuffer<float> buffer(channels, numSamples);
        for (int ch = 0; ch < channels; ++ch)
            for (int sample = 0; sample < numSamples; ++sample)
                buffer.setSample(ch, sample, (sample == ch) ? 1.0f : 0.0f);

        return writer->writeFromAudioSampleBuffer(buffer, 0, numSamples);
    }

    void testValidQuadLoad()
    {
        juce::TemporaryFile tempFile(".wav");
        expect(writeTestWav(tempFile.getFile(), 4, 48000.0, 32), "Should write valid quad WAV");

        bsr::mch::SlotIrManager manager;
        manager.setHostSampleRate(48000.0);

        juce::String error;
        const bool loaded = manager.loadSlotIrFile(0, tempFile.getFile(), error);

        expect(loaded, "Valid quad IR should load");
        expect(error.isEmpty(), "Valid quad load should not set error");
        expect(manager.isSlotLoaded(0), "Slot should be marked loaded");
        expect(manager.getSlotStatus(0) == bsr::mch::SlotIrStatus::loaded,
               "Slot status should be loaded");
        expectEquals(manager.getSlotPath(0), tempFile.getFile().getFullPathName(), "Slot path should be stored");

        const auto snapshot = manager.getSlotSnapshot(0);
        expect(snapshot.isLoaded, "Snapshot should report loaded state");
        expect(snapshot.processedIr != nullptr, "Snapshot should include processed IR");
        if (snapshot.processedIr != nullptr)
        {
            expectEquals(snapshot.processedIr->getNumChannels(), 4, "Processed IR should keep quad channels");
            expectEquals(snapshot.processedIr->getNumSamples(), 32, "Processed IR should keep sample count when sample rate matches");
        }
    }

    void testRejectsStereo()
    {
        juce::TemporaryFile tempFile(".wav");
        expect(writeTestWav(tempFile.getFile(), 2, 48000.0, 32), "Should write stereo WAV fixture");

        bsr::mch::SlotIrManager manager;
        manager.setHostSampleRate(48000.0);

        juce::String error;
        const bool loaded = manager.loadSlotIrFile(1, tempFile.getFile(), error);

        expect(!loaded, "Stereo IR should be rejected");
        expect(error.contains("exactly 4 channels"), "Error should mention quad channel requirement");
        expect(!manager.isSlotLoaded(1), "Slot should not be loaded when validation fails");
        expect(manager.getSlotStatus(1) == bsr::mch::SlotIrStatus::invalid,
               "Slot status should be invalid for non-quad file");
    }

    void testMissingFileRestore()
    {
        bsr::mch::SlotIrManager manager;

        juce::String error;
        const bool restored = manager.restoreSlotFromPath(2, "C:/nonexistent/mch_slot_ir.wav", error);

        expect(!restored, "Restoring a missing saved path should fail");
        expect(error.contains("Missing saved IR file"), "Missing restore error should be descriptive");
        expect(manager.getSlotStatus(2) == bsr::mch::SlotIrStatus::error,
               "Missing saved file should set error state");
        expect(!manager.getSlotPath(2).isEmpty(), "Missing saved path should remain visible for diagnostics");
    }

    void testRestoreQuotedPathWithSpaces()
    {
        const auto baseDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getChildFile("mch slot restore tests");
        expect(baseDir.createDirectory(), "Should create temporary test directory with spaces");

        const auto wavPath = baseDir.getChildFile("quoted restore fixture.wav");
        expect(writeTestWav(wavPath, 4, 48000.0, 64), "Should write quad WAV fixture in spaced path");

        bsr::mch::SlotIrManager manager;
        manager.setHostSampleRate(48000.0);

        const auto quotedPath = "  \"" + wavPath.getFullPathName() + "\"  ";
        juce::String error;
        const bool restored = manager.restoreSlotFromPath(4, quotedPath, error);

        expect(restored, "Restore should accept quoted path with surrounding whitespace");
        expect(error.isEmpty(), "Successful restore should not set error text");
        expect(manager.getSlotStatus(4) == bsr::mch::SlotIrStatus::loaded,
               "Quoted restore should load slot as expected");
        expectEquals(manager.getSlotPath(4), wavPath.getFullPathName(),
                     "Stored slot path should be normalized to a plain full path");

        baseDir.deleteRecursively();
    }

    void testResampling()
    {
        juce::TemporaryFile tempFile(".wav");
        expect(writeTestWav(tempFile.getFile(), 4, 48000.0, 48), "Should write quad WAV for resampling test");

        bsr::mch::SlotIrManager manager;
        manager.setHostSampleRate(96000.0);

        juce::String error;
        const bool loaded = manager.loadSlotIrFile(3, tempFile.getFile(), error);

        expect(loaded, "Valid quad should load before resampling check");
        expect(error.isEmpty(), "No error expected while loading resampling fixture");

        const auto snapshot = manager.getSlotSnapshot(3);
        expect(snapshot.processedIr != nullptr, "Processed IR should be available for resampling test");
        if (snapshot.processedIr != nullptr)
            expectEquals(snapshot.processedIr->getNumSamples(), 96, "48k IR should double to 96 samples at 96k host");
    }

    void testSnapshotVersionAndStability()
    {
        juce::TemporaryFile firstFile(".wav");
        juce::TemporaryFile secondFile(".wav");
        expect(writeTestWav(firstFile.getFile(), 4, 48000.0, 24), "Should write first quad WAV");
        expect(writeTestWav(secondFile.getFile(), 4, 48000.0, 40), "Should write second quad WAV");

        bsr::mch::SlotIrManager manager;
        manager.setHostSampleRate(48000.0);

        juce::String error;
        expect(manager.loadSlotIrFile(0, firstFile.getFile(), error), "Should load first file");
        const auto firstSnapshot = manager.getSlotSnapshot(0);

        expect(manager.loadSlotIrFile(0, secondFile.getFile(), error), "Should hot-reload second file");
        const auto secondSnapshot = manager.getSlotSnapshot(0);

        expect(secondSnapshot.version > firstSnapshot.version, "Version should increase after slot replacement");
        expect(firstSnapshot.processedIr != nullptr && secondSnapshot.processedIr != nullptr,
               "Both snapshots should keep valid buffer references");
        if (firstSnapshot.processedIr != nullptr)
            expectEquals(firstSnapshot.processedIr->getNumSamples(), 24,
                         "Original snapshot should retain original sample count after replacement");
        if (secondSnapshot.processedIr != nullptr)
            expectEquals(secondSnapshot.processedIr->getNumSamples(), 40,
                         "Updated snapshot should expose new sample count");

        manager.clearSlot(0);
        const auto clearedSnapshot = manager.getSlotSnapshot(0);
        expect(!clearedSnapshot.isLoaded, "Cleared slot snapshot should report unloaded state");
        expect(clearedSnapshot.version > secondSnapshot.version, "Version should increase after clear");
    }

    void testFixtureQuadMultiSlot()
    {
        // Find and load the fixture quad IR file
        const auto fixtureFile = juce::File::getCurrentWorkingDirectory()
            .getChildFile("_quad_nr3_sm.wav");

        if (!fixtureFile.exists())
        {
            logMessage("Fixture quad file _quad_nr3_sm.wav not found; skipping fixture load test");
            return;
        }

        bsr::mch::SlotIrManager manager;
        manager.setHostSampleRate(48000.0);

        // Load the same fixture into multiple slots to test slot independence
        for (int slot = 0; slot < 3; ++slot)
        {
            juce::String error;
            const bool loaded = manager.loadSlotIrFile(slot, fixtureFile, error);

            expect(loaded, "Fixture quad file should load into slot " + juce::String(slot));
            expect(error.isEmpty(), "No error loading fixture into slot " + juce::String(slot));
            expect(manager.isSlotLoaded(slot), "Slot " + juce::String(slot) + " should be marked loaded");
            expect(manager.getSlotStatus(slot) == bsr::mch::SlotIrStatus::loaded,
                   "Slot " + juce::String(slot) + " status should be loaded");

            const auto snapshot = manager.getSlotSnapshot(slot);
            expect(snapshot.isLoaded, "Snapshot for slot " + juce::String(slot) + " should be loaded");
            expect(snapshot.processedIr != nullptr,
                   "Snapshot for slot " + juce::String(slot) + " should have processed IR");
            expect(snapshot.processedIr->getNumChannels() == 4,
                   "Loaded fixture should remain 4-channel in slot " + juce::String(slot));
        }

        // Verify slots remain independent
        const auto snapshot0 = manager.getSlotSnapshot(0);
        const auto snapshot1 = manager.getSlotSnapshot(1);
        const auto snapshot2 = manager.getSlotSnapshot(2);

        expect(snapshot0.processedIr != snapshot1.processedIr,
               "Slot 0 and 1 should have separate IR buffers");
        expect(snapshot1.processedIr != snapshot2.processedIr,
               "Slot 1 and 2 should have separate IR buffers");

        logMessage("✓ Fixture quad file loaded successfully into 3 slots");
    }
};

static MchSlotIrManagerTests mchSlotIrManagerTests;
