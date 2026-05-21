#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <memory>

#include "BinauralConvolver.h"
#include "IRLoader.h"
#include "MchBinauralRenderer.h"
#include "MchSlotIrManager.h"
#include "Parameters.h"

class BinauralSpeakerRoomAudioProcessor final : public juce::AudioProcessor,
                                              private juce::AudioProcessorValueTreeState::Listener
{
public:
    struct MeterSnapshot
    {
        float inputPeakL = 0.0f;
        float inputPeakR = 0.0f;
        float outputPeakL = 0.0f;
        float outputPeakR = 0.0f;
        float inputRmsL = 0.0f;
        float inputRmsR = 0.0f;
        float outputRmsL = 0.0f;
        float outputRmsR = 0.0f;
        bool clipWarning = false;
    };

    BinauralSpeakerRoomAudioProcessor();
    ~BinauralSpeakerRoomAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept;

    /// @brief Attempt to load an IR file
    /// @param filePath Path to the WAV file
    /// @return true if successfully loaded, false otherwise (error message available via getLastLoadError)
    bool loadIRFile(const juce::File& filePath) noexcept;

    /// @brief Get the last IR loading error message
    juce::String getLastLoadError() const noexcept { return lastLoadError; }

    /// @brief Get current IR metadata
    const IRMetadata& getIRMetadata() const noexcept { return irLoader.getMetadata(); }

    /// @brief Check if an IR is currently loaded and valid
    bool isIRLoaded() const noexcept { return irLoader.isLoaded(); }

    /// @brief Get the loaded IR file path
    juce::String getLoadedIRPath() const noexcept { return irLoader.getLoadedPath(); }

    /// @brief Get host sample rate used by the processor
    double getHostSampleRateHz() const noexcept { return hostSampleRateHz; }

    /// @brief Get current high-level plugin state for diagnostics
    juce::String getPluginStateText() const noexcept;

    /// @brief Get diagnostics warning text
    juce::String getWarningText() const noexcept;

    bool loadMchSlotIRFile(int slotIndex, const juce::File& filePath) noexcept;
    bool isMchSlotIRLoaded(int slotIndex) const noexcept;
    juce::String getMchSlotStatusText(int slotIndex) const noexcept;

    int getMchActivePairCount() const noexcept { return mchActivePairCount.load(std::memory_order_relaxed); }
    int getMchVisiblePairCount() const noexcept { return mchVisiblePairCount.load(std::memory_order_relaxed); }
    int getMchSkippedPairCount() const noexcept { return mchSkippedPairCount.load(std::memory_order_relaxed); }
    int getMchActiveWetPairCount() const noexcept { return mchActiveWetPairCount.load(std::memory_order_relaxed); }
    int getMchActiveDryPairCount() const noexcept { return mchActiveDryPairCount.load(std::memory_order_relaxed); }
    int getMchActiveConvolutionPathCount() const noexcept { return mchActiveConvolutionPathCount.load(std::memory_order_relaxed); }
    int getMchMaxConvolutionPathCount() const noexcept { return mchMaxConvolutionPathCount.load(std::memory_order_relaxed); }

    int getLastSeenInputChannels() const noexcept { return lastSeenInputChannels.load(std::memory_order_relaxed); }
    int getLastSeenOutputChannels() const noexcept { return lastSeenOutputChannels.load(std::memory_order_relaxed); }

    /// @brief Read the most recent meter/clip snapshot
    MeterSnapshot getMeterSnapshot() const noexcept;

    /// @brief Input RMS for a single host channel (0-based) used by per-slot level meters
    float getMchInputChannelRms(int channel) const noexcept
    {
        if (channel < 0 || channel >= static_cast<int>(mchInputChannelRms.size()))
            return 0.0f;
        return mchInputChannelRms[static_cast<size_t>(channel)].load(std::memory_order_relaxed);
    }

    /// @brief Per-slot stereo contribution RMS (post slot controls, post global output gain)
    float getMchSlotOutputRmsL(int slot) const noexcept
    {
        if (slot < 0 || slot >= static_cast<int>(mchSlotOutputRmsL.size()))
            return 0.0f;
        return mchSlotOutputRmsL[static_cast<size_t>(slot)].load(std::memory_order_relaxed);
    }

    /// @brief Per-slot stereo contribution RMS (post slot controls, post global output gain)
    float getMchSlotOutputRmsR(int slot) const noexcept
    {
        if (slot < 0 || slot >= static_cast<int>(mchSlotOutputRmsR.size()))
            return 0.0f;
        return mchSlotOutputRmsR[static_cast<size_t>(slot)].load(std::memory_order_relaxed);
    }

private:
    struct CachedPairParameters
    {
        std::atomic<float>* enable = nullptr;
        std::atomic<float>* mute = nullptr;
        std::atomic<float>* solo = nullptr;
        std::atomic<float>* gainDb = nullptr;
        std::atomic<float>* pan = nullptr;
        std::atomic<float>* wetDry = nullptr;
    };

    struct IRTransferData
    {
        std::array<std::vector<float>, 4> channels;
        int numSamples = 0;
    };

    juce::AudioProcessorValueTreeState apvts;
    IRLoader irLoader;
    BinauralConvolver convolver;
    juce::String lastLoadError;
    juce::String lastWarningMessage;
    double hostSampleRateHz = 0.0;

    std::atomic<float> inputRmsL { 0.0f };
    std::atomic<float> inputRmsR { 0.0f };
    std::atomic<float> outputRmsL { 0.0f };
    std::atomic<float> outputRmsR { 0.0f };
    std::atomic<float> inputPeakL { 0.0f };
    std::atomic<float> inputPeakR { 0.0f };
    std::atomic<float> outputPeakL { 0.0f };
    std::atomic<float> outputPeakR { 0.0f };
    std::atomic<int> clipWarningHoldBlocks { 0 };
    std::atomic<int> mchActivePairCount { 0 };
    std::atomic<int> mchVisiblePairCount { 0 };
    std::atomic<int> mchSkippedPairCount { 0 };
    std::atomic<int> mchActiveWetPairCount { 0 };
    std::atomic<int> mchActiveDryPairCount { 0 };
    std::atomic<int> mchActiveConvolutionPathCount { 0 };
    std::atomic<int> mchMaxConvolutionPathCount { 0 };
    std::atomic<int> lastSeenInputChannels { 0 };
    std::atomic<int> lastSeenOutputChannels { 0 };
    std::array<std::atomic<float>, bsr::mch::maxRendererInputChannels> mchInputChannelRms {};
    std::array<std::atomic<float>, bsr::parameters::maxPairSlots> mchSlotOutputRmsL {};
    std::array<std::atomic<float>, bsr::parameters::maxPairSlots> mchSlotOutputRmsR {};
    std::atomic<std::shared_ptr<IRTransferData>> pendingIRData;
    juce::AudioBuffer<float> pannedInputBuffer;
    juce::AudioBuffer<float> mchInputScratchBuffer;
    bsr::mch::SlotIrManager mchSlotIrManager;
    bsr::mch::MchBinauralRenderer mchRenderer;
    std::atomic<float>* modeParam = nullptr;
    std::atomic<float>* layoutParam = nullptr;
    std::atomic<float>* wetDryParam = nullptr;
    std::atomic<float>* outputGainParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* inputPanParam = nullptr;
    std::array<CachedPairParameters, bsr::parameters::maxPairSlots> cachedPairParameters {};

    void restoreIRPathFromState() noexcept;
    void restoreMchSlotIRPathsFromState() noexcept;
    void initialiseCachedParameterPointers() noexcept;
    void applySlotIrToRenderer(int slotIndex) noexcept;
    void applyAllSlotIrsToRenderer() noexcept;
    bool queueLoadedIRForConvolver() noexcept;
    void applyPendingIRUpdateIfAvailable() noexcept;

    // AudioProcessorValueTreeState::Listener interface for syncing global controls to slot parameters
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BinauralSpeakerRoomAudioProcessor)
};
