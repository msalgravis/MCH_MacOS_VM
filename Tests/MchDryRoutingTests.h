#pragma once

#include <JuceHeader.h>

#include "../Source/MchDryRouting.h"

class MchDryRoutingTests : public juce::UnitTest
{
public:
    MchDryRoutingTests() : juce::UnitTest("MCH dry routing", "MCH") {}

    void runTest() override
    {
        beginTest("Supported layout input counts match the preset list");
        testSupportedLayoutCounts();

        beginTest("Dry routing sums normal pairs and special-cases C/LFE");
        testDryRouting();
    }

private:
    void testSupportedLayoutCounts()
    {
        const std::array<int, 7> supportedCounts { 2, 6, 8, 10, 12, 14, 16 };

        for (const auto count : supportedCounts)
            expect(bsr::mch::isSupportedLayoutInputCount(count), "Expected preset count to be supported");

        expect(!bsr::mch::isSupportedLayoutInputCount(4), "Unsupported count should be rejected");
    }

    void testDryRouting()
    {
        const auto& preset = bsr::mch::getLayoutPresetByChoiceIndex(1); // 5.1
        const int numSamples = 8;

        std::array<std::vector<float>, 6> inputs;
        std::array<const float*, 6> inputPointers {};

        for (size_t channel = 0; channel < inputs.size(); ++channel)
        {
            inputs[channel].resize(static_cast<size_t>(numSamples));
            inputPointers[channel] = inputs[channel].data();

            for (int sample = 0; sample < numSamples; ++sample)
                inputs[channel][static_cast<size_t>(sample)] = static_cast<float>((channel + 1) * 10 + sample);
        }

        std::vector<float> outputL(static_cast<size_t>(numSamples), 0.0f);
        std::vector<float> outputR(static_cast<size_t>(numSamples), 0.0f);

        bsr::mch::renderDryRouting(preset,
                                   inputPointers.data(),
                                   static_cast<int>(inputPointers.size()),
                                   outputL.data(),
                                   outputR.data(),
                                   numSamples);

        expect(std::abs(outputL[0] - (10.0f + 30.0f + 40.0f + 50.0f)) < 0.001f,
               "Left output should sum L, C, LFE, and Ls");
        expect(std::abs(outputR[0] - (20.0f + 30.0f + 40.0f + 60.0f)) < 0.001f,
               "Right output should sum R, C, LFE, and Rs");

        const auto& stereoPreset = bsr::mch::getLayoutPresetByChoiceIndex(0);
        std::array<std::vector<float>, 2> stereoInputs;
        std::array<const float*, 2> stereoPointers {};

        for (size_t channel = 0; channel < stereoInputs.size(); ++channel)
        {
            stereoInputs[channel].resize(static_cast<size_t>(numSamples));
            stereoPointers[channel] = stereoInputs[channel].data();

            for (int sample = 0; sample < numSamples; ++sample)
                stereoInputs[channel][static_cast<size_t>(sample)] = static_cast<float>((channel + 1) * 100 + sample);
        }

        std::fill(outputL.begin(), outputL.end(), 0.0f);
        std::fill(outputR.begin(), outputR.end(), 0.0f);

        bsr::mch::renderDryRouting(stereoPreset,
                                   stereoPointers.data(),
                                   static_cast<int>(stereoPointers.size()),
                                   outputL.data(),
                                   outputR.data(),
                                   numSamples);

        expect(std::abs(outputL[0] - 100.0f) < 0.001f, "Stereo left dry routing should preserve channel 1");
        expect(std::abs(outputR[0] - 200.0f) < 0.001f, "Stereo right dry routing should preserve channel 2");
    }
};

static MchDryRoutingTests mchDryRoutingTests;