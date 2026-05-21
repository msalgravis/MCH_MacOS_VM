#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "MchDryRouting.h"

#include <cmath>

namespace
{
juce::String normalisePersistedPathString(const juce::String& rawPath)
{
    auto path = rawPath.trim();
    if (path.isEmpty())
        return {};

    const bool hasDoubleQuotes = path.startsWithChar('"') && path.endsWithChar('"');
    const bool hasSingleQuotes = path.startsWithChar('\'') && path.endsWithChar('\'');
    if ((hasDoubleQuotes || hasSingleQuotes) && path.length() > 1)
        path = path.substring(1, path.length() - 1).trim();

    if (path.startsWith("~/") || path.startsWith("~\\"))
    {
        const auto relative = path.substring(2);
        return juce::File::getSpecialLocation(juce::File::userHomeDirectory)
            .getChildFile(relative)
            .getFullPathName();
    }

    return path;
}

juce::File makeFileFromPersistedPath(const juce::String& rawPath)
{
    const auto path = normalisePersistedPathString(rawPath);
    if (path.isEmpty())
        return {};

    juce::File file(path);
    if (juce::File::isAbsolutePath(path))
        return file;

    return juce::File::getCurrentWorkingDirectory().getChildFile(path);
}

float findAbsPeak(const float* data, int numSamples) noexcept
{
    if (data == nullptr || numSamples <= 0)
        return 0.0f;

    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
        peak = juce::jmax(peak, std::abs(data[i]));

    return peak;
}

float findBlockRms(const float* data, int numSamples) noexcept
{
    if (data == nullptr || numSamples <= 0)
        return 0.0f;

    double sumSquares = 0.0;
    for (int i = 0; i < numSamples; ++i)
        sumSquares += static_cast<double>(data[i]) * static_cast<double>(data[i]);

    return static_cast<float>(std::sqrt(sumSquares / static_cast<double>(numSamples)));
}

float smoothShortTermRms(float previous, float currentBlockRms, int numSamples, double sampleRate) noexcept
{
    if (sampleRate <= 0.0 || numSamples <= 0)
        return currentBlockRms;

    constexpr double rmsWindowSeconds = 0.3;
    const auto alpha = static_cast<float>(std::exp(-static_cast<double>(numSamples) / (sampleRate * rmsWindowSeconds)));
    return alpha * previous + (1.0f - alpha) * currentBlockRms;
}
} // namespace

