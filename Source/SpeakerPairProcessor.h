#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <array>

namespace bsr::mch
{
class SpeakerPairProcessor
{
public:
    SpeakerPairProcessor() = default;

    void prepareToPlay(double sampleRate, int maximumBlockSize) noexcept;
    void releaseResources() noexcept;

    bool loadIR(const juce::AudioBuffer<float>& irBuffer) noexcept;
    void clearIR() noexcept;
    bool hasValidIR() const noexcept
    {
        const int activeIndex = activeIrStateIndex.load(std::memory_order_acquire);
        return irStates[static_cast<size_t>(activeIndex)].hasLoadedIr;
    }

    int process(const float* inputA,
                const float* inputB,
                float* outputL,
                float* outputR,
                int numSamples,
                bool isCenterLfePair,
                float pan,
                float effectiveWet,
                float pairGainLinear,
                bool allowWetProcessing) noexcept;

private:
    struct MonoConvolver
    {
        juce::dsp::Convolution convolver { juce::dsp::Convolution::Latency { 0 } };
        juce::dsp::ProcessSpec processSpec {};
        bool hasProcessSpec = false;
        juce::AudioBuffer<float> inputBuffer;
        juce::AudioBuffer<float> outputBuffer;
        juce::AudioBuffer<float> irChannelBuffer;

        void prepare(double sampleRate, int maximumBlockSize) noexcept;
        void loadIR(const float* ir, int irNumSamples, double sampleRate) noexcept;
        void process(const float* input, float* output, int numSamples) noexcept;
        void reset() noexcept;
    };

    struct IrConvolverState
    {
        std::array<MonoConvolver, 4> convolvers;
        bool hasLoadedIr = false;
        juce::uint64 version = 0;
    };

    std::array<IrConvolverState, 2> irStates;
    std::atomic<int> activeIrStateIndex { 0 };
    juce::AudioBuffer<float> wetBuffer;
    juce::AudioBuffer<float> pathScratch;
    juce::AudioBuffer<float> pannedInputs;
    int maxBlockSize = 0;
    double sampleRate = 0.0;
    bool prepared = false;
};
} // namespace bsr::mch
