#include "PluginEditor.h"

namespace
{
juce::String formatRmsDbfsText(float rmsLinear)
{
    const float dB = juce::Decibels::gainToDecibels(rmsLinear, -60.0f);
    if (dB > 0.0f)
        return "CLIP";

    const float clamped = juce::jlimit(-60.0f, 0.0f, dB);
    return juce::String(clamped, 1) + " dB";
}
}

BinauralSpeakerRoomAudioProcessorEditor::BinauralSpeakerRoomAudioProcessorEditor(BinauralSpeakerRoomAudioProcessor& processor)
    : AudioProcessorEditor(&processor),
      audioProcessor(processor)
{
    setSize(760, 720);

    headerLabel.setText("Sonarworks VMPRO MCH", juce::dontSendNotification);
    headerLabel.setJustificationType(juce::Justification::centredLeft);
    headerLabel.setFont(juce::Font(juce::FontOptions(24.0f).withStyle("Bold")));
    addAndMakeVisible(headerLabel);

    outputModeLabel.setText("Output: Stereo binaural headphones", juce::dontSendNotification);
    outputModeLabel.setJustificationType(juce::Justification::centredLeft);
    outputModeLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(outputModeLabel);

    hostInputChannelLabel.setText("Host I/O: -- in / -- out", juce::dontSendNotification);
    hostInputChannelLabel.setJustificationType(juce::Justification::centredRight);
    hostInputChannelLabel.setColour(juce::Label::textColourId, juce::Colours::lightblue);
    hostInputChannelLabel.setFont(juce::Font(juce::FontOptions(13.0f)));
    addAndMakeVisible(hostInputChannelLabel);

    loadIrButton.setButtonText("Load Stereo IR");
    loadIrButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Load 4-Channel IR WAV File",
            juce::File::getSpecialLocation(juce::File::userMusicDirectory),
            "*.wav");

        auto* chooserPtr = fileChooser.get();
        chooserPtr->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc)
            {
                const auto file = fc.getResult();
                if (file != juce::File{})
                {
                    if (!audioProcessor.loadIRFile(file))
                    {
                        const auto error = audioProcessor.getLastLoadError();
                        statusMessage.setText("Error: " + error, juce::dontSendNotification);
                        statusMessage.setColour(juce::Label::textColourId, juce::Colours::red);
                    }
                    else
                    {
                        updateIRStatus();
                    }
                }

                fileChooser = nullptr;
            });
    };
    addAndMakeVisible(loadIrButton);

    modeLabel.setText("Mode", juce::dontSendNotification);
    modeLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(modeLabel);

    modeSelector.addItem("Stereo", 1);
    modeSelector.addItem("Multichannel Renderer", 2);
    addAndMakeVisible(modeSelector);

    layoutLabel.setText("Layout", juce::dontSendNotification);
    layoutLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(layoutLabel);

    {
        const auto layoutNames = bsr::mch::getLayoutDisplayNames();
        for (int i = 0; i < layoutNames.size(); ++i)
            layoutSelector.addItem(layoutNames[i], i + 1);
    }
    addAndMakeVisible(layoutSelector);

    wetDryLabel.setText("Wet/Dry", juce::dontSendNotification);
    wetDryLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(wetDryLabel);

    wetDrySlider.setSliderStyle(juce::Slider::LinearHorizontal);
    wetDrySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 90, 22);
    addAndMakeVisible(wetDrySlider);

    outputGainLabel.setText("Output Gain", juce::dontSendNotification);
    outputGainLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(outputGainLabel);

    outputGainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    outputGainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 90, 22);
    addAndMakeVisible(outputGainSlider);

    inputPanLabel.setText("Input Pan", juce::dontSendNotification);
    inputPanLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(inputPanLabel);

    inputPanSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    inputPanSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 90, 22);
    addAndMakeVisible(inputPanSlider);

    bypassToggle.setButtonText("Bypass");
    addAndMakeVisible(bypassToggle);

    statusTitle.setText("Status", juce::dontSendNotification);
    statusTitle.setFont(juce::Font(juce::FontOptions(17.0f).withStyle("Bold")));
    statusTitle.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(statusTitle);

    statusMessage.setText("No IR loaded", juce::dontSendNotification);
    statusMessage.setJustificationType(juce::Justification::centredLeft);
    statusMessage.setColour(juce::Label::textColourId, juce::Colours::orange);
    addAndMakeVisible(statusMessage);

    warningTitle.setText("Warnings", juce::dontSendNotification);
    warningTitle.setFont(juce::Font(juce::FontOptions(15.0f).withStyle("Bold")));
    warningTitle.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(warningTitle);

    warningMessage.setText("No warnings", juce::dontSendNotification);
    warningMessage.setJustificationType(juce::Justification::topLeft);
    warningMessage.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(warningMessage);

    pairSlotsViewport.setViewedComponent(&pairSlotsContainer, false);
    addAndMakeVisible(pairSlotsViewport);

    auto& apvts = audioProcessor.getValueTreeState();
    modeAttachment = std::make_unique<ComboBoxAttachment>(apvts, bsr::parameters::renderMode, modeSelector);
    layoutAttachment = std::make_unique<ComboBoxAttachment>(apvts, bsr::parameters::layoutPreset, layoutSelector);
    wetDryAttachment = std::make_unique<SliderAttachment>(apvts, bsr::parameters::wetDry, wetDrySlider);
    outputGainAttachment = std::make_unique<SliderAttachment>(apvts, bsr::parameters::outputGainDb, outputGainSlider);
    inputPanAttachment = std::make_unique<SliderAttachment>(apvts, bsr::parameters::inputPan, inputPanSlider);
    bypassAttachment = std::make_unique<ButtonAttachment>(apvts, bsr::parameters::bypass, bypassToggle);

    initialisePairSlots();
    resized(); // re-run layout now that slot rows exist and have bounds
    updateMchUi();
    updateIRStatus();
    startTimerHz(15);
}

