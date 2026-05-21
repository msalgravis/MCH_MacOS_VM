#pragma once

#include <JuceHeader.h>

#include <array>
#include <cstdint>

#include "LayoutPreset.h"
#include "MchSlotIrManager.h"
#include "SpeakerPairProcessor.h"

namespace bsr::mch
{
inline constexpr int maxRendererInputChannels = 16;

class MchBinauralRenderer
{
public:
    struct PairRuntimeParameters
    {
        bool enabled = true;
        bool muted = false;
        bool solo = false;
        float gainLinear = 1.0f;
        float pan = 0.0f; // normalized -1..1
        float pairWet = 1.0f; // normalized 0..1
    };

    struct RuntimeStats
    {
        int activePairs = 0;
        int visiblePairs = 0;
        int skippedPairs = 0;
        int activeWetPairs = 0;
        int activeDryPairs = 0;
        int activeConvolutionPaths = 0;
        int maxConvolutionPaths = 0;
        bool soloActive = false;
        std::array<float, maxPairSlots> slotOutputRmsL {};
        std::array<float, maxPairSlots> slotOutputRmsR {};
    };

    void prepareToPlay(double sampleRate, int maximumBlockSize) noexcept;
    void releaseResources() noexcept;

    void applySlotIrSnapshot(int slotIndex, const SlotIrManager::SlotIrSnapshot& snapshot) noexcept;
    void applyAllSlotIrSnapshots(const SlotIrManager& slotIrManager) noexcept;

    RuntimeStats render(const LayoutPresetDefinition& preset,
                        const std::array<const float*, maxRendererInputChannels>& inputChannels,
                        int inputChannelCount,
                        const std::array<PairRuntimeParameters, maxPairSlots>& pairParameters,
                        float globalWet,
                        float globalOutputGain,
                        float* outputL,
                        float* outputR,
                        int numSamples) noexcept;

private:
    std::array<SpeakerPairProcessor, maxPairSlots> pairProcessors;
    std::array<juce::uint64, maxPairSlots> appliedSlotVersions {};
    juce::AudioBuffer<float> zeroInput;
    juce::AudioBuffer<float> slotOutputScratch;
    int maxBlockSize = 0;
};
} // namespace bsr::mch