BinauralSpeakerRoomAudioProcessor::BinauralSpeakerRoomAudioProcessor()
    : AudioProcessor(BusesProperties()
                         // Use 9.1.6 (16ch) as the default input bus so JUCE maps it to
                         // VST3's k91_6_W. Reaper then shows 16 input pins, covering every
                         // layout from 5.1 (6ch) up to 9.1.6 (16ch). On smaller tracks
                         // (e.g. 5.1), Reaper connects only channels 1-6 and leaves the rest
                         // silent. isBusesLayoutSupported still requires ≥6ch so Reaper's
                         // stereo probe is rejected and the 16-ch default is used.
                         .withInput("Input", juce::AudioChannelSet::create9point1point6(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", bsr::parameters::createParameterLayout())
{
    initialiseCachedParameterPointers();
    restoreIRPathFromState();
    restoreMchSlotIRPathsFromState();

    // Listener to sync global pan/gain/wet-dry to slot parameters in MCH mode
    apvts.addParameterListener(bsr::parameters::inputPan, this);
    apvts.addParameterListener(bsr::parameters::outputGainDb, this);
    apvts.addParameterListener(bsr::parameters::wetDry, this);
}

BinauralSpeakerRoomAudioProcessor::~BinauralSpeakerRoomAudioProcessor()
{
    apvts.removeParameterListener(bsr::parameters::inputPan, this);
    apvts.removeParameterListener(bsr::parameters::outputGainDb, this);
    apvts.removeParameterListener(bsr::parameters::wetDry, this);
}


void BinauralSpeakerRoomAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    hostSampleRateHz = sampleRate;
    for (auto& rms : mchSlotOutputRmsL)
        rms.store(0.0f, std::memory_order_relaxed);
    for (auto& rms : mchSlotOutputRmsR)
        rms.store(0.0f, std::memory_order_relaxed);
    convolver.prepareToPlay(sampleRate, samplesPerBlock);
    pannedInputBuffer.setSize(2, samplesPerBlock, true, false, true);
    mchInputScratchBuffer.setSize(bsr::mch::maxRendererInputChannels, samplesPerBlock, false, true, true);
    mchSlotIrManager.setHostSampleRate(sampleRate);
    mchRenderer.prepareToPlay(sampleRate, samplesPerBlock);
    applyAllSlotIrsToRenderer();
}

void BinauralSpeakerRoomAudioProcessor::releaseResources()
{
    inputRmsL.store(0.0f, std::memory_order_relaxed);
    inputRmsR.store(0.0f, std::memory_order_relaxed);
    outputRmsL.store(0.0f, std::memory_order_relaxed);
    outputRmsR.store(0.0f, std::memory_order_relaxed);
    inputPeakL.store(0.0f, std::memory_order_relaxed);
    inputPeakR.store(0.0f, std::memory_order_relaxed);
    outputPeakL.store(0.0f, std::memory_order_relaxed);
    outputPeakR.store(0.0f, std::memory_order_relaxed);
    clipWarningHoldBlocks.store(0, std::memory_order_relaxed);
    for (auto& rms : mchInputChannelRms)
        rms.store(0.0f, std::memory_order_relaxed);
    for (auto& rms : mchSlotOutputRmsL)
        rms.store(0.0f, std::memory_order_relaxed);
    for (auto& rms : mchSlotOutputRmsR)
        rms.store(0.0f, std::memory_order_relaxed);
    mchActivePairCount.store(0, std::memory_order_relaxed);
    mchVisiblePairCount.store(0, std::memory_order_relaxed);
    mchSkippedPairCount.store(0, std::memory_order_relaxed);
    mchActiveWetPairCount.store(0, std::memory_order_relaxed);
    mchActiveDryPairCount.store(0, std::memory_order_relaxed);
    mchActiveConvolutionPathCount.store(0, std::memory_order_relaxed);
    mchMaxConvolutionPathCount.store(0, std::memory_order_relaxed);
    {
        const juce::SpinLock::ScopedLockType lock(pendingIRDataLock);
        pendingIRData.reset();
    }
    pannedInputBuffer.setSize(0, 0, true, false, true);
    convolver.releaseResources();
    mchRenderer.releaseResources();
}

bool BinauralSpeakerRoomAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainInput = layouts.getMainInputChannelSet();
    const auto& mainOutput = layouts.getMainOutputChannelSet();

    if (mainOutput != juce::AudioChannelSet::stereo())
        return false;

    const int inputChannelCount = mainInput.size();
    // Require at least 6 input channels. If we accept stereo (2ch), Reaper locks in
    // 2 input pins even on 6-channel tracks, because Reaper's VST3 negotiation probes
    // stereo first and uses it if accepted. By rejecting <6ch, Reaper falls back to
    // the plugin's declared default (create5point1 = 6ch), giving 6 input pins.
    // On a stereo track, Reaper connects ch1/ch2 to pins 1/2 and leaves pins 3-6
    // silent; slot 0 (L/R = ch1+ch2) still processes correctly.
    if (inputChannelCount < 6 || inputChannelCount > bsr::mch::maxRendererInputChannels)
        return false;

    return true;
}

void BinauralSpeakerRoomAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const int inputChannelCount = this->getTotalNumInputChannels();
    const int outputChannelCount = this->getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    // Hosts may probe plugins with transient zero-channel layouts.
    // Avoid any pointer access unless at least one output channel exists.
    if (outputChannelCount <= 0 || numSamples <= 0)
        return;

    lastSeenInputChannels.store(inputChannelCount, std::memory_order_relaxed);
    lastSeenOutputChannels.store(outputChannelCount, std::memory_order_relaxed);

    const bool mchMode = modeParam != nullptr && modeParam->load() > 0.5f;

    if (mchMode)
    {
        const bool isBypassed = bypassParam != nullptr && bypassParam->load() > 0.5f;

        const int layoutIndex = layoutParam != nullptr ? juce::roundToInt(layoutParam->load()) : 0;
        const auto& preset = bsr::mch::getLayoutPresetByChoiceIndex(layoutIndex);
        const int usableInputChannels = juce::jmin(inputChannelCount, preset.channelCount);
        const int visibleSlotCount = juce::jlimit(0, bsr::mch::maxPairSlots, preset.visibleSlotCount);

        const float globalWetPercent = wetDryParam != nullptr ? wetDryParam->load() : 100.0f;
        const float outputGainDb = outputGainParam != nullptr ? outputGainParam->load() : 0.0f;
        const float globalWet = juce::jlimit(0.0f, 1.0f, globalWetPercent / 100.0f);
        const float outputGain = juce::Decibels::decibelsToGain(outputGainDb);
        const float meterWet = isBypassed ? 0.0f : globalWet;

        std::array<const float*, bsr::mch::maxRendererInputChannels> inputPointers {};
        // Copy input channels to scratch buffer first: render() clears outputL/outputR which
        // alias with buffer channels 0 and 1, corrupting L/R input before convolution.
        for (int channel = 0; channel < usableInputChannels; ++channel)
            mchInputScratchBuffer.copyFrom(channel, 0, buffer, channel, 0, numSamples);
        for (int channel = 0; channel < usableInputChannels; ++channel)
            inputPointers[static_cast<size_t>(channel)] = mchInputScratchBuffer.getReadPointer(channel);

        // Per-channel RMS for slot-level meters (UI reads via getMchInputChannelRms)
        for (int ch = 0; ch < bsr::mch::maxRendererInputChannels; ++ch)
        {
            const float blockRms = ch < usableInputChannels ? findBlockRms(inputPointers[ch], numSamples) : 0.0f;
            mchInputChannelRms[ch].store(
                smoothShortTermRms(mchInputChannelRms[ch].load(std::memory_order_relaxed), blockRms, numSamples, hostSampleRateHz),
                std::memory_order_relaxed);
        }

        std::array<bsr::mch::MchBinauralRenderer::PairRuntimeParameters, bsr::mch::maxPairSlots> pairRuntimeParams {};
        for (int slot = 0; slot < visibleSlotCount; ++slot)
        {
            auto& pairParams = pairRuntimeParams[static_cast<size_t>(slot)];

            const auto& cached = cachedPairParameters[static_cast<size_t>(slot)];
            if (cached.enable != nullptr)
                pairParams.enabled = cached.enable->load() >= 0.5f;
            if (cached.mute != nullptr)
                pairParams.muted = cached.mute->load() >= 0.5f;
            if (cached.solo != nullptr)
                pairParams.solo = cached.solo->load() >= 0.5f;

            const float pairGainDb = cached.gainDb != nullptr ? cached.gainDb->load() : 0.0f;
            pairParams.gainLinear = juce::Decibels::decibelsToGain(pairGainDb);

            const float pairPanPercent = cached.pan != nullptr ? cached.pan->load() : 0.0f;
            pairParams.pan = juce::jlimit(-1.0f, 1.0f, pairPanPercent / 100.0f);

            const float pairWetPercent = cached.wetDry != nullptr ? cached.wetDry->load() : 100.0f;
            pairParams.pairWet = juce::jlimit(0.0f, 1.0f, pairWetPercent / 100.0f);
        }

        // Render to output or to bypass scratch buffer depending on bypass state
        juce::AudioBuffer<float> bypassOutputBuffer;
        float* renderOutputL = nullptr;
        float* renderOutputR = nullptr;

        if (isBypassed)
        {
            // Still render to measure slot contributions, but don't sum into main output
            bypassOutputBuffer.setSize(2, numSamples, false, true, true);
            renderOutputL = bypassOutputBuffer.getWritePointer(0);
            renderOutputR = bypassOutputBuffer.getWritePointer(1);
        }
        else
        {
            renderOutputL = buffer.getWritePointer(0);
            renderOutputR = outputChannelCount > 1 ? buffer.getWritePointer(1) : renderOutputL;
        }

        const auto renderStats = mchRenderer.render(preset,
                                inputPointers,
                                usableInputChannels,
                                pairRuntimeParams,
                                meterWet,
                                outputGain,
                                renderOutputL,
                                renderOutputR,
                                numSamples);

        mchActivePairCount.store(renderStats.activePairs, std::memory_order_relaxed);
        mchVisiblePairCount.store(renderStats.visiblePairs, std::memory_order_relaxed);
        mchSkippedPairCount.store(renderStats.skippedPairs, std::memory_order_relaxed);
        mchActiveWetPairCount.store(renderStats.activeWetPairs, std::memory_order_relaxed);
        mchActiveDryPairCount.store(renderStats.activeDryPairs, std::memory_order_relaxed);
        mchActiveConvolutionPathCount.store(renderStats.activeConvolutionPaths, std::memory_order_relaxed);
        mchMaxConvolutionPathCount.store(renderStats.maxConvolutionPaths, std::memory_order_relaxed);
        // In bypass, renderStats already reflect dry-only slot contributions for metering.
        for (int slot = 0; slot < bsr::parameters::maxPairSlots; ++slot)
        {
            const float slotRmsL = renderStats.slotOutputRmsL[static_cast<size_t>(slot)];
            const float slotRmsR = renderStats.slotOutputRmsR[static_cast<size_t>(slot)];
            mchSlotOutputRmsL[static_cast<size_t>(slot)].store(
                smoothShortTermRms(mchSlotOutputRmsL[static_cast<size_t>(slot)].load(std::memory_order_relaxed),
                                   slotRmsL,
                                   numSamples,
                                   hostSampleRateHz),
                std::memory_order_relaxed);
            mchSlotOutputRmsR[static_cast<size_t>(slot)].store(
                smoothShortTermRms(mchSlotOutputRmsR[static_cast<size_t>(slot)].load(std::memory_order_relaxed),
                                   slotRmsR,
                                   numSamples,
                                   hostSampleRateHz),
                std::memory_order_relaxed);
        }

        const auto* inputL = usableInputChannels > 0 ? inputPointers[0] : nullptr;
        const auto* inputR = usableInputChannels > 1 ? inputPointers[1] : inputL;

        const float inPeakL = findAbsPeak(inputL, numSamples);
        const float inPeakR = findAbsPeak(inputR, numSamples);
        const float inBlockRmsL = findBlockRms(inputL, numSamples);
        const float inBlockRmsR = findBlockRms(inputR, numSamples);

        auto* outputL = buffer.getWritePointer(0);
        auto* outputR = outputChannelCount > 1 ? buffer.getWritePointer(1) : outputL;

        const float outPeakL = findAbsPeak(outputL, numSamples);
        const float outPeakR = findAbsPeak(outputR, numSamples);
        const float outBlockRmsL = findBlockRms(outputL, numSamples);
        const float outBlockRmsR = findBlockRms(outputR, numSamples);

        inputRmsL.store(smoothShortTermRms(inputRmsL.load(std::memory_order_relaxed), inBlockRmsL, numSamples, hostSampleRateHz),
                        std::memory_order_relaxed);
        inputRmsR.store(smoothShortTermRms(inputRmsR.load(std::memory_order_relaxed), inBlockRmsR, numSamples, hostSampleRateHz),
                        std::memory_order_relaxed);
        outputRmsL.store(smoothShortTermRms(outputRmsL.load(std::memory_order_relaxed), outBlockRmsL, numSamples, hostSampleRateHz),
                         std::memory_order_relaxed);
        outputRmsR.store(smoothShortTermRms(outputRmsR.load(std::memory_order_relaxed), outBlockRmsR, numSamples, hostSampleRateHz),
                         std::memory_order_relaxed);
        inputPeakL.store(inPeakL, std::memory_order_relaxed);
        inputPeakR.store(inPeakR, std::memory_order_relaxed);
        outputPeakL.store(outPeakL, std::memory_order_relaxed);
        outputPeakR.store(outPeakR, std::memory_order_relaxed);

        constexpr float clipThreshold = 0.89125094f; // -1 dBFS
        if (outPeakL >= clipThreshold || outPeakR >= clipThreshold)
            clipWarningHoldBlocks.store(24, std::memory_order_relaxed);
        else
            clipWarningHoldBlocks.store(juce::jmax(0, clipWarningHoldBlocks.load(std::memory_order_relaxed) - 1),
                                        std::memory_order_relaxed);

        if (isBypassed)
        {
            // Bypass: pass input L/R straight through (already in buffer channels 0/1)
            // Slot measurements are already updated from rendering above
        }

        return;
    }

    // Clear extra output channels
    for (auto channel = this->getTotalNumInputChannels(); channel < this->getTotalNumOutputChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

    mchActivePairCount.store(1, std::memory_order_relaxed);
    mchVisiblePairCount.store(1, std::memory_order_relaxed);
    mchSkippedPairCount.store(0, std::memory_order_relaxed);
    mchActiveWetPairCount.store(1, std::memory_order_relaxed);
    mchActiveDryPairCount.store(1, std::memory_order_relaxed);
    mchActiveConvolutionPathCount.store(4, std::memory_order_relaxed);
    mchMaxConvolutionPathCount.store(4, std::memory_order_relaxed);

    // Get parameters
    bool bypass = bypassParam != nullptr && bypassParam->load() > 0.5f;
    float wetDryPercent = wetDryParam != nullptr ? wetDryParam->load() : 100.0f;
    float outputGainDb = outputGainParam != nullptr ? outputGainParam->load() : 0.0f;
    float inputPanPercent = inputPanParam != nullptr ? inputPanParam->load() : 0.0f;

    // Convert wet/dry from percentage to 0-1 range
    float wetDryMix = juce::jlimit(0.0f, 1.0f, wetDryPercent / 100.0f);
    float outputGain = juce::Decibels::decibelsToGain(outputGainDb);
    const float pan = juce::jlimit(-1.0f, 1.0f, inputPanPercent / 100.0f);
    const float inputLGain = pan > 0.0f ? (1.0f - pan) : 1.0f;
    const float inputRGain = pan < 0.0f ? (1.0f + pan) : 1.0f;

    // Get input/output pointers
    const auto* inputL = inputChannelCount > 0 ? buffer.getReadPointer(0) : nullptr;
    const auto* inputR = this->getTotalNumInputChannels() > 1 ? buffer.getReadPointer(1) : inputL;
    auto* outputL = buffer.getWritePointer(0);
    auto* outputR = this->getTotalNumOutputChannels() > 1 ? buffer.getWritePointer(1) : outputL;

    if (inputL == nullptr)
    {
        juce::FloatVectorOperations::clear(outputL, buffer.getNumSamples());
        if (outputR != outputL)
            juce::FloatVectorOperations::clear(outputR, buffer.getNumSamples());
        return;
    }

    const bool hasPanScratch = pannedInputBuffer.getNumChannels() >= 2
                               && pannedInputBuffer.getNumSamples() >= buffer.getNumSamples();
    jassert(hasPanScratch);

    const float* procInputL = inputL;
    const float* procInputR = inputR;
    if (hasPanScratch)
    {
        auto* pannedL = pannedInputBuffer.getWritePointer(0);
        auto* pannedR = pannedInputBuffer.getWritePointer(1);
        juce::FloatVectorOperations::copy(pannedL, inputL, buffer.getNumSamples());
        juce::FloatVectorOperations::copy(pannedR, inputR, buffer.getNumSamples());
        juce::FloatVectorOperations::multiply(pannedL, inputLGain, buffer.getNumSamples());
        juce::FloatVectorOperations::multiply(pannedR, inputRGain, buffer.getNumSamples());
        procInputL = pannedL;
        procInputR = pannedR;
    }

    applyPendingIRUpdateIfAvailable();

    // Meter taps are read-only snapshots and do not alter DSP output.
    const float inPeakL = findAbsPeak(procInputL, buffer.getNumSamples());
    const float inPeakR = findAbsPeak(procInputR, buffer.getNumSamples());
    const float inBlockRmsL = findBlockRms(procInputL, buffer.getNumSamples());
    const float inBlockRmsR = findBlockRms(procInputR, buffer.getNumSamples());

    convolver.process(procInputL, procInputR, outputL, outputR, buffer.getNumSamples(), wetDryMix, outputGain, bypass);
    const float outPeakL = findAbsPeak(outputL, buffer.getNumSamples());
    const float outPeakR = findAbsPeak(outputR, buffer.getNumSamples());
    const float outBlockRmsL = findBlockRms(outputL, buffer.getNumSamples());
    const float outBlockRmsR = findBlockRms(outputR, buffer.getNumSamples());

    const float smoothedInRmsL = smoothShortTermRms(inputRmsL.load(std::memory_order_relaxed),
                                                    inBlockRmsL,
                                                    buffer.getNumSamples(),
                                                    hostSampleRateHz);
    const float smoothedInRmsR = smoothShortTermRms(inputRmsR.load(std::memory_order_relaxed),
                                                    inBlockRmsR,
                                                    buffer.getNumSamples(),
                                                    hostSampleRateHz);
    const float smoothedOutRmsL = smoothShortTermRms(outputRmsL.load(std::memory_order_relaxed),
                                                     outBlockRmsL,
                                                     buffer.getNumSamples(),
                                                     hostSampleRateHz);
    const float smoothedOutRmsR = smoothShortTermRms(outputRmsR.load(std::memory_order_relaxed),
                                                     outBlockRmsR,
                                                     buffer.getNumSamples(),
                                                     hostSampleRateHz);

    inputRmsL.store(smoothedInRmsL, std::memory_order_relaxed);
    inputRmsR.store(smoothedInRmsR, std::memory_order_relaxed);
    outputRmsL.store(smoothedOutRmsL, std::memory_order_relaxed);
    outputRmsR.store(smoothedOutRmsR, std::memory_order_relaxed);
    inputPeakL.store(inPeakL, std::memory_order_relaxed);
    inputPeakR.store(inPeakR, std::memory_order_relaxed);
    outputPeakL.store(outPeakL, std::memory_order_relaxed);
    outputPeakR.store(outPeakR, std::memory_order_relaxed);

    constexpr float clipThreshold = 0.89125094f; // -1 dBFS
    if (outPeakL >= clipThreshold || outPeakR >= clipThreshold)
        clipWarningHoldBlocks.store(24, std::memory_order_relaxed);
    else
        clipWarningHoldBlocks.store(juce::jmax(0, clipWarningHoldBlocks.load(std::memory_order_relaxed) - 1),
                                    std::memory_order_relaxed);

    juce::ignoreUnused(buffer);
}

juce::AudioProcessorEditor* BinauralSpeakerRoomAudioProcessor::createEditor()
{
    return new BinauralSpeakerRoomAudioProcessorEditor(*this);
}

bool BinauralSpeakerRoomAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String BinauralSpeakerRoomAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool BinauralSpeakerRoomAudioProcessor::acceptsMidi() const
{
    return false;
}

bool BinauralSpeakerRoomAudioProcessor::producesMidi() const
{
    return false;
}

bool BinauralSpeakerRoomAudioProcessor::isMidiEffect() const
{
    return false;
}

double BinauralSpeakerRoomAudioProcessor::getTailLengthSeconds() const
{
    const auto convolverLatencySamples = convolver.getLatencySamples();
    if (convolverLatencySamples <= 0 || this->getSampleRate() <= 0.0)
        return 0.0;
    return static_cast<double>(convolverLatencySamples) / this->getSampleRate();
}

int BinauralSpeakerRoomAudioProcessor::getNumPrograms()
{
    return 1;
}

int BinauralSpeakerRoomAudioProcessor::getCurrentProgram()
{
    return 0;
}

void BinauralSpeakerRoomAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String BinauralSpeakerRoomAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void BinauralSpeakerRoomAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void BinauralSpeakerRoomAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("schemaVersion", 1, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void BinauralSpeakerRoomAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary(data, sizeInBytes))
    {
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }

    // Restore IR path after state is loaded
    restoreIRPathFromState();
    restoreMchSlotIRPathsFromState();
}