BinauralSpeakerRoomAudioProcessorEditor::~BinauralSpeakerRoomAudioProcessorEditor()
{
    stopTimer();
}

void BinauralSpeakerRoomAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(24, 30, 36));

    auto bounds = getLocalBounds().reduced(14);
    g.setColour(juce::Colour::fromRGB(36, 45, 54));
    g.fillRoundedRectangle(bounds.toFloat(), 10.0f);
}

void BinauralSpeakerRoomAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(24);

    auto top = bounds.removeFromTop(36);
    headerLabel.setBounds(top.removeFromLeft(380));
    hostInputChannelLabel.setBounds(top.removeFromRight(170));
    outputModeLabel.setBounds(top);

    bounds.removeFromTop(10);
    auto row1 = bounds.removeFromTop(32);
    modeLabel.setBounds(row1.removeFromLeft(48));
    modeSelector.setBounds(row1.removeFromLeft(200));
    row1.removeFromLeft(12);
    layoutLabel.setBounds(row1.removeFromLeft(54));
    layoutSelector.setBounds(row1.removeFromLeft(170));
    row1.removeFromLeft(12);
    loadIrButton.setBounds(row1.removeFromLeft(130));
    row1.removeFromLeft(12);
    bypassToggle.setBounds(row1.removeFromLeft(90));

    bounds.removeFromTop(14);
    auto row2 = bounds.removeFromTop(30);
    wetDryLabel.setBounds(row2.removeFromLeft(90));
    wetDrySlider.setBounds(row2);

    bounds.removeFromTop(10);
    auto row3 = bounds.removeFromTop(30);
    outputGainLabel.setBounds(row3.removeFromLeft(90));
    outputGainSlider.setBounds(row3);

    bounds.removeFromTop(10);
    auto row4 = bounds.removeFromTop(30);
    inputPanLabel.setBounds(row4.removeFromLeft(90));
    inputPanSlider.setBounds(row4);

    bounds.removeFromTop(10);

    // Size the pairSlotsContainer explicitly so getLocalBounds() is valid.
    // Each slot: controls row + two horizontal meter rows + status row + gap.
    const int slotControlH = 44;
    const int slotMeterH   = 18;
    const int slotStatusH  = 18;
    const int slotRowH     = slotControlH + slotMeterH + slotMeterH + slotStatusH + 4;
    const int containerW   = bounds.getWidth();
    const int containerH   = slotRowH * bsr::parameters::maxPairSlots + 8;
    pairSlotsContainer.setSize(containerW, containerH);

    // Layout pair slot rows inside the container using its local coordinate space.
    auto pairBounds = juce::Rectangle<int>(0, 4, containerW, containerH - 8);
    for (int slot = 0; slot < bsr::parameters::maxPairSlots; ++slot)
    {
        if (pairSlotRows[slot])
        {
            auto slotArea   = pairBounds.removeFromTop(slotRowH);
            auto controlRow = slotArea.removeFromTop(slotControlH);
            auto meterRowA  = slotArea.removeFromTop(slotMeterH);
            auto meterRowB  = slotArea.removeFromTop(slotMeterH);
            slotArea.removeFromTop(2);
            auto statusRow  = slotArea.removeFromTop(slotStatusH);

            pairSlotRows[slot]->group.setBounds(controlRow.removeFromLeft(78));
            pairSlotRows[slot]->enableToggle.setBounds(controlRow.removeFromLeft(60));
            pairSlotRows[slot]->muteToggle.setBounds(controlRow.removeFromLeft(50));
            pairSlotRows[slot]->soloToggle.setBounds(controlRow.removeFromLeft(50));
            pairSlotRows[slot]->gainSlider.setBounds(controlRow.removeFromLeft(90));
            pairSlotRows[slot]->panSlider.setBounds(controlRow.removeFromLeft(90));
            pairSlotRows[slot]->wetDrySlider.setBounds(controlRow.removeFromLeft(90));
            pairSlotRows[slot]->loadReplaceButton.setBounds(controlRow);

            auto layoutMeterRow = [](juce::Rectangle<int> row,
                                     LevelMeterBar& meter,
                                     juce::Label& valueLabel,
                                     const juce::String& prefix)
            {
                row.reduce(6, 0);
                auto prefixArea = row.removeFromLeft(14);
                auto valueArea = row.removeFromRight(62);
                valueLabel.setBounds(valueArea);
                meter.setBounds(row);
                valueLabel.setText(prefix + " " + formatRmsDbfsText(0.0f), juce::dontSendNotification);
                juce::ignoreUnused(prefixArea);
            };

            layoutMeterRow(meterRowA, pairSlotRows[slot]->meterA, pairSlotRows[slot]->meterAValueLabel, "L");
            layoutMeterRow(meterRowB, pairSlotRows[slot]->meterB, pairSlotRows[slot]->meterBValueLabel, "R");
            pairSlotRows[slot]->statusLabel.setBounds(statusRow);
        }
    }

    // Viewport clips the container; set its bounds in the editor coordinate space.
    pairSlotsViewport.setScrollBarsShown(true, false);
    pairSlotsViewport.setBounds(bounds.removeFromTop(260));

    bounds.removeFromTop(10);
    statusTitle.setBounds(bounds.removeFromTop(24));
    statusMessage.setBounds(bounds.removeFromTop(24));
    bounds.removeFromTop(10);
    warningTitle.setBounds(bounds.removeFromTop(24));
    warningMessage.setBounds(bounds.removeFromTop(84));
}

