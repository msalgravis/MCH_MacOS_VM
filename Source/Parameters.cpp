#include "Parameters.h"

#include "LayoutPreset.h"

namespace
{
juce::String makePairPrefix(int slotIndex)
{
    const int slotNumber = juce::jlimit(1, bsr::parameters::maxPairSlots, slotIndex + 1);
    return juce::String::formatted("pair%02d", slotNumber);
}

juce::String makePairParameterName(const juce::String& label, int slotIndex)
{
    return "Pair " + juce::String(slotIndex + 1) + " " + label;
}
} // namespace

juce::String bsr::parameters::pairEnable(int slotIndex)
{
    return makePairPrefix(slotIndex) + "Enable";
}

juce::String bsr::parameters::pairMute(int slotIndex)
{
    return makePairPrefix(slotIndex) + "Mute";
}

juce::String bsr::parameters::pairSolo(int slotIndex)
{
    return makePairPrefix(slotIndex) + "Solo";
}

juce::String bsr::parameters::pairGainDb(int slotIndex)
{
    return makePairPrefix(slotIndex) + "GainDb";
}

juce::String bsr::parameters::pairPan(int slotIndex)
{
    return makePairPrefix(slotIndex) + "Pan";
}

juce::String bsr::parameters::pairWetDry(int slotIndex)
{
    return makePairPrefix(slotIndex) + "WetDry";
}

juce::String bsr::parameters::pairIrPathStateKey(int slotIndex)
{
    return makePairPrefix(slotIndex) + "IrPath";
}

juce::AudioProcessorValueTreeState::ParameterLayout bsr::parameters::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { renderMode, 1 },
        "Mode",
        juce::StringArray { "Stereo", "Multichannel Renderer" },
        0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { layoutPreset, 1 },
        "Input Layout",
        bsr::mch::getLayoutDisplayNames(),
        0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { wetDry, 1 },
        "Wet/Dry",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 0.01f },
        100.0f,
        juce::AudioParameterFloatAttributes {}
            .withStringFromValueFunction([](float value, int) { return juce::String(value, 1) + " %"; })
            .withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { outputGainDb, 1 },
        "Output Gain",
        juce::NormalisableRange<float> { -24.0f, 12.0f, 0.01f },
        0.0f,
        juce::AudioParameterFloatAttributes {}
            .withStringFromValueFunction([](float value, int) { return juce::String(value, 1) + " dB"; })
            .withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { inputPan, 1 },
        "Input Pan",
        juce::NormalisableRange<float> { -100.0f, 100.0f, 0.01f },
        0.0f,
        juce::AudioParameterFloatAttributes {}
            .withStringFromValueFunction([](float value, int)
            {
                if (std::abs(value) < 0.05f)
                    return juce::String("C");
                return (value < 0.0f ? "L " : "R ") + juce::String(std::abs(value), 1) + " %";
            })
            .withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { bypass, 1 },
        "Bypass",
        false));

    // Keep this parameter for stable host state mapping. The actual path string is
    // stored in APVTS state properties, not in this boolean value.
    // Using a bool instead of AudioParameterChoice avoids pluginval NaN state restoration issues.
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { irPath, 1 },
        "IR Path",
        false));

    for (int slot = 0; slot < maxPairSlots; ++slot)
    {
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { pairEnable(slot), 1 },
            makePairParameterName("Enable", slot),
            true));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { pairMute(slot), 1 },
            makePairParameterName("Mute", slot),
            false));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { pairSolo(slot), 1 },
            makePairParameterName("Solo", slot),
            false));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { pairGainDb(slot), 1 },
            makePairParameterName("Gain", slot),
            juce::NormalisableRange<float> { -24.0f, 12.0f, 0.01f },
            0.0f,
            juce::AudioParameterFloatAttributes {}
                .withStringFromValueFunction([](float value, int) { return juce::String(value, 1) + " dB"; })
                .withLabel("dB")));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { pairPan(slot), 1 },
            makePairParameterName("Pan", slot),
            juce::NormalisableRange<float> { -100.0f, 100.0f, 0.01f },
            0.0f,
            juce::AudioParameterFloatAttributes {}
                .withStringFromValueFunction([](float value, int)
                {
                    if (std::abs(value) < 0.05f)
                        return juce::String("C");
                    return (value < 0.0f ? "A " : "B ") + juce::String(std::abs(value), 1) + " %";
                })
                .withLabel("%")));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { pairWetDry(slot), 1 },
            makePairParameterName("Wet/Dry", slot),
            juce::NormalisableRange<float> { 0.0f, 100.0f, 0.01f },
            100.0f,
            juce::AudioParameterFloatAttributes {}
                .withStringFromValueFunction([](float value, int) { return juce::String(value, 1) + " %"; })
                .withLabel("%")));
    }

    return { params.begin(), params.end() };
}

