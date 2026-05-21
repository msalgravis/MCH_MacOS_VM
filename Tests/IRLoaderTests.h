#pragma once

#include <JuceHeader.h>
#include "../Source/IRLoader.h"

class IRLoaderTests : public juce::UnitTest
{
public:
    IRLoaderTests() : juce::UnitTest("IRLoader", "IR Loading") {}

    void runTest() override
    {
        beginTest("Valid 4-channel WAV file");
        {
            juce::TemporaryFile tempWav(".wav");
            auto outputStream = tempWav.getFile().createOutputStream();

            expect(outputStream != nullptr, "Should create a temporary WAV output stream");

            if (outputStream != nullptr)
            {
                juce::WavAudioFormat format;
                std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(outputStream.get(), 48000.0, 4, 24, {}, 0));

                expect(writer != nullptr, "Should create a WAV writer for 4-channel test data");

                if (writer != nullptr)
                {
                    outputStream.release();

                    juce::AudioBuffer<float> buffer(4, 16);
                    buffer.clear();
                    buffer.setSample(0, 0, 1.0f);
                    buffer.setSample(1, 1, 0.5f);
                    buffer.setSample(2, 2, -0.5f);
                    buffer.setSample(3, 3, 0.25f);

                    expect(writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples()), "Should write the temporary test WAV");
                    writer.reset();

                    IRLoader loader;
                    juce::String errorMsg;

                    bool result = loader.loadWavFile(tempWav.getFile(), errorMsg);
                    expect(result, "Should load a valid 4-channel WAV file");
                    expect(errorMsg.isEmpty(), "Valid load should not set an error message");
                    expect(loader.isLoaded(), "Loader should report the file as loaded");
                    expectEquals(loader.getMetadata().channels, 4, "Metadata should report four channels");
                    expectEquals(loader.getMetadata().sampleRate, 48000, "Metadata should preserve sample rate");
                    expectEquals(loader.getMetadata().numSamples, 16, "Metadata should preserve sample count");
                }
            }
        }

        beginTest("Reject non-WAV files");
        {
            IRLoader loader;
            juce::String errorMsg;

            // Create a non-WAV file
            juce::TemporaryFile testFile(".txt");
            expect(testFile.getFile().replaceWithText("not a wav"), "Should create the temporary text file");

            bool result = loader.loadWavFile(testFile.getFile(), errorMsg);
            expect(result == false, "Should reject non-WAV files");
            expect(errorMsg.contains("WAV"), "Error message should mention WAV format");
        }

        beginTest("Reject missing file");
        {
            IRLoader loader;
            juce::String errorMsg;

            auto missingFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                .getChildFile("irloader_missing_fixture")
                .getChildFile("missing.wav");
            bool result = loader.loadWavFile(missingFile, errorMsg);

            expect(result == false, "Should fail when file doesn't exist");
            expect(errorMsg.contains("not found"), "Error should mention file not found");
        }

        beginTest("IRMetadata duration calculation");
        {
            IRMetadata metadata;
            metadata.sampleRate = 48000;
            metadata.numSamples = 48000; // 1 second at 48kHz

            expectEquals(metadata.getDurationMs(), 1000.0, "Should calculate 1 second correctly");

            metadata.numSamples = 24000; // 0.5 seconds
            expectEquals(metadata.getDurationMs(), 500.0, "Should calculate 0.5 seconds correctly");

            metadata.sampleRate = 0;
            metadata.numSamples = 1000;
            expectEquals(metadata.getDurationMs(), 0.0, "Should return 0 for invalid sample rate");
        }

        beginTest("IRLoader unload");
        {
            IRLoader loader;
            juce::String errorMsg;

            // Create a temporary valid WAV-like structure (simplified for testing)
            // For this test, we'll just verify the unload functionality
            loader.unload();
            expect(loader.getLoadedPath().isEmpty(), "Loaded path should be empty after unload");
            expect(!loader.isLoaded(), "isLoaded() should return false after unload");
        }
    }
};

static IRLoaderTests irLoaderTests;