void BinauralSpeakerRoomAudioProcessorEditor::timerCallback()
{
    updateIRStatus();
    updateMchUi();

    // Update live I/O channel count display
    {
        const int inCh  = audioProcessor.getLastSeenInputChannels();
        const int outCh = audioProcessor.getLastSeenOutputChannels();
        const bool mchMode = isMchModeSelected();

        const int layoutIndex = getSelectedLayoutIndex();
        const auto& preset = bsr::mch::getLayoutPresetByChoiceIndex(layoutIndex);
        const int expectedCh = preset.channelCount; // plugin always has ≥6 pins; check against layout
        // Flag mismatch when in MCH mode and the active bus has fewer channels than the layout needs.
        // (e.g. host gave 6 pins but user selected 7.1 which needs 8)
        const bool busMismatch = mchMode && inCh > 0 && inCh < expectedCh;

        if (inCh == 0)
        {
            hostInputChannelLabel.setText("no signal yet", juce::dontSendNotification);
            hostInputChannelLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
        }
        else if (busMismatch)
        {
            hostInputChannelLabel.setText(juce::String(inCh) + "in / needs " + juce::String(expectedCh),
                                          juce::dontSendNotification);
            hostInputChannelLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
            // Surface actionable fix in the warnings area
            warningMessage.setText(
                "Bus mismatch: plugin sees " + juce::String(inCh) + " channels but layout needs "
                + juce::String(expectedCh) + ".\n"
                "Fix: REMOVE this plugin from the track and RE-ADD it. "
                "The host must negotiate " + juce::String(expectedCh) + " channels at insert time.",
                juce::dontSendNotification);
            warningMessage.setColour(juce::Label::textColourId, juce::Colours::orange);
        }
        else
        {
            hostInputChannelLabel.setText(juce::String(inCh) + " in / " + juce::String(outCh) + " out",
                                          juce::dontSendNotification);
            hostInputChannelLabel.setColour(juce::Label::textColourId, juce::Colours::lightblue);
        }
    }

    // Refresh individual slot status labels every tick (independent of mode/layout change)
    if (isMchModeSelected())
    {
        for (int slot = 0; slot < bsr::parameters::maxPairSlots; ++slot)
        {
            if (!pairSlotRows[slot] || !pairSlotRows[slot]->statusLabel.isVisible())
                continue;
            pairSlotRows[slot]->statusLabel.setText(getSlotStatusText(slot), juce::dontSendNotification);
            pairSlotRows[slot]->loadReplaceButton.setButtonText(getSlotLoadButtonText(slot));
            const float rmsA = audioProcessor.getMchSlotOutputRmsL(slot);
            const float rmsB = audioProcessor.getMchSlotOutputRmsR(slot);
            pairSlotRows[slot]->meterA.setLevel(rmsA);
            pairSlotRows[slot]->meterB.setLevel(rmsB);
            pairSlotRows[slot]->meterAValueLabel.setText("L " + formatRmsDbfsText(rmsA), juce::dontSendNotification);
            pairSlotRows[slot]->meterBValueLabel.setText("R " + formatRmsDbfsText(rmsB), juce::dontSendNotification);
        }
    }
}

