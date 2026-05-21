#pragma once

#include <JuceHeader.h>

#include "../Source/Parameters.h"

class ParameterTests : public juce::UnitTest
{
public:
    ParameterTests() : juce::UnitTest("Parameters", "State") {}

    void runTest() override
    {
        beginTest("Parameter defaults are correct");
        testDefaults();

        beginTest("Parameter ranges are correct");
        testRanges();

        beginTest("State save and restore preserves values");
        testStateRoundTrip();
    }

private:
    class TestProcessor : public juce::AudioProcessor
    {
    public:
        TestProcessor() : juce::AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                                               .withOutput("Output", juce::AudioChannelSet::stereo(), true))
        {
        }

        const juce::String getName() const override { return "ParameterTests"; }
        void prepareToPlay(double, int) override {}
        void releaseResources() override {}
        bool isBusesLayoutSupported(const BusesLayout&) const override { return true; }
        void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
        juce::AudioProcessorEditor* createEditor() override { return nullptr; }
        bool hasEditor() const override { return false; }
        bool acceptsMidi() const override { return false; }
        bool producesMidi() const override { return false; }
        bool isMidiEffect() const override { return false; }
        double getTailLengthSeconds() const override { return 0.0; }
        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram(int) override {}
        const juce::String getProgramName(int) override { return {}; }
        void changeProgramName(int, const juce::String&) override {}
        void getStateInformation(juce::MemoryBlock&) override {}
        void setStateInformation(const void*, int) override {}
    };

    juce::AudioProcessorValueTreeState createState(TestProcessor& processor)
    {
        return { processor, nullptr, "Parameters", bsr::parameters::createParameterLayout() };
    }

    void testDefaults()
    {
        TestProcessor processor;
        auto state = createState(processor);

        expect(std::abs(*state.getRawParameterValue(bsr::parameters::renderMode) - 0.0f) < 0.001f, "Mode should default to Stereo");
        expect(std::abs(*state.getRawParameterValue(bsr::parameters::layoutPreset) - 0.0f) < 0.001f, "Layout should default to Stereo");
        expect(std::abs(*state.getRawParameterValue(bsr::parameters::wetDry) - 100.0f) < 0.001f, "Wet/Dry should default to 100%");
        expect(std::abs(*state.getRawParameterValue(bsr::parameters::outputGainDb) - 0.0f) < 0.001f, "Output Gain should default to 0 dB");
        expect(std::abs(*state.getRawParameterValue(bsr::parameters::inputPan) - 0.0f) < 0.001f, "Input Pan should default to center");
        expect(std::abs(*state.getRawParameterValue(bsr::parameters::bypass) - 0.0f) < 0.001f, "Bypass should default to off");
        expect(std::abs(*state.getRawParameterValue(bsr::parameters::irPath) - 0.0f) < 0.001f, "IR Path bool should default to false");
        expect(std::abs(*state.getRawParameterValue(bsr::parameters::pairEnable(0)) - 1.0f) < 0.001f, "Pair 1 enable should default to on");
        expect(std::abs(*state.getRawParameterValue(bsr::parameters::pairMute(0)) - 0.0f) < 0.001f, "Pair 1 mute should default to off");
        expect(std::abs(*state.getRawParameterValue(bsr::parameters::pairSolo(0)) - 0.0f) < 0.001f, "Pair 1 solo should default to off");
        expect(std::abs(*state.getRawParameterValue(bsr::parameters::pairGainDb(0)) - 0.0f) < 0.001f, "Pair 1 gain should default to 0 dB");
        expect(std::abs(*state.getRawParameterValue(bsr::parameters::pairPan(0)) - 0.0f) < 0.001f, "Pair 1 pan should default to center");
        expect(std::abs(*state.getRawParameterValue(bsr::parameters::pairWetDry(0)) - 100.0f) < 0.001f, "Pair 1 wet/dry should default to 100%");
    }

    void testRanges()
    {
        TestProcessor processor;
        auto state = createState(processor);

        auto* wetDry = dynamic_cast<juce::AudioParameterFloat*>(state.getParameter(bsr::parameters::wetDry));
        auto* outputGain = dynamic_cast<juce::AudioParameterFloat*>(state.getParameter(bsr::parameters::outputGainDb));
        auto* inputPan = dynamic_cast<juce::AudioParameterFloat*>(state.getParameter(bsr::parameters::inputPan));
        auto* bypass = dynamic_cast<juce::AudioParameterBool*>(state.getParameter(bsr::parameters::bypass));
        auto* irPath = dynamic_cast<juce::AudioParameterBool*>(state.getParameter(bsr::parameters::irPath));
        auto* mode = dynamic_cast<juce::AudioParameterChoice*>(state.getParameter(bsr::parameters::renderMode));
        auto* layout = dynamic_cast<juce::AudioParameterChoice*>(state.getParameter(bsr::parameters::layoutPreset));
        auto* pairGain = dynamic_cast<juce::AudioParameterFloat*>(state.getParameter(bsr::parameters::pairGainDb(0)));
        auto* pairPan = dynamic_cast<juce::AudioParameterFloat*>(state.getParameter(bsr::parameters::pairPan(0)));
        auto* pairWetDry = dynamic_cast<juce::AudioParameterFloat*>(state.getParameter(bsr::parameters::pairWetDry(0)));

        expect(wetDry != nullptr, "Wet/Dry parameter should be a float parameter");
        expect(outputGain != nullptr, "Output Gain parameter should be a float parameter");
        expect(inputPan != nullptr, "Input Pan parameter should be a float parameter");
        expect(bypass != nullptr, "Bypass parameter should be a bool parameter");
        expect(irPath != nullptr, "IR Path parameter should be a bool parameter");
        expect(mode != nullptr, "Mode parameter should be a choice parameter");
        expect(layout != nullptr, "Layout parameter should be a choice parameter");
        expect(pairGain != nullptr, "Pair gain should be a float parameter");
        expect(pairPan != nullptr, "Pair pan should be a float parameter");
        expect(pairWetDry != nullptr, "Pair wet/dry should be a float parameter");

        if (wetDry != nullptr)
        {
            expect(std::abs(wetDry->range.start - 0.0f) < 0.001f, "Wet/Dry range should start at 0%");
            expect(std::abs(wetDry->range.end - 100.0f) < 0.001f, "Wet/Dry range should end at 100%");
        }

        if (outputGain != nullptr)
        {
            expect(std::abs(outputGain->range.start - (-24.0f)) < 0.001f, "Output Gain range should start at -24 dB");
            expect(std::abs(outputGain->range.end - 12.0f) < 0.001f, "Output Gain range should end at 12 dB");
        }

        if (inputPan != nullptr)
        {
            expect(std::abs(inputPan->range.start - (-100.0f)) < 0.001f, "Input Pan range should start at -100%");
            expect(std::abs(inputPan->range.end - 100.0f) < 0.001f, "Input Pan range should end at 100%");
        }

        if (pairGain != nullptr)
        {
            expect(std::abs(pairGain->range.start - (-24.0f)) < 0.001f, "Pair gain range should start at -24 dB");
            expect(std::abs(pairGain->range.end - 12.0f) < 0.001f, "Pair gain range should end at 12 dB");
        }

        if (pairPan != nullptr)
        {
            expect(std::abs(pairPan->range.start - (-100.0f)) < 0.001f, "Pair pan range should start at -100%");
            expect(std::abs(pairPan->range.end - 100.0f) < 0.001f, "Pair pan range should end at 100%");
        }

        if (pairWetDry != nullptr)
        {
            expect(std::abs(pairWetDry->range.start - 0.0f) < 0.001f, "Pair wet/dry range should start at 0%");
            expect(std::abs(pairWetDry->range.end - 100.0f) < 0.001f, "Pair wet/dry range should end at 100%");
        }
    }

    void testStateRoundTrip()
    {
        TestProcessor sourceProcessor;
        auto sourceState = createState(sourceProcessor);

        auto setFloatValue = [](juce::AudioProcessorValueTreeState& state, const juce::String& paramId, float actualValue)
        {
            if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*>(state.getParameter(paramId)))
                parameter->setValueNotifyingHost(parameter->convertTo0to1(actualValue));
        };

        setFloatValue(sourceState, bsr::parameters::wetDry, 42.5f);
        setFloatValue(sourceState, bsr::parameters::outputGainDb, -6.0f);
        setFloatValue(sourceState, bsr::parameters::inputPan, -25.0f);
        setFloatValue(sourceState, bsr::parameters::bypass, 1.0f);
        setFloatValue(sourceState, bsr::parameters::renderMode, 1.0f);
        setFloatValue(sourceState, bsr::parameters::layoutPreset, 6.0f);
        setFloatValue(sourceState, bsr::parameters::pairEnable(0), 0.0f);
        setFloatValue(sourceState, bsr::parameters::pairGainDb(0), -3.5f);
        setFloatValue(sourceState, bsr::parameters::pairPan(0), 30.0f);
        setFloatValue(sourceState, bsr::parameters::pairWetDry(0), 65.0f);

        auto savedState = sourceState.copyState();

        TestProcessor restoredProcessor;
        auto restoredState = createState(restoredProcessor);
        restoredState.replaceState(savedState);

        expect(std::abs(*restoredState.getRawParameterValue(bsr::parameters::wetDry) - 42.5f) < 0.001f, "Wet/Dry should survive state restore");
        expect(std::abs(*restoredState.getRawParameterValue(bsr::parameters::outputGainDb) - (-6.0f)) < 0.001f, "Output Gain should survive state restore");
        expect(std::abs(*restoredState.getRawParameterValue(bsr::parameters::inputPan) - (-25.0f)) < 0.001f, "Input Pan should survive state restore");
        expect(std::abs(*restoredState.getRawParameterValue(bsr::parameters::bypass) - 1.0f) < 0.001f, "Bypass should survive state restore");
        expect(std::abs(*restoredState.getRawParameterValue(bsr::parameters::renderMode) - 1.0f) < 0.001f, "Mode should survive state restore");
        expect(std::abs(*restoredState.getRawParameterValue(bsr::parameters::layoutPreset) - 6.0f) < 0.001f, "Layout should survive state restore");
        expect(std::abs(*restoredState.getRawParameterValue(bsr::parameters::pairEnable(0)) - 0.0f) < 0.001f, "Pair enable should survive state restore");
        expect(std::abs(*restoredState.getRawParameterValue(bsr::parameters::pairGainDb(0)) - (-3.5f)) < 0.001f, "Pair gain should survive state restore");
        expect(std::abs(*restoredState.getRawParameterValue(bsr::parameters::pairPan(0)) - 30.0f) < 0.001f, "Pair pan should survive state restore");
        expect(std::abs(*restoredState.getRawParameterValue(bsr::parameters::pairWetDry(0)) - 65.0f) < 0.001f, "Pair wet/dry should survive state restore");
    }
};

static ParameterTests parameterTests;