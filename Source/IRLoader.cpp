#include "IRLoader.h"

bool IRLoader::loadWavFile(const juce::File& filePath, juce::String& errorMessage) noexcept
{
    // Validate file exists
    if (!filePath.exists())
    {
        errorMessage = "File not found: " + filePath.getFullPathName();
        unload();
        return false;
    }

    // Check file extension
    if (filePath.getFileExtension().toLowerCase() != ".wav")
    {
        errorMessage = "File must be WAV format";
        unload();
        return false;
    }

    // Try to read the WAV file
    std::unique_ptr<juce::AudioFormatReader> reader;
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    auto* wavReader = formatManager.createReaderFor(filePath);
    if (!wavReader)
    {
        errorMessage = "Failed to read WAV file";
        unload();
        return false;
    }
    reader.reset(wavReader);

    // Validate channel count (must be exactly 4)
    if (reader->numChannels != 4)
    {
        errorMessage = juce::String::formatted("IR must have exactly 4 channels, but file has %d", reader->numChannels);
        unload();
        return false;
    }

    int numSamples = static_cast<int>(reader->lengthInSamples);
    irBuffer.setSize(4, numSamples, true, false, true);

    // Read normalized float samples directly so all WAV formats map consistently.
    if (!reader->read(&irBuffer, 0, numSamples, 0, true, true))
    {
        errorMessage = "Failed to read audio data from WAV file";
        unload();
        return false;
    }

    // Extract metadata
    metadata.fileName = filePath.getFileName();
    metadata.sampleRate = static_cast<int>(reader->sampleRate);
    metadata.channels = reader->numChannels;
    metadata.numSamples = numSamples;

    // Store the loaded path
    loadedPath = filePath.getFullPathName();

    return true;
}

const float* IRLoader::getChannelData(int channel) const noexcept
{
    if (!isLoaded() || channel < 0 || channel >= 4)
        return nullptr;
    return irBuffer.getReadPointer(channel);
}

void IRLoader::unload() noexcept
{
    metadata = IRMetadata();
    loadedPath.clear();
    irBuffer.setSize(0, 0, true, false, true);
}