void BinauralSpeakerRoomAudioProcessorEditor::updateIRStatus() noexcept
{
    const auto status = audioProcessor.getPluginStateText();
    statusMessage.setText(status, juce::dontSendNotification);

    const bool statusOk = status.startsWith("[OK]");
    statusMessage.setColour(juce::Label::textColourId,
                            statusOk ? juce::Colours::lightgreen : juce::Colours::orange);

    const auto warning = audioProcessor.getWarningText();
    warningMessage.setText(warning.isEmpty() ? "No warnings" : warning, juce::dontSendNotification);
}

void BinauralSpeakerRoomAudioProcessorEditor::initialisePairSlots()
{
    auto& apvts = audioProcessor.getValueTreeState();
    const auto& presets = bsr::mch::getLayoutPresets();
    
    for (int slot = 0; slot < bsr::parameters::maxPairSlots; ++slot)
    {
        auto& row = pairSlotRows[slot];
        if (!row)
            row = std::make_unique<PairSlotRow>();
        
        auto& slotRow = *row;
        
        slotRow.group.setText(presets[0].slots[slot].pairName);
        slotRow.pairLabel.setText("Pair " + juce::String(slot + 1), juce::dontSendNotification);
        slotRow.statusLabel.setText("", juce::dontSendNotification);
        
        slotRow.enableToggle.setButtonText("Enable");
        slotRow.muteToggle.setButtonText("Mute");
        slotRow.soloToggle.setButtonText("Solo");
        slotRow.meterAValueLabel.setJustificationType(juce::Justification::centredRight);
        slotRow.meterBValueLabel.setJustificationType(juce::Justification::centredRight);
        slotRow.meterAValueLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        slotRow.meterBValueLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        slotRow.meterAValueLabel.setFont(juce::Font(juce::FontOptions(11.0f)));
        slotRow.meterBValueLabel.setFont(juce::Font(juce::FontOptions(11.0f)));
        slotRow.meterAValueLabel.setText("L " + formatRmsDbfsText(0.0f), juce::dontSendNotification);
        slotRow.meterBValueLabel.setText("R " + formatRmsDbfsText(0.0f), juce::dontSendNotification);
        slotRow.gainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        slotRow.panSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        slotRow.wetDrySlider.setSliderStyle(juce::Slider::LinearHorizontal);
        
        slotRow.enableAttachment = std::make_unique<ButtonAttachment>(apvts, bsr::parameters::pairEnable(slot), slotRow.enableToggle);
        slotRow.muteAttachment = std::make_unique<ButtonAttachment>(apvts, bsr::parameters::pairMute(slot), slotRow.muteToggle);
        slotRow.soloAttachment = std::make_unique<ButtonAttachment>(apvts, bsr::parameters::pairSolo(slot), slotRow.soloToggle);
        slotRow.gainAttachment = std::make_unique<SliderAttachment>(apvts, bsr::parameters::pairGainDb(slot), slotRow.gainSlider);
        slotRow.panAttachment = std::make_unique<SliderAttachment>(apvts, bsr::parameters::pairPan(slot), slotRow.panSlider);
        slotRow.wetDryAttachment = std::make_unique<SliderAttachment>(apvts, bsr::parameters::pairWetDry(slot), slotRow.wetDrySlider);
        
        slotRow.loadReplaceButton.setButtonText(getSlotLoadButtonText(slot));
        slotRow.loadReplaceButton.onClick = [this, slot]
        {
            showSlotLoadDialog(slot);
        };
        
        pairSlotsContainer.addAndMakeVisible(slotRow.group);
        pairSlotsContainer.addAndMakeVisible(slotRow.enableToggle);
        pairSlotsContainer.addAndMakeVisible(slotRow.muteToggle);
        pairSlotsContainer.addAndMakeVisible(slotRow.soloToggle);
        pairSlotsContainer.addAndMakeVisible(slotRow.gainSlider);
        pairSlotsContainer.addAndMakeVisible(slotRow.panSlider);
        pairSlotsContainer.addAndMakeVisible(slotRow.wetDrySlider);
        pairSlotsContainer.addAndMakeVisible(slotRow.loadReplaceButton);
        pairSlotsContainer.addAndMakeVisible(slotRow.statusLabel);
        pairSlotsContainer.addAndMakeVisible(slotRow.meterA);
        pairSlotsContainer.addAndMakeVisible(slotRow.meterB);
        pairSlotsContainer.addAndMakeVisible(slotRow.meterAValueLabel);
        pairSlotsContainer.addAndMakeVisible(slotRow.meterBValueLabel);
    }
}