juce::AudioProcessorValueTreeState& BinauralSpeakerRoomAudioProcessor::getValueTreeState() noexcept
{
    return apvts;
}

bool BinauralSpeakerRoomAudioProcessor::loadMchSlotIRFile(int slotIndex, const juce::File& filePath) noexcept
{
    juce::String errorMessage;
    if (!mchSlotIrManager.loadSlotIrFile(slotIndex, filePath, errorMessage))
    {
        lastLoadError = errorMessage;
        lastWarningMessage = errorMessage;
        return false;
    }

    lastLoadError.clear();
    lastWarningMessage.clear();
    apvts.state.setProperty(bsr::parameters::pairIrPathStateKey(slotIndex), filePath.getFullPathName(), nullptr);
    applySlotIrToRenderer(slotIndex);

    return true;
}

bool BinauralSpeakerRoomAudioProcessor::isMchSlotIRLoaded(int slotIndex) const noexcept
{
    return mchSlotIrManager.isSlotLoaded(slotIndex);
}

juce::String BinauralSpeakerRoomAudioProcessor::getMchSlotStatusText(int slotIndex) const noexcept
{
    const auto status = mchSlotIrManager.getSlotStatus(slotIndex);
    const auto error = mchSlotIrManager.getSlotError(slotIndex);

    switch (status)
    {
        case bsr::mch::SlotIrStatus::loaded:
            return "[OK] " + mchSlotIrManager.getSlotMetadataText(slotIndex);
        case bsr::mch::SlotIrStatus::invalid:
            return error.isEmpty() ? "[INVALID]" : ("[INVALID] " + error);
        case bsr::mch::SlotIrStatus::error:
            return error.isEmpty() ? "[ERROR]" : ("[ERROR] " + error);
        case bsr::mch::SlotIrStatus::missing:
        default:
            return "Missing IR";
    }
}

