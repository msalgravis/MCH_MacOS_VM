#include "SpeakerPairProcessor.h"

namespace bsr::mch
{
namespace
{
constexpr float panMuteThreshold = 1.0e-6f;
constexpr float minAudibleGain = 1.0e-6f;
}

void SpeakerPairProcessor::MonoConvolver::prepare(double sampleRate, int maximumBlockSize) noexcept
{
    processSpec.sampleRate = sampleRate;
    processSpec.maximumBlockSize = static_cast<juce::uint32>(juce::jmax(1, maximumBlockSize));
    processSpec.numChannels = 1;
    hasProcessSpec = true;

    convolver.prepare(processSpec);
    inputBuffer.setSize(1, maximumBlockSize, false, false, true);
    outputBuffer.setSize(1, maximumBlockSize, false, false, true);
}

void SpeakerPairProcessor::MonoConvolver::loadIR(const float* ir, int irNumSamples, double sampleRate) noexcept
{
    if (ir == nullptr || irNumSamples <= 0)
        return;

    irChannelBuffer.setSize(1, irNumSamples, false, false, true);
    juce::FloatVectorOperations::copy(irChannelBuffer.getWritePointer(0), ir, irNumSamples);

    convolver.loadImpulseResponse(std::move(irChannelBuffer),
                                  sampleRate,
                                  juce::dsp::Convolution::Stereo::no,
                                  juce::dsp::Convolution::Trim::no,
                                  juce::dsp::Convolution::Normalise::no);

    if (hasProcessSpec)
        convolver.prepare(processSpec);

    convolver.reset();
}

void SpeakerPairProcessor::MonoConvolver::process(const float* input, float* output, int numSamples) noexcept
{
    if (output == nullptr || numSamples <= 0)
        return;

    if (input == nullptr)
    {
        juce::FloatVectorOperations::clear(output, numSamples);
        return;
    }

    auto* in = inputBuffer.getWritePointer(0);
    auto* out = outputBuffer.getWritePointer(0);

    juce::FloatVectorOperations::copy(in, input, numSamples);
    juce::FloatVectorOperations::clear(out, numSamples);

    const auto inBlock = juce::dsp::AudioBlock<const float>(inputBuffer).getSubBlock(0, static_cast<size_t>(numSamples));
    auto outBlock = juce::dsp::AudioBlock<float>(outputBuffer).getSubBlock(0, static_cast<size_t>(numSamples));
    juce::dsp::ProcessContextNonReplacing<float> context(inBlock, outBlock);
    convolver.process(context);

    juce::FloatVectorOperations::copy(output, out, numSamples);
}

void SpeakerPairProcessor::MonoConvolver::reset() noexcept
{
    convolver.reset();
}

void SpeakerPairProcessor::prepareToPlay(double newSampleRate, int maximumBlockSize) noexcept
{
    sampleRate = newSampleRate;
    maxBlockSize = juce::jmax(1, maximumBlockSize);

    wetBuffer.setSize(2, maxBlockSize, true, false, true);
    pathScratch.setSize(2, maxBlockSize, true, false, true);
    pannedInputs.setSize(2, maxBlockSize, true, false, true);

    for (auto& state : irStates)
    {
        for (auto& convolver : state.convolvers)
            convolver.prepare(sampleRate, maxBlockSize);

        state.hasLoadedIr = false;
        state.version = 0;
    }

    activeIrStateIndex.store(0, std::memory_order_release);

    prepared = true;
}

void SpeakerPairProcessor::releaseResources() noexcept
{
    for (auto& state : irStates)
    {
        for (auto& convolver : state.convolvers)
            convolver.reset();

        state.hasLoadedIr = false;
        state.version = 0;
    }

    wetBuffer.setSize(0, 0, true, false, true);
    pathScratch.setSize(0, 0, true, false, true);
    pannedInputs.setSize(0, 0, true, false, true);

    prepared = false;
    activeIrStateIndex.store(0, std::memory_order_release);
    maxBlockSize = 0;
}

bool SpeakerPairProcessor::loadIR(const juce::AudioBuffer<float>& irBuffer) noexcept
{
    if (!prepared)
        return false;

    if (irBuffer.getNumChannels() != 4 || irBuffer.getNumSamples() <= 0)
        return false;

    const int currentActiveIndex = activeIrStateIndex.load(std::memory_order_acquire);
    const int pendingIndex = 1 - currentActiveIndex;
    auto& pendingState = irStates[static_cast<size_t>(pendingIndex)];

    for (int channel = 0; channel < 4; ++channel)
        pendingState.convolvers[static_cast<size_t>(channel)].loadIR(irBuffer.getReadPointer(channel),
                                                                     irBuffer.getNumSamples(),
                                                                     sampleRate);

    pendingState.hasLoadedIr = true;
    pendingState.version += 1;
    activeIrStateIndex.store(pendingIndex, std::memory_order_release);
    return true;
}

void SpeakerPairProcessor::clearIR() noexcept
{
    const int currentActiveIndex = activeIrStateIndex.load(std::memory_order_acquire);
    const int pendingIndex = 1 - currentActiveIndex;
    auto& pendingState = irStates[static_cast<size_t>(pendingIndex)];

    for (auto& convolver : pendingState.convolvers)
        convolver.reset();

    pendingState.hasLoadedIr = false;
    pendingState.version += 1;
    activeIrStateIndex.store(pendingIndex, std::memory_order_release);
}

int SpeakerPairProcessor::process(const float* inputA,
                                  const float* inputB,
                                  float* outputL,
                                  float* outputR,
                                  int numSamples,
                                  bool isCenterLfePair,
                                  float pan,
                                  float effectiveWet,
                                  float pairGainLinear,
                                  bool allowWetProcessing) noexcept
{
    if (!prepared || outputL == nullptr || outputR == nullptr || numSamples <= 0 || numSamples > maxBlockSize)
        return 0;

    const int activeIndex = activeIrStateIndex.load(std::memory_order_acquire);
    auto& activeState = irStates[static_cast<size_t>(activeIndex)];

    if (std::abs(pairGainLinear) <= minAudibleGain)
        return 0;

    const float clampedPan = juce::jlimit(-1.0f, 1.0f, pan);
    const float wet = juce::jlimit(0.0f, 1.0f, effectiveWet);
    const float dry = 1.0f - wet;

    const float panGainA = clampedPan > 0.0f ? (1.0f - clampedPan) : 1.0f;
    const float panGainB = clampedPan < 0.0f ? (1.0f + clampedPan) : 1.0f;

    const bool useDryPath = dry > minAudibleGain;
    const bool useAForWet = allowWetProcessing && activeState.hasLoadedIr && wet > minAudibleGain
                            && panGainA > panMuteThreshold && inputA != nullptr;
    const bool useBForWet = allowWetProcessing && activeState.hasLoadedIr && wet > minAudibleGain
                            && panGainB > panMuteThreshold && inputB != nullptr;
    const bool useWetPath = useAForWet || useBForWet;

    if (!useDryPath && !useWetPath)
        return 0;

    auto* wetL = wetBuffer.getWritePointer(0);
    auto* wetR = wetBuffer.getWritePointer(1);
    int activeConvolutionPaths = 0;

    if (useWetPath)
    {
        auto* pannedA = pannedInputs.getWritePointer(0);
        auto* pannedB = pannedInputs.getWritePointer(1);
        auto* scratchL = pathScratch.getWritePointer(0);
        auto* scratchR = pathScratch.getWritePointer(1);
        bool hasWetL = false;
        bool hasWetR = false;

        const float* wetInputA = inputA;
        const float* wetInputB = inputB;

        if (useAForWet)
        {
            if (std::abs(panGainA - 1.0f) > panMuteThreshold)
            {
                juce::FloatVectorOperations::copy(pannedA, inputA, numSamples);
                juce::FloatVectorOperations::multiply(pannedA, panGainA, numSamples);
                wetInputA = pannedA;
            }

            activeState.convolvers[0].process(wetInputA, scratchL, numSamples);
            activeState.convolvers[1].process(wetInputA, scratchR, numSamples);
            juce::FloatVectorOperations::copy(wetL, scratchL, numSamples);
            juce::FloatVectorOperations::copy(wetR, scratchR, numSamples);
            hasWetL = true;
            hasWetR = true;
            activeConvolutionPaths += 2;
        }

        if (useBForWet)
        {
            if (std::abs(panGainB - 1.0f) > panMuteThreshold)
            {
                juce::FloatVectorOperations::copy(pannedB, inputB, numSamples);
                juce::FloatVectorOperations::multiply(pannedB, panGainB, numSamples);
                wetInputB = pannedB;
            }

            activeState.convolvers[2].process(wetInputB, scratchL, numSamples);
            activeState.convolvers[3].process(wetInputB, scratchR, numSamples);

            if (hasWetL)
                juce::FloatVectorOperations::add(wetL, scratchL, numSamples);
            else
                juce::FloatVectorOperations::copy(wetL, scratchL, numSamples);

            if (hasWetR)
                juce::FloatVectorOperations::add(wetR, scratchR, numSamples);
            else
                juce::FloatVectorOperations::copy(wetR, scratchR, numSamples);

            activeConvolutionPaths += 2;
        }
    }

    const float wetGain = wet * pairGainLinear;
    const float dryGain = dry * pairGainLinear;

    if (!useDryPath)
    {
        if (std::abs(wetGain - 1.0f) <= minAudibleGain)
        {
            juce::FloatVectorOperations::add(outputL, wetL, numSamples);
            juce::FloatVectorOperations::add(outputR, wetR, numSamples);
        }
        else
        {
            juce::FloatVectorOperations::addWithMultiply(outputL, wetL, wetGain, numSamples);
            juce::FloatVectorOperations::addWithMultiply(outputR, wetR, wetGain, numSamples);
        }

        return activeConvolutionPaths;
    }

    if (!useWetPath)
    {
        if (!isCenterLfePair)
        {
            if (inputA != nullptr)
                juce::FloatVectorOperations::addWithMultiply(outputL, inputA, dryGain, numSamples);
            if (inputB != nullptr)
                juce::FloatVectorOperations::addWithMultiply(outputR, inputB, dryGain, numSamples);
            return activeConvolutionPaths;
        }

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float a = inputA != nullptr ? inputA[sample] : 0.0f;
            const float b = inputB != nullptr ? inputB[sample] : 0.0f;
            const float cAndLfe = 0.5f * (a + b);
            const float scaled = cAndLfe * dryGain;
            outputL[sample] += scaled;
            outputR[sample] += scaled;
        }

        return activeConvolutionPaths;
    }

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float a = inputA != nullptr ? inputA[sample] : 0.0f;
        const float b = inputB != nullptr ? inputB[sample] : 0.0f;

        float dryL = 0.0f;
        float dryR = 0.0f;
        if (isCenterLfePair)
        {
            const float cAndLfe = 0.5f * (a + b);
            dryL = cAndLfe;
            dryR = cAndLfe;
        }
        else
        {
            dryL = a;
            dryR = b;
        }

        outputL[sample] += dryGain * dryL + wetGain * wetL[sample];
        outputR[sample] += dryGain * dryR + wetGain * wetR[sample];
    }

    return activeConvolutionPaths;
}
} // namespace bsr::mch