void BinauralSpeakerRoomAudioProcessorEditor::updateMchUi() noexcept
{
    const auto currentMchMode = isMchModeSelected();
    const auto currentLayoutIndex = getSelectedLayoutIndex();

    // Fire immediately on any change; no decimator
    if (currentMchMode == lastMchMode && currentLayoutIndex == lastLayoutIndex)
        return;

    lastMchMode = currentMchMode;
    lastLayoutIndex = currentLayoutIndex;

    auto setRowVisible = [](PairSlotRow& row, bool visible)
    {
        row.group.setVisible(visible);
        row.meterA.setVisible(visible);
        row.meterB.setVisible(visible);
        row.meterAValueLabel.setVisible(visible);
        row.meterBValueLabel.setVisible(visible);
        row.enableToggle.setVisible(visible);
        row.muteToggle.setVisible(visible);
        row.soloToggle.setVisible(visible);
        row.gainSlider.setVisible(visible);
        row.panSlider.setVisible(visible);
        row.wetDrySlider.setVisible(visible);
        row.loadReplaceButton.setVisible(visible);
        row.statusLabel.setVisible(visible);
    };

    if (!currentMchMode)
    {
        pairSlotsContainer.setVisible(false);
        loadIrButton.setButtonText("Load Stereo IR");
        return;
    }

    pairSlotsContainer.setVisible(true);
    loadIrButton.setButtonText("MCH Mode - Use slot buttons to load");

    const auto& presets = bsr::mch::getLayoutPresets();
    const auto& currentPreset = presets[juce::jlimit(0, static_cast<int>(presets.size()) - 1, currentLayoutIndex)];
    const auto visibleSlots = currentPreset.visibleSlotCount;

    for (int slot = 0; slot < bsr::parameters::maxPairSlots; ++slot)
    {
        if (!pairSlotRows[slot]) continue;
        const bool visible = (slot < visibleSlots);
        setRowVisible(*pairSlotRows[slot], visible);
        if (visible)
        {
            pairSlotRows[slot]->group.setText(currentPreset.slots[slot].pairName);
            pairSlotRows[slot]->statusLabel.setText(getSlotStatusText(slot), juce::dontSendNotification);
            pairSlotRows[slot]->loadReplaceButton.setButtonText(getSlotLoadButtonText(slot));
        }
    }
}

