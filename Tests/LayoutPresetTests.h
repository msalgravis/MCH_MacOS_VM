#pragma once

#include <JuceHeader.h>

#include "../Source/LayoutPreset.h"

class LayoutPresetTests : public juce::UnitTest
{
public:
    LayoutPresetTests() : juce::UnitTest("Layout presets", "MCH") {}

    void runTest() override
    {
        beginTest("Expected preset count and names");
        testPresetNames();

        beginTest("Visible slot counts match PRD");
        testVisibleSlotCounts();

        beginTest("9.1.6 channel order and slots are correct");
        testNineOneSixDefinition();
    }

private:
    void testPresetNames()
    {
        const auto names = bsr::mch::getLayoutDisplayNames();
        expectEquals(names.size(), bsr::mch::numLayoutPresets, "Unexpected number of layout presets");

        const juce::StringArray expected { "Stereo", "5.1", "7.1", "7.1.2", "7.1.4", "9.1.4", "9.1.6" };
        expect(names == expected, "Layout preset names do not match PRD");
    }

    void testVisibleSlotCounts()
    {
        const auto& presets = bsr::mch::getLayoutPresets();
        const std::array<int, bsr::mch::numLayoutPresets> expectedCounts { 1, 3, 4, 5, 6, 7, 8 };

        for (size_t index = 0; index < presets.size(); ++index)
            expectEquals(presets[index].visibleSlotCount,
                         expectedCounts[index],
                         "Unexpected visible slot count for preset index " + juce::String(static_cast<int>(index)));
    }

    void testNineOneSixDefinition()
    {
        const auto& preset = bsr::mch::getLayoutPresetByChoiceIndex(6);

        expectEquals(preset.channelCount, 16, "9.1.6 should declare 16 channels");
        expectEquals(preset.visibleSlotCount, 8, "9.1.6 should use 8 slots");

        expect(juce::String(preset.channelOrder[10]) == "Tfl", "Channel 11 should be Tfl");
        expect(juce::String(preset.channelOrder[11]) == "Tfr", "Channel 12 should be Tfr");
        expect(juce::String(preset.channelOrder[12]) == "Tml", "Channel 13 should be Tml");
        expect(juce::String(preset.channelOrder[13]) == "Tmr", "Channel 14 should be Tmr");
        expect(juce::String(preset.channelOrder[14]) == "Trl", "Channel 15 should be Trl");
        expect(juce::String(preset.channelOrder[15]) == "Trr", "Channel 16 should be Trr");

        const auto& slot8 = preset.slots[7];
        expect(juce::String(slot8.pairName) == "Trl / Trr", "Slot 8 should map Trl / Trr");
        expectEquals(slot8.inputA, 15, "Slot 8 input A should be channel 15");
        expectEquals(slot8.inputB, 16, "Slot 8 input B should be channel 16");
    }
};

static LayoutPresetTests layoutPresetTests;
