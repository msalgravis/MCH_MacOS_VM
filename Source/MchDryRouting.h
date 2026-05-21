#pragma once

#include <JuceHeader.h>

#include "LayoutPreset.h"

namespace bsr::mch
{
inline constexpr int maxLayoutInputChannels = 16;

inline bool isSupportedLayoutInputCount(int channelCount) noexcept
{
    const auto& presets = getLayoutPresets();
    return std::any_of(presets.begin(), presets.end(), [channelCount](const auto& preset)
    {
        return preset.channelCount == channelCount;
    });
}

inline bool isCAndLfePair(const PairSlotDefinition& pairSlot) noexcept
{
    return pairSlot.inputA == 3 && pairSlot.inputB == 4;
}

inline void renderDryRouting(const LayoutPresetDefinition& preset,
                             const float* const* inputChannels,
                             int inputChannelCount,
                             float* outputL,
                             float* outputR,
                             int numSamples) noexcept
{
    if (inputChannels == nullptr || outputL == nullptr || outputR == nullptr || numSamples <= 0)
        return;

    juce::FloatVectorOperations::clear(outputL, numSamples);
    juce::FloatVectorOperations::clear(outputR, numSamples);

    const auto visibleSlotCount = juce::jlimit(0, maxPairSlots, preset.visibleSlotCount);
    if (visibleSlotCount <= 0)
        return;

    const auto& firstSlot = preset.slots[0];
    const auto firstA = firstSlot.inputA - 1;
    const auto firstB = firstSlot.inputB - 1;

    if (firstA >= 0 && firstA < inputChannelCount && inputChannels[static_cast<size_t>(firstA)] != nullptr)
        juce::FloatVectorOperations::copy(outputL, inputChannels[static_cast<size_t>(firstA)], numSamples);

    if (firstB >= 0 && firstB < inputChannelCount && inputChannels[static_cast<size_t>(firstB)] != nullptr)
        juce::FloatVectorOperations::copy(outputR, inputChannels[static_cast<size_t>(firstB)], numSamples);

    for (int slotIndex = 1; slotIndex < visibleSlotCount; ++slotIndex)
    {
        const auto& slot = preset.slots[static_cast<size_t>(slotIndex)];

        const auto inputA = slot.inputA - 1;
        const auto inputB = slot.inputB - 1;
        const bool hasInputA = inputA >= 0 && inputA < inputChannelCount && inputChannels[static_cast<size_t>(inputA)] != nullptr;
        const bool hasInputB = inputB >= 0 && inputB < inputChannelCount && inputChannels[static_cast<size_t>(inputB)] != nullptr;

        if (isCAndLfePair(slot))
        {
            if (hasInputA)
            {
                juce::FloatVectorOperations::add(outputL, inputChannels[static_cast<size_t>(inputA)], numSamples);
                juce::FloatVectorOperations::add(outputR, inputChannels[static_cast<size_t>(inputA)], numSamples);
            }

            if (hasInputB)
            {
                juce::FloatVectorOperations::add(outputL, inputChannels[static_cast<size_t>(inputB)], numSamples);
                juce::FloatVectorOperations::add(outputR, inputChannels[static_cast<size_t>(inputB)], numSamples);
            }

            continue;
        }

        if (hasInputA)
            juce::FloatVectorOperations::add(outputL, inputChannels[static_cast<size_t>(inputA)], numSamples);

        if (hasInputB)
            juce::FloatVectorOperations::add(outputR, inputChannels[static_cast<size_t>(inputB)], numSamples);
    }
}
} // namespace bsr::mch