bool BinauralSpeakerRoomAudioProcessor::loadIRFile(const juce::File& filePath) noexcept
{
    juce::String errorMsg;
    if (!irLoader.loadWavFile(filePath, errorMsg))
    {
        lastLoadError = errorMsg;
        lastWarningMessage = errorMsg;
        apvts.state.setProperty("irLastError", lastLoadError, nullptr);
        return false;
    }

    lastLoadError.clear();
    lastWarningMessage.clear();
    apvts.state.setProperty("irLastError", juce::String{}, nullptr);

    if (!queueLoadedIRForConvolver())
    {
        lastLoadError = "Failed to stage IR for processing";
        lastWarningMessage = lastLoadError;
        apvts.state.setProperty("irLastError", lastLoadError, nullptr);
        return false;
    }

    auto* pathParam = dynamic_cast<juce::AudioParameterBool*>(
        apvts.getParameter(bsr::parameters::irPath));
    if (pathParam != nullptr)
        pathParam->setValueNotifyingHost(!pathParam->get());  // Toggle to signal host state changed

    // Update APVTS state with the path
    apvts.state.setProperty(bsr::parameters::irPath, filePath.getFullPathName(), nullptr);

    return true;
}

void BinauralSpeakerRoomAudioProcessor::restoreIRPathFromState() noexcept
{
    auto lastErrorVar = apvts.state.getProperty("irLastError");
    if (lastErrorVar.isString())
        lastLoadError = lastErrorVar.toString();

    auto irPathVar = apvts.state.getProperty(bsr::parameters::irPath);
    if (irPathVar.isString())
    {
        const auto pathStr = normalisePersistedPathString(irPathVar.toString());
        if (!pathStr.isEmpty())
        {
            const auto irFile = makeFileFromPersistedPath(pathStr);
            if (irFile.existsAsFile())
            {
                juce::String errorMsg;
                if (irLoader.loadWavFile(irFile, errorMsg))
                {
                    if (queueLoadedIRForConvolver())
                    {
                        apvts.state.setProperty(bsr::parameters::irPath, irFile.getFullPathName(), nullptr);
                        lastLoadError.clear();
                        lastWarningMessage.clear();
                        apvts.state.setProperty("irLastError", juce::String{}, nullptr);
                    }
                    else
                    {
                        lastLoadError = "Restored IR is invalid and was ignored";
                        lastWarningMessage = lastLoadError;
                        apvts.state.setProperty("irLastError", lastLoadError, nullptr);
                    }
                }
                else
                {
                    lastLoadError = errorMsg;
                    lastWarningMessage = errorMsg;
                    apvts.state.setProperty("irLastError", lastLoadError, nullptr);
                }
            }
            else
            {
                lastLoadError = "Missing saved IR file: " + irFile.getFullPathName();
                lastWarningMessage = lastLoadError;
                apvts.state.setProperty("irLastError", lastLoadError, nullptr);
            }
        }
    }

    if (!lastLoadError.isEmpty() && lastWarningMessage.isEmpty())
        lastWarningMessage = lastLoadError;
}

