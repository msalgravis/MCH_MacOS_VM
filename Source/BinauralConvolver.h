#pragma once

#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>
#include <array>

class BinauralConvolver
{
public:
    BinauralConvolver() = default;
    ~BinauralConvolver() = default;

    /// @brief Prepare the convolver for audio processing
    /// @param sampleRate Host sample rate
    /// @param maximumBlockSize Maximum samples per processBlock call
    void prepareToPlay(double newSampleRate, int maximumBlockSize) noexcept;

    /// @brief Load IR data and initialize convolution paths
    /// @param hLL Channel 0 (left input -> left output) IR data
    /// @param hLR Channel 1 (left input -> right output) IR data
    /// @param hRL Channel 2 (right input -> left output) IR data
    /// @param hRR Channel 3 (right input -> right output) IR data
    /// @param irLength Number of samples in each IR channel
    void loadIR(const float* hLL, const float* hLR, const float* hRL, const float* hRR, int irLength) noexcept;

    /// @brief Process audio block with binaural convolution
    /// @param inputL Input left channel buffer
    /// @param inputR Input right channel buffer
    /// @param outputL Output left channel buffer
    /// @param outputR Output right channel buffer
    /// @param numSamples Number of samples to process
    /// @param wetDryMix Wet/dry mix: 0.0 = dry only, 1.0 = wet only
    /// @param outputGain Linear output gain multiplier
    /// @param bypass If true, output = input (dry pass-through)
    void process(const float* inputL, const float* inputR, float* outputL, float* outputR, int numSamples,
                 float wetDryMix, float outputGain, bool bypass) noexcept;

    /// @brief Get the latency in samples introduced by convolution
    /// @return Latency in samples (0 if not yet prepared)
    int getLatencySamples() const noexcept { return latencySamples; }

    /// @brief Release resources
    void releaseResources() noexcept;

private:
    struct MonoConvolver
    {
        juce::dsp::Convolution convolver { juce::dsp::Convolution::Latency { 0 } };

        void prepare(double sampleRate, int maximumBlockSize) noexcept;
        void loadIR(const float* ir, int irNumSamples, double sampleRate) noexcept;
        void process(const float* input, float* output, int numSamples) noexcept;
        void reset() noexcept;

    private:
        juce::dsp::ProcessSpec processSpec {};
        bool hasProcessSpec = false;
        juce::AudioBuffer<float> inputBuffer;
        juce::AudioBuffer<float> outputBuffer;
        juce::AudioBuffer<float> irChannelBuffer;
    };

    std::array<MonoConvolver, 4> convolvers; // LL, LR, RL, RR
    juce::AudioBuffer<float> wetL, wetR; // Temporary wet output buffers
    juce::AudioBuffer<float> pathScratch; // 4 channels for LL/LR/RL/RR outputs
    double sampleRate = 0.0;
    int maxBlockSize = 0;
    int latencySamples = 0;
    bool isInitialized = false;
    bool hasValidIR = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BinauralConvolver)
};
