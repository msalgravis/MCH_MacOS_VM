#pragma once

#include <JuceHeader.h>

#include <array>

#include "LayoutPreset.h"

namespace bsr::mch
{
enum class SlotIrStatus
{
    missing,
    loaded,
    invalid,
    error
};

class SlotIrManager
{
public:
    struct SlotIrSnapshot
    {
        bool isLoaded = false;
        juce::uint64 version = 0;
        std::shared_ptr<const juce::AudioBuffer<float>> processedIr;
    };

    SlotIrManager();

    void setHostSampleRate(double sampleRateHz) noexcept;
    double getHostSampleRateHz() const noexcept { return hostSampleRateHz; }

    bool loadSlotIrFile(int slotIndex, const juce::File& filePath, juce::String& errorMessage) noexcept;
    bool restoreSlotFromPath(int slotIndex, const juce::String& storedPath, juce::String& errorMessage) noexcept;

    void clearSlot(int slotIndex) noexcept;
    void clearAll() noexcept;

    bool isSlotLoaded(int slotIndex) const noexcept;
    SlotIrStatus getSlotStatus(int slotIndex) const noexcept;
    juce::String getSlotPath(int slotIndex) const;
    juce::String getSlotError(int slotIndex) const;
    SlotIrSnapshot getSlotSnapshot(int slotIndex) const noexcept;
    juce::String getSlotMetadataText(int slotIndex) const noexcept;

private:
    struct SlotData
    {
        SlotIrStatus status = SlotIrStatus::missing;
        juce::uint64 version = 0;
        juce::String sourcePath;
        juce::String lastError;
        int sourceSampleRate = 0;
        int sourceNumSamples = 0;
        juce::AudioBuffer<float> sourceBuffer;
        juce::AudioBuffer<float> processedBuffer;
        std::shared_ptr<const juce::AudioBuffer<float>> processedSnapshot;
    };

    std::array<SlotData, maxPairSlots> slots;
    double hostSampleRateHz = 0.0;
    juce::AudioFormatManager formatManager;

    static bool isValidSlotIndex(int slotIndex) noexcept;
    bool readQuadWav(const juce::File& filePath,
                     juce::AudioBuffer<float>& outBuffer,
                     int& outSampleRate,
                     juce::String& errorMessage) noexcept;

    void setSlotFailure(int slotIndex,
                        SlotIrStatus status,
                        const juce::String& message,
                        const juce::String& sourcePath) noexcept;

    void resampleSlotIfNeeded(int slotIndex) noexcept;
    static void resampleChannelLinear(const float* src, int srcSamples, float* dst, int dstSamples) noexcept;
};

} // namespace bsr::mch
