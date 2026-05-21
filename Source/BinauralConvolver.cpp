#include "BinauralConvolver.h"

void BinauralConvolver::MonoConvolver::prepare(double sampleRate, int maximumBlockSize) noexcept
{
    processSpec.sampleRate = sampleRate;
    processSpec.maximumBlockSize = static_cast<juce::uint32>(juce::jmax(1, maximumBlockSize));
    processSpec.numChannels = 1;
    hasProcessSpec = true;

    convolver.prepare(processSpec);
    inputBuffer.setSize(1, maximumBlockSize, false, false, true);
    outputBuffer.setSize(1, maximumBlockSize, false, false, true);
}

void BinauralConvolver::MonoConvolver::loadIR(const float* ir, int irNumSamples, double sampleRate) noexcept
{
    if (!ir || irNumSamples <= 0)
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

    reset();
}

void BinauralConvolver::MonoConvolver::process(const float* input, float* output, int numSamples) noexcept
{
    if (input == nullptr || output == nullptr || numSamples <= 0)
    {
        if (output != nullptr)
            juce::FloatVectorOperations::clear(output, numSamples);
        return;
    }

    auto* in = inputBuffer.getWritePointer(0);
    juce::FloatVectorOperations::copy(in, input, numSamples);

    auto* out = outputBuffer.getWritePointer(0);
    juce::FloatVectorOperations::clear(out, numSamples);

    const auto inBlock = juce::dsp::AudioBlock<const float>(inputBuffer).getSubBlock(0, static_cast<size_t>(numSamples));
    auto outBlock = juce::dsp::AudioBlock<float>(outputBuffer).getSubBlock(0, static_cast<size_t>(numSamples));
    juce::dsp::ProcessContextNonReplacing<float> context(inBlock, outBlock);
    convolver.process(context);

    juce::FloatVectorOperations::copy(output, out, numSamples);
}

void BinauralConvolver::MonoConvolver::reset() noexcept
{
    convolver.reset();
}

// BinauralConvolver Implementation
void BinauralConvolver::prepareToPlay(double newSampleRate, int maximumBlockSize) noexcept
{
    this->sampleRate = newSampleRate;
    this->maxBlockSize = maximumBlockSize;

    // Allocate temporary wet buffers
    wetL.setSize(1, maximumBlockSize, true, false, true);
    wetR.setSize(1, maximumBlockSize, true, false, true);
    pathScratch.setSize(4, maximumBlockSize, true, false, true);

    // Prepare all convolvers
    for (auto& conv : convolvers)
    {
        conv.prepare(sampleRate, maximumBlockSize);
    }

    latencySamples = 0;
    isInitialized = true;
}

void BinauralConvolver::loadIR(const float* hLL, const float* hLR, const float* hRL, const float* hRR,
                                int irLength) noexcept
{
    if (!hLL || !hLR || !hRL || !hRR || irLength <= 0)
        return;

    // Load each IR into corresponding convolver
    const float* irPtrs[] = {hLL, hLR, hRL, hRR};

    for (int i = 0; i < 4; ++i)
    {
        convolvers[i].loadIR(irPtrs[i], irLength, sampleRate);
    }

    latencySamples = 0;
    for (auto& conv : convolvers)
        latencySamples = juce::jmax(latencySamples, static_cast<int>(conv.convolver.getLatency()));

    hasValidIR = true;
}

void BinauralConvolver::process(const float* inputL, const float* inputR, float* outputL, float* outputR,
                                 int numSamples, float wetDryMix, float outputGain, bool bypass) noexcept
{
    if (bypass)
    {
        // Bypass: output = input * outputGain
        juce::FloatVectorOperations::copy(outputL, inputL, numSamples);
        juce::FloatVectorOperations::copy(outputR, inputR, numSamples);
        juce::FloatVectorOperations::multiply(outputL, outputGain, numSamples);
        juce::FloatVectorOperations::multiply(outputR, outputGain, numSamples);
        return;
    }

    if (!isInitialized || !hasValidIR)
    {
        // No IR loaded: pass dry only with gain applied
        juce::FloatVectorOperations::copy(outputL, inputL, numSamples);
        juce::FloatVectorOperations::copy(outputR, inputR, numSamples);
        juce::FloatVectorOperations::multiply(outputL, outputGain, numSamples);
        juce::FloatVectorOperations::multiply(outputR, outputGain, numSamples);
        return;
    }

    auto* wetLPtr = wetL.getWritePointer(0);
    auto* wetRPtr = wetR.getWritePointer(0);

    auto* tempOutputLL = pathScratch.getWritePointer(0);
    auto* tempOutputLR = pathScratch.getWritePointer(1);
    auto* tempOutputRL = pathScratch.getWritePointer(2);
    auto* tempOutputRR = pathScratch.getWritePointer(3);

    // Process LL: input L -> output L
    convolvers[0].process(inputL, tempOutputLL, numSamples);
    juce::FloatVectorOperations::copy(wetLPtr, tempOutputLL, numSamples);

    // Process LR: input L -> output R
    convolvers[1].process(inputL, tempOutputLR, numSamples);
    juce::FloatVectorOperations::copy(wetRPtr, tempOutputLR, numSamples);

    // Process RL: input R -> output L
    convolvers[2].process(inputR, tempOutputRL, numSamples);
    juce::FloatVectorOperations::add(wetLPtr, tempOutputRL, numSamples);

    // Process RR: input R -> output R
    convolvers[3].process(inputR, tempOutputRR, numSamples);
    juce::FloatVectorOperations::add(wetRPtr, tempOutputRR, numSamples);

    // Apply wet/dry mix and output gain
    // output = outputGain * ((1 - wet) * input + wet * wetOutput)
    float dryMix = 1.0f - wetDryMix;

    for (int i = 0; i < numSamples; ++i)
    {
        outputL[i] = outputGain * (dryMix * inputL[i] + wetDryMix * wetLPtr[i]);
        outputR[i] = outputGain * (dryMix * inputR[i] + wetDryMix * wetRPtr[i]);
    }
}

void BinauralConvolver::releaseResources() noexcept
{
    for (auto& conv : convolvers)
    {
        conv.reset();
    }
    wetL.setSize(0, 0, true, false, true);
    wetR.setSize(0, 0, true, false, true);
    pathScratch.setSize(0, 0, true, false, true);
    latencySamples = 0;
    hasValidIR = false;
    isInitialized = false;
}