void BinauralSpeakerRoomAudioProcessor::restoreMchSlotIRPathsFromState() noexcept
{
    for (int slotIndex = 0; slotIndex < bsr::parameters::maxPairSlots; ++slotIndex)
    {
        const auto stateKey = bsr::parameters::pairIrPathStateKey(slotIndex);
        const auto pathValue = apvts.state.getProperty(stateKey);
        if (!pathValue.isString())
        {
            mchSlotIrManager.clearSlot(slotIndex);
            applySlotIrToRenderer(slotIndex);
            continue;
        }

        const auto path = normalisePersistedPathString(pathValue.toString());
        if (path != pathValue.toString())
            apvts.state.setProperty(stateKey, path, nullptr);

        juce::String errorMessage;
        if (!mchSlotIrManager.restoreSlotFromPath(slotIndex, path, errorMessage) && !errorMessage.isEmpty())
            lastWarningMessage = errorMessage;

        applySlotIrToRenderer(slotIndex);
    }
}

void BinauralSpeakerRoomAudioProcessor::initialiseCachedParameterPointers() noexcept
{
    modeParam = apvts.getRawParameterValue(bsr::parameters::renderMode);
    layoutParam = apvts.getRawParameterValue(bsr::parameters::layoutPreset);
    wetDryParam = apvts.getRawParameterValue(bsr::parameters::wetDry);
    outputGainParam = apvts.getRawParameterValue(bsr::parameters::outputGainDb);
    bypassParam = apvts.getRawParameterValue(bsr::parameters::bypass);
    inputPanParam = apvts.getRawParameterValue(bsr::parameters::inputPan);

    for (int slotIndex = 0; slotIndex < bsr::parameters::maxPairSlots; ++slotIndex)
    {
        auto& cached = cachedPairParameters[static_cast<size_t>(slotIndex)];
        cached.enable = apvts.getRawParameterValue(bsr::parameters::pairEnable(slotIndex));
        cached.mute = apvts.getRawParameterValue(bsr::parameters::pairMute(slotIndex));
        cached.solo = apvts.getRawParameterValue(bsr::parameters::pairSolo(slotIndex));
        cached.gainDb = apvts.getRawParameterValue(bsr::parameters::pairGainDb(slotIndex));
        cached.pan = apvts.getRawParameterValue(bsr::parameters::pairPan(slotIndex));
        cached.wetDry = apvts.getRawParameterValue(bsr::parameters::pairWetDry(slotIndex));
    }
}

