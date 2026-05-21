#pragma once

#include <JuceHeader.h>

namespace bsr::parameters
{
inline constexpr int maxPairSlots = 8;

inline constexpr auto renderMode = "renderMode";
inline constexpr auto layoutPreset = "layoutPreset";

inline constexpr auto wetDry = "wetDry";
inline constexpr auto outputGainDb = "outputGainDb";
inline constexpr auto inputPan = "inputPan";
inline constexpr auto bypass = "bypass";
inline constexpr auto irPath = "irPath";

juce::String pairEnable(int slotIndex);
juce::String pairMute(int slotIndex);
juce::String pairSolo(int slotIndex);
juce::String pairGainDb(int slotIndex);
juce::String pairPan(int slotIndex);
juce::String pairWetDry(int slotIndex);
juce::String pairIrPathStateKey(int slotIndex);

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
} // namespace bsr::parameters
