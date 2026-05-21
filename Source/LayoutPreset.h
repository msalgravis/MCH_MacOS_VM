#pragma once

#include <JuceHeader.h>

#include <algorithm>
#include <array>

namespace bsr::mch
{
inline constexpr int maxPairSlots = 8;
inline constexpr int numLayoutPresets = 7;

struct PairSlotDefinition
{
    int slotIndex = 0; // 0-based
    const char* pairName = "";
    int inputA = 0; // 1-based host channel index
    int inputB = 0; // 1-based host channel index
};

struct LayoutPresetDefinition
{
    const char* displayName = "Stereo";
    int channelCount = 2;
    std::array<const char*, 16> channelOrder {};
    std::array<PairSlotDefinition, maxPairSlots> slots {};
    int visibleSlotCount = 1;
};

inline const std::array<LayoutPresetDefinition, numLayoutPresets>& getLayoutPresets()
{
    static const std::array<LayoutPresetDefinition, numLayoutPresets> presets {
        LayoutPresetDefinition {
            "Stereo",
            2,
            { "L", "R", "", "", "", "", "", "", "", "", "", "", "", "", "", "" },
            {
                PairSlotDefinition { 0, "L / R", 1, 2 },
                PairSlotDefinition { 1, "", 0, 0 },
                PairSlotDefinition { 2, "", 0, 0 },
                PairSlotDefinition { 3, "", 0, 0 },
                PairSlotDefinition { 4, "", 0, 0 },
                PairSlotDefinition { 5, "", 0, 0 },
                PairSlotDefinition { 6, "", 0, 0 },
                PairSlotDefinition { 7, "", 0, 0 },
            },
            1,
        },
        LayoutPresetDefinition {
            "5.1",
            6,
            { "L", "R", "C", "LFE", "Ls", "Rs", "", "", "", "", "", "", "", "", "", "" },
            {
                PairSlotDefinition { 0, "L / R", 1, 2 },
                PairSlotDefinition { 1, "C / LFE", 3, 4 },
                PairSlotDefinition { 2, "Ls / Rs", 5, 6 },
                PairSlotDefinition { 3, "", 0, 0 },
                PairSlotDefinition { 4, "", 0, 0 },
                PairSlotDefinition { 5, "", 0, 0 },
                PairSlotDefinition { 6, "", 0, 0 },
                PairSlotDefinition { 7, "", 0, 0 },
            },
            3,
        },
        LayoutPresetDefinition {
            "7.1",
            8,
            { "L", "R", "C", "LFE", "Ls", "Rs", "Lrs", "Rrs", "", "", "", "", "", "", "", "" },
            {
                PairSlotDefinition { 0, "L / R", 1, 2 },
                PairSlotDefinition { 1, "C / LFE", 3, 4 },
                PairSlotDefinition { 2, "Ls / Rs", 5, 6 },
                PairSlotDefinition { 3, "Lrs / Rrs", 7, 8 },
                PairSlotDefinition { 4, "", 0, 0 },
                PairSlotDefinition { 5, "", 0, 0 },
                PairSlotDefinition { 6, "", 0, 0 },
                PairSlotDefinition { 7, "", 0, 0 },
            },
            4,
        },
        LayoutPresetDefinition {
            "7.1.2",
            10,
            { "L", "R", "C", "LFE", "Ls", "Rs", "Lrs", "Rrs", "Tfl", "Tfr", "", "", "", "", "", "" },
            {
                PairSlotDefinition { 0, "L / R", 1, 2 },
                PairSlotDefinition { 1, "C / LFE", 3, 4 },
                PairSlotDefinition { 2, "Ls / Rs", 5, 6 },
                PairSlotDefinition { 3, "Lrs / Rrs", 7, 8 },
                PairSlotDefinition { 4, "Tfl / Tfr", 9, 10 },
                PairSlotDefinition { 5, "", 0, 0 },
                PairSlotDefinition { 6, "", 0, 0 },
                PairSlotDefinition { 7, "", 0, 0 },
            },
            5,
        },
        LayoutPresetDefinition {
            "7.1.4",
            12,
            { "L", "R", "C", "LFE", "Ls", "Rs", "Lrs", "Rrs", "Tfl", "Tfr", "Trl", "Trr", "", "", "", "" },
            {
                PairSlotDefinition { 0, "L / R", 1, 2 },
                PairSlotDefinition { 1, "C / LFE", 3, 4 },
                PairSlotDefinition { 2, "Ls / Rs", 5, 6 },
                PairSlotDefinition { 3, "Lrs / Rrs", 7, 8 },
                PairSlotDefinition { 4, "Tfl / Tfr", 9, 10 },
                PairSlotDefinition { 5, "Trl / Trr", 11, 12 },
                PairSlotDefinition { 6, "", 0, 0 },
                PairSlotDefinition { 7, "", 0, 0 },
            },
            6,
        },
        LayoutPresetDefinition {
            "9.1.4",
            14,
            { "L", "R", "C", "LFE", "Ls", "Rs", "Lrs", "Rrs", "Lw", "Rw", "Tfl", "Tfr", "Trl", "Trr", "", "" },
            {
                PairSlotDefinition { 0, "L / R", 1, 2 },
                PairSlotDefinition { 1, "C / LFE", 3, 4 },
                PairSlotDefinition { 2, "Ls / Rs", 5, 6 },
                PairSlotDefinition { 3, "Lrs / Rrs", 7, 8 },
                PairSlotDefinition { 4, "Lw / Rw", 9, 10 },
                PairSlotDefinition { 5, "Tfl / Tfr", 11, 12 },
                PairSlotDefinition { 6, "Trl / Trr", 13, 14 },
                PairSlotDefinition { 7, "", 0, 0 },
            },
            7,
        },
        LayoutPresetDefinition {
            "9.1.6",
            16,
            { "L", "R", "C", "LFE", "Ls", "Rs", "Lrs", "Rrs", "Lw", "Rw", "Tfl", "Tfr", "Tml", "Tmr", "Trl", "Trr" },
            {
                PairSlotDefinition { 0, "L / R", 1, 2 },
                PairSlotDefinition { 1, "C / LFE", 3, 4 },
                PairSlotDefinition { 2, "Ls / Rs", 5, 6 },
                PairSlotDefinition { 3, "Lrs / Rrs", 7, 8 },
                PairSlotDefinition { 4, "Lw / Rw", 9, 10 },
                PairSlotDefinition { 5, "Tfl / Tfr", 11, 12 },
                PairSlotDefinition { 6, "Tml / Tmr", 13, 14 },
                PairSlotDefinition { 7, "Trl / Trr", 15, 16 },
            },
            8,
        },
    };

    return presets;
}

inline const LayoutPresetDefinition& getLayoutPresetByChoiceIndex(int choiceIndex)
{
    const auto& presets = getLayoutPresets();
    const auto clampedIndex = std::clamp(choiceIndex, 0, static_cast<int>(presets.size() - 1));
    return presets[static_cast<size_t>(clampedIndex)];
}

inline juce::StringArray getLayoutDisplayNames()
{
    juce::StringArray names;
    for (const auto& preset : getLayoutPresets())
        names.add(preset.displayName);
    return names;
}

inline juce::String buildChannelMapText(const LayoutPresetDefinition& preset)
{
    juce::String text = "Input channel order: ";

    for (int channel = 0; channel < preset.channelCount; ++channel)
    {
        if (channel > 0)
            text << "  |  ";

        text << juce::String(channel + 1) << ": " << preset.channelOrder[static_cast<size_t>(channel)];
    }

    return text;
}

} // namespace bsr::mch