void BinauralSpeakerRoomAudioProcessor::applySlotIrToRenderer(int slotIndex) noexcept
{
    mchRenderer.applySlotIrSnapshot(slotIndex, mchSlotIrManager.getSlotSnapshot(slotIndex));
}

void BinauralSpeakerRoomAudioProcessor::applyAllSlotIrsToRenderer() noexcept
{
    mchRenderer.applyAllSlotIrSnapshots(mchSlotIrManager);
}

juce::String BinauralSpeakerRoomAudioProcessor::getPluginStateText() const noexcept
{
    auto* modeValue = apvts.getRawParameterValue(bsr::parameters::renderMode);
    const bool mchMode = modeValue != nullptr && modeValue->load() >= 0.5f;

    if (mchMode)
    {
        auto* layoutValue = apvts.getRawParameterValue(bsr::parameters::layoutPreset);
        const int layoutIndex = layoutValue != nullptr ? juce::roundToInt(layoutValue->load()) : 0;
        const auto& preset = bsr::mch::getLayoutPresetByChoiceIndex(layoutIndex);

        int loadedCount = 0;
        for (int i = 0; i < preset.visibleSlotCount; ++i)
            if (mchSlotIrManager.isSlotLoaded(i))
                ++loadedCount;

        if (loadedCount == preset.visibleSlotCount)
            return "[OK] MCH mode - all " + juce::String(loadedCount) + " slots loaded";
        if (loadedCount > 0)
            return "[PARTIAL] MCH mode - " + juce::String(loadedCount) + "/" + juce::String(preset.visibleSlotCount) + " slots loaded";
        return "MCH mode - no slots loaded";
    }

    if (isIRLoaded())
    {
        const auto& metadata = getIRMetadata();
        const auto durationMs = metadata.getDurationMs();
        const auto durationSec = durationMs / 1000.0;
        return "[OK] " + metadata.fileName + juce::String::formatted(
            " (%d-ch, %d Hz, %.2fs)",
            metadata.channels,
            metadata.sampleRate,
            durationSec);
    }

    if (!lastLoadError.isEmpty())
        return "[ERROR] " + lastLoadError;

    return "No IR loaded";
}

