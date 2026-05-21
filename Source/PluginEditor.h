#pragma once

#include <JuceHeader.h>
#include <array>

#include "PluginProcessor.h"

/// Simple horizontal level bar used for per-slot output contribution meters.
class LevelMeterBar final : public juce::Component
{
public:
    void setLevel(float rmsLinear) noexcept
    {
        const float dB = juce::Decibels::gainToDecibels(rmsLinear, -60.0f);
        const float n = juce::jlimit(0.0f, 1.0f, (dB + 60.0f) / 60.0f);
        if (std::abs(n - normLevel) > 0.004f) { normLevel = n; repaint(); }
    }
    void paint(juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(juce::Colours::black.withAlpha(0.7f));
        g.fillRect(b);
        const float fillW = b.getWidth() * normLevel;
        if (fillW > 0.5f)
        {
            auto fill = b.withWidth(fillW);
            g.setColour(normLevel < 0.75f ? juce::Colours::limegreen
                      : normLevel < 0.92f ? juce::Colours::yellow
                                          : juce::Colours::red);
            g.fillRect(fill);
        }
        g.setColour(juce::Colours::grey.withAlpha(0.4f));
        g.drawRect(b, 0.5f);
    }
private:
    float normLevel = 0.0f;
};

class BinauralSpeakerRoomAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                      private juce::Timer
{
public:
    explicit BinauralSpeakerRoomAudioProcessorEditor(BinauralSpeakerRoomAudioProcessor& processor);
    ~BinauralSpeakerRoomAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    BinauralSpeakerRoomAudioProcessor& audioProcessor;

    struct PairSlotRow
    {
        juce::GroupComponent group;
        juce::Label pairLabel;
        juce::Label inputLabel;
        juce::Label statusLabel;
        juce::ToggleButton enableToggle;
        juce::ToggleButton muteToggle;
        juce::ToggleButton soloToggle;
        juce::Slider gainSlider;
        juce::Slider panSlider;
        juce::Slider wetDrySlider;
        juce::TextButton loadReplaceButton;

        LevelMeterBar meterA, meterB;
        juce::Label meterAValueLabel, meterBValueLabel;

        std::unique_ptr<ButtonAttachment> enableAttachment;
        std::unique_ptr<ButtonAttachment> muteAttachment;
        std::unique_ptr<ButtonAttachment> soloAttachment;
        std::unique_ptr<SliderAttachment> gainAttachment;
        std::unique_ptr<SliderAttachment> panAttachment;
        std::unique_ptr<SliderAttachment> wetDryAttachment;
    };

    juce::Label headerLabel;
    juce::TextButton loadIrButton;

    juce::Label modeLabel;
    juce::ComboBox modeSelector;
    juce::Label layoutLabel;
    juce::ComboBox layoutSelector;
    juce::Label outputModeLabel;
    juce::Label activePairPathCountLabel;
    juce::Label dspLoadDetailLabel;
    juce::Label hostInputChannelLabel;
    juce::Label channelMapLabel;

    juce::Slider wetDrySlider;
    juce::Label wetDryLabel;

    juce::Slider outputGainSlider;
    juce::Label outputGainLabel;

    juce::Slider inputPanSlider;
    juce::Label inputPanLabel;

    juce::ToggleButton bypassToggle;

    juce::Label statusTitle;
    juce::Label statusMessage;
    juce::Label irMetadataLabel;
    juce::Label hostMetadataLabel;
    juce::Label routingSummary;
    juce::Label gainModeSummary;

    juce::Label metersTitle;
    juce::Label inputMeterLabel;
    juce::Label outputMeterLabel;
    juce::Label inputRmsLabel;
    juce::Label outputRmsLabel;

    double inputMeterLValue = 0.0;
    double inputMeterRValue = 0.0;
    double outputMeterLValue = 0.0;
    double outputMeterRValue = 0.0;

    juce::ProgressBar inputMeterL { inputMeterLValue };
    juce::ProgressBar inputMeterR { inputMeterRValue };
    juce::ProgressBar outputMeterL { outputMeterLValue };
    juce::ProgressBar outputMeterR { outputMeterRValue };

    juce::Label warningTitle;
    juce::Label warningMessage;
    juce::Label clipIndicator;

    juce::Component pairSlotsContainer;
    juce::Viewport pairSlotsViewport;
    std::array<std::unique_ptr<PairSlotRow>, bsr::parameters::maxPairSlots> pairSlotRows;

    std::unique_ptr<SliderAttachment> wetDryAttachment;
    std::unique_ptr<SliderAttachment> outputGainAttachment;
    std::unique_ptr<SliderAttachment> inputPanAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;
    std::unique_ptr<ComboBoxAttachment> modeAttachment;
    std::unique_ptr<ComboBoxAttachment> layoutAttachment;

    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::FileChooser> slotFileChooser;
    int slotFileChooserIndex = -1;
    bool lastMchMode = false;
    int lastLayoutIndex = -1;

    juce::Rectangle<int> upperPanelBounds;
    juce::Rectangle<int> mchPanelBounds;

    void timerCallback() override;
    void updateIRStatus() noexcept;
    void initialisePairSlots();
    void updateMchUi() noexcept;
    bool isMchModeSelected() const noexcept;
    int getSelectedLayoutIndex() const noexcept;
    void showSlotLoadDialog(int slotIndex);
    juce::String getSlotStatusText(int slotIndex) const;
    juce::String getSlotLoadButtonText(int slotIndex) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BinauralSpeakerRoomAudioProcessorEditor)
};
