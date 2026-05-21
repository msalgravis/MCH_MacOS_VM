#pragma once

#include <JuceHeader.h>
#include <cmath>

#include "../Source/BinauralConvolver.h"

class BinauralConvolverTests : public juce::UnitTest
{
public:
    BinauralConvolverTests() : juce::UnitTest("BinauralConvolver", "DSP") {}

    void runTest() override
    {
        beginTest("LL channel routes left input to left output");
        testLLChannel();

        beginTest("LR channel routes left input to right output");
        testLRChannel();

        beginTest("RL channel routes right input to left output");
        testRLChannel();

        beginTest("RR channel routes right input to right output");
        testRRChannel();

        beginTest("Wet/Dry 0 percent returns dry input");
        testWetDry0Percent();

        beginTest("Wet/Dry 100 percent returns wet output");
        testWetDry100Percent();

        beginTest("Output gain applies to wet output");
        testOutputGainApplication();

        beginTest("Bypass returns dry input");
        testBypass();
    }

private:
    static constexpr float EPSILON = 1e-5f;

    void expectNear(float actual, float expected, const juce::String& message)
    {
        expect(std::abs(actual - expected) < EPSILON, message + " Expected: " + juce::String(expected) + ", Actual: " + juce::String(actual));
    }

    void expectSilent(float sample, const juce::String& message)
    {
        expect(std::abs(sample) < EPSILON, message + " Actual: " + juce::String(sample));
    }

    void testLLChannel()
    {
        const int irLength = 512;
        std::vector<float> hLL(irLength, 0.0f), hLR(irLength, 0.0f), hRL(irLength, 0.0f), hRR(irLength, 0.0f);
        hLL[0] = 1.0f;

        BinauralConvolver convolver;
        convolver.prepareToPlay(48000.0, 256);
        convolver.loadIR(hLL.data(), hLR.data(), hRL.data(), hRR.data(), irLength);

        const int blockSize = 256;
        std::vector<float> inputL(blockSize, 0.0f), inputR(blockSize, 0.0f);
        std::vector<float> outputL(blockSize, 0.0f), outputR(blockSize, 0.0f);
        inputL[0] = 1.0f;

        convolver.process(inputL.data(), inputR.data(), outputL.data(), outputR.data(), blockSize, 1.0f, 1.0f, false);

        expectNear(outputL[0], 1.0f, "LL output should contain the input impulse");
        for (int i = 1; i < blockSize; ++i)
        {
            expectSilent(outputL[i], "LL tail should be silent for an impulse IR");
            expectSilent(outputR[i], "Right output should remain silent in LL-only routing");
        }
    }

    void testLRChannel()
    {
        const int irLength = 512;
        std::vector<float> hLL(irLength, 0.0f), hLR(irLength, 0.0f), hRL(irLength, 0.0f), hRR(irLength, 0.0f);
        hLR[0] = 1.0f;

        BinauralConvolver convolver;
        convolver.prepareToPlay(48000.0, 256);
        convolver.loadIR(hLL.data(), hLR.data(), hRL.data(), hRR.data(), irLength);

        const int blockSize = 256;
        std::vector<float> inputL(blockSize, 0.0f), inputR(blockSize, 0.0f);
        std::vector<float> outputL(blockSize, 0.0f), outputR(blockSize, 0.0f);
        inputL[0] = 1.0f;

        convolver.process(inputL.data(), inputR.data(), outputL.data(), outputR.data(), blockSize, 1.0f, 1.0f, false);

        expectNear(outputR[0], 1.0f, "LR output should contain the left input impulse");
        for (int i = 1; i < blockSize; ++i)
        {
            expectSilent(outputL[i], "Left output should remain silent in LR-only routing");
            expectSilent(outputR[i], "LR tail should be silent for an impulse IR");
        }
    }

    void testRLChannel()
    {
        const int irLength = 512;
        std::vector<float> hLL(irLength, 0.0f), hLR(irLength, 0.0f), hRL(irLength, 0.0f), hRR(irLength, 0.0f);
        hRL[0] = 1.0f;

        BinauralConvolver convolver;
        convolver.prepareToPlay(48000.0, 256);
        convolver.loadIR(hLL.data(), hLR.data(), hRL.data(), hRR.data(), irLength);

        const int blockSize = 256;
        std::vector<float> inputL(blockSize, 0.0f), inputR(blockSize, 0.0f);
        std::vector<float> outputL(blockSize, 0.0f), outputR(blockSize, 0.0f);
        inputR[0] = 1.0f;

        convolver.process(inputL.data(), inputR.data(), outputL.data(), outputR.data(), blockSize, 1.0f, 1.0f, false);

        expectNear(outputL[0], 1.0f, "RL output should contain the right input impulse");
        for (int i = 1; i < blockSize; ++i)
        {
            expectSilent(outputL[i], "RL tail should be silent for an impulse IR");
            expectSilent(outputR[i], "Right output should remain silent in RL-only routing");
        }
    }