juce::String BinauralSpeakerRoomAudioProcessor::getWarningText() const noexcept
{
    juce::String warning;

    auto* modeValueEarly = apvts.getRawParameterValue(bsr::parameters::renderMode);
    const bool mchModeEarly = modeValueEarly != nullptr && modeValueEarly->load() >= 0.5f;

    if (!lastWarningMessage.isEmpty())
        warning = lastWarningMessage;
    else if (!mchModeEarly && !isIRLoaded())
        warning = "No IR loaded - passing dry audio";

    const auto& metadata = getIRMetadata();
    if (metadata.sampleRate > 0 && hostSampleRateHz > 0.0
        && std::abs(static_cast<double>(metadata.sampleRate) - hostSampleRateHz) > 0.5)
    {
        const auto mismatch = juce::String::formatted("Sample-rate mismatch: IR %d Hz vs host %.0f Hz",
                                                      metadata.sampleRate,
                                                      hostSampleRateHz);
        warning = warning.isEmpty() ? mismatch : (warning + " | " + mismatch);
    }

    if (clipWarningHoldBlocks.load(std::memory_order_relaxed) > 0)
        warning = warning.isEmpty() ? "Output clipping risk: peak above -1 dBFS"
                                    : (warning + " | Output clipping risk: peak above -1 dBFS");

    auto* modeValue = apvts.getRawParameterValue(bsr::parameters::renderMode);
    const bool mchMode = modeValue != nullptr && modeValue->load() >= 0.5f;
    if (mchMode)
    {
        auto* layoutValue = apvts.getRawParameterValue(bsr::parameters::layoutPreset);
        const int layoutIndex = layoutValue != nullptr ? juce::roundToInt(layoutValue->load()) : 0;
        const auto& preset = bsr::mch::getLayoutPresetByChoiceIndex(layoutIndex);

        juce::StringArray missingSlotWarnings;
        for (int slotIndex = 0; slotIndex < preset.visibleSlotCount; ++slotIndex)
        {
            const auto* enabledValue = apvts.getRawParameterValue(bsr::parameters::pairEnable(slotIndex));
            const bool enabled = enabledValue == nullptr || enabledValue->load() >= 0.5f;
            if (!enabled)
                continue;

            if (mchSlotIrManager.isSlotLoaded(slotIndex))
                continue;

            const auto& slot = preset.slots[static_cast<size_t>(slotIndex)];
            const auto slotStatus = getMchSlotStatusText(slotIndex);
            missingSlotWarnings.add("Pair " + juce::String(slotIndex + 1)
                                    + " (" + juce::String(slot.pairName) + ") "
                                    + slotStatus + " - wet contribution muted");
        }

        if (!missingSlotWarnings.isEmpty())
        {
            const auto mchWarning = missingSlotWarnings.joinIntoString(" | ");
            warning = warning.isEmpty() ? mchWarning : (warning + " | " + mchWarning);
        }
    }

    return warning;
}

