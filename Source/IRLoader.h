#pragma once

#include <JuceHeader.h>
#include <memory>

struct IRMetadata
{
    juce::String fileName;
    int sampleRate = 0;
    int channels = 0;
    int numSamples = 0;

    double getDurationMs() const
    {
        if (sampleRate <= 0)
            return 0.0;
        return (static_cast<double>(numSamples) / static_cast<double>(sampleRate)) * 1000.0;
    }
};

class IRLoader
{
public:
    IRLoader() = default;
    ~IRLoader() = default;

    /// @brief Load a WAV file and validate it contains exactly 4 channels
    /// @param filePath Path to the WAV file
    /// @param errorMessage Reference to store error message if loading fails
    /// @return true if successfully loaded, false otherwise
    bool loadWavFile(const juce::File& filePath, juce::String& errorMessage) noexcept;

    /// @brief Get metadata of the currently loaded IR
    /// @return IRMetadata struct containing file info
    const IRMetadata& getMetadata() const noexcept { return metadata; }

    /// @brief Get the path of the currently loaded IR file
    /// @return File path, or empty string if no IR loaded
    juce::String getLoadedPath() const noexcept { return loadedPath; }

    /// @brief Check if an IR is currently loaded
    /// @return true if IR is loaded and valid
    bool isLoaded() const noexcept { return !loadedPath.isEmpty() && metadata.channels == 4; }

    /// @brief Get IR channel data (LL, LR, RL, or RR)
    /// @param channel Channel index: 0=LL, 1=LR, 2=RL, 3=RR
    /// @return Const pointer to channel data, or nullptr if not loaded or invalid channel
    const float* getChannelData(int channel) const noexcept;

    /// @brief Clear the loaded IR (unload)
    void unload() noexcept;

private:
    IRMetadata metadata;
    juce::String loadedPath;
    juce::AudioBuffer<float> irBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IRLoader)
};