bool BinauralSpeakerRoomAudioProcessorEditor::isMchModeSelected() const noexcept
{
    return modeSelector.getSelectedId() == 2;
}

int BinauralSpeakerRoomAudioProcessorEditor::getSelectedLayoutIndex() const noexcept
{
    const auto selectedId = layoutSelector.getSelectedId();
    return (selectedId > 0) ? (selectedId - 1) : 0;
}

void BinauralSpeakerRoomAudioProcessorEditor::showSlotLoadDialog(int slotIndex)
{
    slotFileChooserIndex = slotIndex;
    slotFileChooser = std::make_unique<juce::FileChooser>(
        "Load 4-Channel Quad IR WAV File",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.wav");

    auto* chooserPtr = slotFileChooser.get();
    chooserPtr->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            const auto file = fc.getResult();
            if (file != juce::File{} && slotFileChooserIndex >= 0)
            {
                if (!audioProcessor.loadMchSlotIRFile(slotFileChooserIndex, file))
                {
                    const auto error = audioProcessor.getLastLoadError();
                    statusMessage.setText("Error loading slot " + juce::String(slotFileChooserIndex + 1) + ": " + error, 
                                         juce::dontSendNotification);
                    statusMessage.setColour(juce::Label::textColourId, juce::Colours::red);
                }
                else
                {
                    updateIRStatus();
                }
            }
            
            slotFileChooser = nullptr;
            slotFileChooserIndex = -1;
        });
}

juce::String BinauralSpeakerRoomAudioProcessorEditor::getSlotStatusText(int slotIndex) const
{
    return audioProcessor.getMchSlotStatusText(slotIndex);
}

juce::String BinauralSpeakerRoomAudioProcessorEditor::getSlotLoadButtonText(int slotIndex) const
{
    return audioProcessor.isMchSlotIRLoaded(slotIndex) ? "Replace IR" : "Load IR";
}