BinauralSpeakerRoomAudioProcessor::MeterSnapshot BinauralSpeakerRoomAudioProcessor::getMeterSnapshot() const noexcept
{
    MeterSnapshot snapshot;
    snapshot.inputPeakL = inputPeakL.load(std::memory_order_relaxed);
    snapshot.inputPeakR = inputPeakR.load(std::memory_order_relaxed);
    snapshot.outputPeakL = outputPeakL.load(std::memory_order_relaxed);
    snapshot.outputPeakR = outputPeakR.load(std::memory_order_relaxed);
    snapshot.inputRmsL = inputRmsL.load(std::memory_order_relaxed);
    snapshot.inputRmsR = inputRmsR.load(std::memory_order_relaxed);
    snapshot.outputRmsL = outputRmsL.load(std::memory_order_relaxed);
    snapshot.outputRmsR = outputRmsR.load(std::memory_order_relaxed);
    snapshot.clipWarning = clipWarningHoldBlocks.load(std::memory_order_relaxed) > 0;
    return snapshot;
}

bool BinauralSpeakerRoomAudioProcessor::queueLoadedIRForConvolver() noexcept
{
    const auto& metadata = irLoader.getMetadata();
    if (metadata.channels != 4 || metadata.numSamples <= 0)
        return false;

    auto data = std::make_shared<IRTransferData>();
    data->numSamples = metadata.numSamples;

    for (int channel = 0; channel < 4; ++channel)
    {
        const float* channelData = irLoader.getChannelData(channel);
        if (channelData == nullptr)
            return false;

        auto& dst = data->channels[static_cast<size_t>(channel)];
        dst.assign(channelData, channelData + metadata.numSamples);
    }

    {
        const juce::SpinLock::ScopedLockType lock(pendingIRDataLock);
        pendingIRData = std::move(data);
    }

    return true;
}

void BinauralSpeakerRoomAudioProcessor::applyPendingIRUpdateIfAvailable() noexcept
{
    std::shared_ptr<IRTransferData> data;
    {
        const juce::SpinLock::ScopedLockType lock(pendingIRDataLock);
        data = std::move(pendingIRData);
        pendingIRData.reset();
    }

    if (!data || data->numSamples <= 0)
        return;

    convolver.loadIR(data->channels[0].data(),
                     data->channels[1].data(),
                     data->channels[2].data(),
                     data->channels[3].data(),
                     data->numSamples);
}

void BinauralSpeakerRoomAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    // Sync global pan/gain/wet-dry to all visible slot parameters
    if (parameterID == bsr::parameters::inputPan)
    {
        for (int slot = 0; slot < bsr::parameters::maxPairSlots; ++slot)
        {
            auto* param = apvts.getParameter(bsr::parameters::pairPan(slot));
            if (param != nullptr)
                param->setValueNotifyingHost(param->convertTo0to1(newValue));
        }
    }
    else if (parameterID == bsr::parameters::outputGainDb)
    {
        for (int slot = 0; slot < bsr::parameters::maxPairSlots; ++slot)
        {
            auto* param = apvts.getParameter(bsr::parameters::pairGainDb(slot));
            if (param != nullptr)
                param->setValueNotifyingHost(param->convertTo0to1(newValue));
        }
    }
    else if (parameterID == bsr::parameters::wetDry)
    {
        for (int slot = 0; slot < bsr::parameters::maxPairSlots; ++slot)
        {
            auto* param = apvts.getParameter(bsr::parameters::pairWetDry(slot));
            if (param != nullptr)
                param->setValueNotifyingHost(param->convertTo0to1(newValue));
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BinauralSpeakerRoomAudioProcessor();
}