    void testRRChannel()
    {
        const int irLength = 512;
        std::vector<float> hLL(irLength, 0.0f), hLR(irLength, 0.0f), hRL(irLength, 0.0f), hRR(irLength, 0.0f);
        hRR[0] = 1.0f;

        BinauralConvolver convolver;
        convolver.prepareToPlay(48000.0, 256);
        convolver.loadIR(hLL.data(), hLR.data(), hRL.data(), hRR.data(), irLength);

        const int blockSize = 256;
        std::vector<float> inputL(blockSize, 0.0f), inputR(blockSize, 0.0f);
        std::vector<float> outputL(blockSize, 0.0f), outputR(blockSize, 0.0f);
        inputR[0] = 1.0f;

        convolver.process(inputL.data(), inputR.data(), outputL.data(), outputR.data(), blockSize, 1.0f, 1.0f, false);

        expectNear(outputR[0], 1.0f, "RR output should contain the input impulse");
        for (int i = 1; i < blockSize; ++i)
        {
            expectSilent(outputL[i], "Left output should remain silent in RR-only routing");
            expectSilent(outputR[i], "RR tail should be silent for an impulse IR");
        }
    }

    void testWetDry0Percent()
    {
        const int irLength = 512;
        std::vector<float> hLL(irLength, 0.0f), hLR(irLength, 0.0f), hRL(irLength, 0.0f), hRR(irLength, 0.0f);
        hLL[0] = 2.0f;

        BinauralConvolver convolver;
        convolver.prepareToPlay(48000.0, 256);
        convolver.loadIR(hLL.data(), hLR.data(), hRL.data(), hRR.data(), irLength);

        const int blockSize = 256;
        std::vector<float> inputL(blockSize, 0.5f), inputR(blockSize, -0.5f);
        std::vector<float> outputL(blockSize, 0.0f), outputR(blockSize, 0.0f);

        convolver.process(inputL.data(), inputR.data(), outputL.data(), outputR.data(), blockSize, 0.0f, 1.0f, false);

        for (int i = 0; i < blockSize; ++i)
        {
            expectNear(outputL[i], inputL[i], "Wet/Dry 0% should preserve the dry left sample");
            expectNear(outputR[i], inputR[i], "Wet/Dry 0% should preserve the dry right sample");
        }
    }

    void testWetDry100Percent()
    {
        const int irLength = 512;
        std::vector<float> hLL(irLength, 0.0f), hLR(irLength, 0.0f), hRL(irLength, 0.0f), hRR(irLength, 0.0f);
        hLL[0] = 1.0f;
        hLR[0] = 0.0f;
        hRL[0] = 0.0f;
        hRR[0] = 1.0f;

        BinauralConvolver convolver;
        convolver.prepareToPlay(48000.0, 256);
        convolver.loadIR(hLL.data(), hLR.data(), hRL.data(), hRR.data(), irLength);

        const int blockSize = 256;
        std::vector<float> inputL(blockSize, 0.5f), inputR(blockSize, -0.3f);
        std::vector<float> outputL(blockSize, 0.0f), outputR(blockSize, 0.0f);

        convolver.process(inputL.data(), inputR.data(), outputL.data(), outputR.data(), blockSize, 1.0f, 1.0f, false);

        expectNear(outputL[0], 0.5f, "Wet left output should match the LL impulse response");
        expectNear(outputR[0], -0.3f, "Wet right output should match the RR impulse response");
    }

    void testOutputGainApplication()
    {
        const int irLength = 512;
        std::vector<float> hLL(irLength, 0.0f), hLR(irLength, 0.0f), hRL(irLength, 0.0f), hRR(irLength, 0.0f);
        hLL[0] = 1.0f;

        BinauralConvolver convolver;
        convolver.prepareToPlay(48000.0, 256);
        convolver.loadIR(hLL.data(), hLR.data(), hRL.data(), hRR.data(), irLength);

        const int blockSize = 256;
        std::vector<float> inputL(blockSize, 1.0f), inputR(blockSize, 0.0f);
        std::vector<float> outputL(blockSize, 0.0f), outputR(blockSize, 0.0f);

        float gain = 0.5f;
        convolver.process(inputL.data(), inputR.data(), outputL.data(), outputR.data(), blockSize, 1.0f, gain, false);

        expectNear(outputL[0], 0.5f, "Output gain should scale the wet left output");
        expectNear(outputR[0], 0.0f, "Output gain should preserve silence on untouched channels");
    }

    void testBypass()
    {
        const int irLength = 512;
        std::vector<float> hLL(irLength, 0.0f), hLR(irLength, 0.0f), hRL(irLength, 0.0f), hRR(irLength, 0.0f);
        hLL[0] = 10.0f;

        BinauralConvolver convolver;
        convolver.prepareToPlay(48000.0, 256);
        convolver.loadIR(hLL.data(), hLR.data(), hRL.data(), hRR.data(), irLength);

        const int blockSize = 256;
        std::vector<float> inputL(blockSize, 0.2f), inputR(blockSize, -0.1f);
        std::vector<float> outputL(blockSize, 0.0f), outputR(blockSize, 0.0f);

        convolver.process(inputL.data(), inputR.data(), outputL.data(), outputR.data(), blockSize, 1.0f, 1.0f, true);

        for (int i = 0; i < blockSize; ++i)
        {
            expectNear(outputL[i], inputL[i], "Bypass should preserve the dry left sample");
            expectNear(outputR[i], inputR[i], "Bypass should preserve the dry right sample");
        }
    }
};

static BinauralConvolverTests binauralConvolverTests;
