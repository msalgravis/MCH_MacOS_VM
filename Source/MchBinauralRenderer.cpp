#include "MchBinauralRenderer.h"

#include <cmath>

namespace bsr::mch
{
namespace
{
constexpr float minAudibleGain = 1.0e-6f;

float findBlockRms(const float* data, int numSamples) noexcept
{
    if (data == nullptr || numSamples <= 0)
        return 0.0f;

    double sumSquares = 0.0;
    for (int i = 0; i < numSamples; ++i)
        sumSquares += static_cast<double>(data[i]) * static_cast<double>(data[i]);

    return static_cast<float>(std::sqrt(sumSquares / static_cast<double>(numSamples)));
}

bool isCenterLfePair(const PairSlotDefinition& slotDef) noexcept
{
    return slotDef.inputA == 3 && slotDef.inputB == 4;
}
}

void MchBinauralRenderer::prepareToPlay(double sampleRate, int maximumBlockSize) noexcept
{
    maxBlockSize = juce::jmax(1, maximumBlockSize);
    zeroInput.setSize(1, maxBlockSize, true, false, true);
    slotOutputScratch.setSize(2, maxBlockSize, false, true, true);

    for (auto& processor : pairProcessors)
        processor.prepareToPlay(sampleRate, maxBlockSize);

    appliedSlotVersions.fill(0);
}

void MchBinauralRenderer::releaseResources() noexcept
{
    for (auto& processor : pairProcessors)
        processor.releaseResources();

    appliedSlotVersions.fill(0);
    zeroInput.setSize(0, 0, true, false, true);
    slotOutputScratch.setSize(0, 0, true, false, true);
    maxBlockSize = 0;
}

void MchBinauralRenderer::applySlotIrSnapshot(int slotIndex, const SlotIrManager::SlotIrSnapshot& snapshot) noexcept
{
    if (slotIndex < 0 || slotIndex >= maxPairSlots)
        return;

    auto& appliedVersion = appliedSlotVersions[static_cast<size_t>(slotIndex)];
    if (snapshot.version == appliedVersion)
        return;

    auto& pairProcessor = pairProcessors[static_cast<size_t>(slotIndex)];
    if (snapshot.isLoaded && snapshot.processedIr != nullptr)
        pairProcessor.loadIR(*snapshot.processedIr);
    else
        pairProcessor.clearIR();

    appliedVersion = snapshot.version;
}

void MchBinauralRenderer::applyAllSlotIrSnapshots(const SlotIrManager& slotIrManager) noexcept
{
    for (int slotIndex = 0; slotIndex < maxPairSlots; ++slotIndex)
        applySlotIrSnapshot(slotIndex, slotIrManager.getSlotSnapshot(slotIndex));
}

MchBinauralRenderer::RuntimeStats MchBinauralRenderer::render(const LayoutPresetDefinition& preset,
                                                              const std::array<const float*, maxRendererInputChannels>& inputChannels,
                                                              int inputChannelCount,
                                                              const std::array<PairRuntimeParameters, maxPairSlots>& pairParameters,
                                                              float globalWet,
                                                              float globalOutputGain,
                                                              float* outputL,
                                                              float* outputR,
                                                              int numSamples) noexcept
{
    RuntimeStats stats;

    if (outputL == nullptr || outputR == nullptr || numSamples <= 0 || numSamples > maxBlockSize)
        return stats;

    juce::FloatVectorOperations::clear(outputL, numSamples);
    juce::FloatVectorOperations::clear(outputR, numSamples);

    const auto visibleSlotCount = juce::jlimit(0, maxPairSlots, preset.visibleSlotCount);
    stats.visiblePairs = visibleSlotCount;
    stats.maxConvolutionPaths = visibleSlotCount * 4;

    bool soloActive = false;
    for (int slot = 0; slot < visibleSlotCount; ++slot)
    {
        const auto& pair = pairParameters[static_cast<size_t>(slot)];
        if (pair.enabled && !pair.muted && pair.solo)
        {
            soloActive = true;
            break;
        }
    }
    stats.soloActive = soloActive;

    const auto* zero = zeroInput.getReadPointer(0);
    auto* slotOutL = slotOutputScratch.getWritePointer(0);
    auto* slotOutR = slotOutputScratch.getWritePointer(1);

    for (int slot = 0; slot < visibleSlotCount; ++slot)
    {
        const auto& pair = pairParameters[static_cast<size_t>(slot)];
        if (!pair.enabled || pair.muted)
            continue;

        if (soloActive && !pair.solo)
            continue;

        const auto& slotDef = preset.slots[static_cast<size_t>(slot)];
        const int inputAIndex = slotDef.inputA - 1;
        const int inputBIndex = slotDef.inputB - 1;

        const bool hasInputA = inputAIndex >= 0 && inputAIndex < inputChannelCount
                               && inputChannels[static_cast<size_t>(inputAIndex)] != nullptr;
        const bool hasInputB = inputBIndex >= 0 && inputBIndex < inputChannelCount
                               && inputChannels[static_cast<size_t>(inputBIndex)] != nullptr;

        if (!hasInputA && !hasInputB)
        {
            ++stats.skippedPairs;
            continue;
        }

        const float* inputA = hasInputA ? inputChannels[static_cast<size_t>(inputAIndex)] : zero;
        const float* inputB = hasInputB ? inputChannels[static_cast<size_t>(inputBIndex)] : zero;

        const bool hasValidIR = pairProcessors[static_cast<size_t>(slot)].hasValidIR();
        // PRD fallback: when no IR is loaded for a slot, force dry pass-through regardless of
        // the wet/dry knob position. This ensures the user always hears audio in MCH mode.
        const float slotEffectiveWet = hasValidIR
                                           ? (juce::jlimit(0.0f, 1.0f, pair.pairWet) * juce::jlimit(0.0f, 1.0f, globalWet))
                                           : 0.0f;
        const float dryGain = 1.0f - slotEffectiveWet;
        const bool allowWetProcessing = slotEffectiveWet > minAudibleGain;
        const bool hasDryContribution = dryGain > minAudibleGain;
        const bool hasWetContribution = allowWetProcessing;

        if (!hasDryContribution && !hasWetContribution)
        {
            ++stats.skippedPairs;
            continue;
        }

        if (hasDryContribution)
            ++stats.activeDryPairs;
        if (hasWetContribution)
            ++stats.activeWetPairs;

        juce::FloatVectorOperations::clear(slotOutL, numSamples);
        juce::FloatVectorOperations::clear(slotOutR, numSamples);

        stats.activeConvolutionPaths += pairProcessors[static_cast<size_t>(slot)].process(
            inputA,
            inputB,
            slotOutL,
            slotOutR,
            numSamples,
            isCenterLfePair(slotDef),
            pair.pan,
            slotEffectiveWet,
            pair.gainLinear,
            allowWetProcessing);

        stats.slotOutputRmsL[static_cast<size_t>(slot)] = findBlockRms(slotOutL, numSamples);
        stats.slotOutputRmsR[static_cast<size_t>(slot)] = findBlockRms(slotOutR, numSamples);

        juce::FloatVectorOperations::add(outputL, slotOutL, numSamples);
        juce::FloatVectorOperations::add(outputR, slotOutR, numSamples);

        ++stats.activePairs;
    }

    const float outputGainAbs = std::abs(globalOutputGain);
    if (std::abs(outputGainAbs - 1.0f) > minAudibleGain)
    {
        for (int slot = 0; slot < visibleSlotCount; ++slot)
        {
            stats.slotOutputRmsL[static_cast<size_t>(slot)] *= outputGainAbs;
            stats.slotOutputRmsR[static_cast<size_t>(slot)] *= outputGainAbs;
        }
    }

    juce::FloatVectorOperations::multiply(outputL, globalOutputGain, numSamples);
    juce::FloatVectorOperations::multiply(outputR, globalOutputGain, numSamples);

    return stats;
}
} // namespace bsr::mch
