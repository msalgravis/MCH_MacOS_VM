#pragma once

#include <JuceHeader.h>

#include "../Source/LayoutPreset.h"
#include "../Source/MchBinauralRenderer.h"
#include "../Source/MchSlotIrManager.h"

class MchBinauralRendererTests : public juce::UnitTest
{
public:
    MchBinauralRendererTests() : juce::UnitTest("MchBinauralRenderer", "MCH") {}

    void runTest() override
    {
        beginTest("Quad routing and hard pan behavior");
        testRoutingAndPan();

        beginTest("Solo and mute determine audible pairs and summing");
        testSoloMuteAndSumming();

        beginTest("Global wet and pair wet multiplication");
        testWetModel();

        beginTest("Performance matrix path scheduling and skip behavior");
        testPerformanceMatrixScheduling();

        beginTest("Hot reload applies updated slot content");
        testHotReloadVersionedUpdate();

        beginTest("5.1 full chain: white noise through real quad IR produces non-zero stereo output");
        test5Point1FullChainWithRealIr();
    }

private:
    static constexpr int numSamples = 32;

    static bool writeQuadImpulseIr(const juce::File& file,
                                   double sampleRate,
                                   const std::array<float, 4>& gains,
                                   int irNumSamples = 1)
    {
        if (irNumSamples <= 0)
            return false;

        auto stream = file.createOutputStream();
        if (stream == nullptr)
            return false;

        juce::WavAudioFormat format;
        auto writer = std::unique_ptr<juce::AudioFormatWriter>(
            format.createWriterFor(stream.get(), sampleRate, 4, 24, {}, 0));
        if (writer == nullptr)
            return false;

        stream.release();

        juce::AudioBuffer<float> buffer(4, irNumSamples);
        buffer.clear();
        for (int ch = 0; ch < 4; ++ch)
            buffer.setSample(ch, 0, gains[static_cast<size_t>(ch)]);

        return writer->writeFromAudioSampleBuffer(buffer, 0, irNumSamples);
    }

    static std::array<const float*, bsr::mch::maxRendererInputChannels> makeInputPointers(
        const std::array<std::vector<float>, bsr::mch::maxRendererInputChannels>& channels)
    {
        std::array<const float*, bsr::mch::maxRendererInputChannels> pointers {};
        for (int i = 0; i < bsr::mch::maxRendererInputChannels; ++i)
            pointers[static_cast<size_t>(i)] = channels[static_cast<size_t>(i)].data();
        return pointers;
    }

    static std::array<std::vector<float>, bsr::mch::maxRendererInputChannels> makeSilentInputs()
    {
        std::array<std::vector<float>, bsr::mch::maxRendererInputChannels> channels;
        for (auto& channel : channels)
            channel.assign(numSamples, 0.0f);
        return channels;
    }

    static std::array<bsr::mch::MchBinauralRenderer::PairRuntimeParameters, bsr::mch::maxPairSlots> makeDefaultPairParams()
    {
        std::array<bsr::mch::MchBinauralRenderer::PairRuntimeParameters, bsr::mch::maxPairSlots> pairParams {};
        for (auto& pair : pairParams)
        {
            pair.enabled = true;
            pair.muted = false;
            pair.solo = false;
            pair.gainLinear = 1.0f;
            pair.pan = 0.0f;
            pair.pairWet = 1.0f;
        }
        return pairParams;
    }

    void testRoutingAndPan()
    {
        juce::TemporaryFile irFile(".wav");
        expect(writeQuadImpulseIr(irFile.getFile(), 48000.0, { 0.25f, 0.5f, 0.75f, 1.0f }), "Should write routing IR");

        bsr::mch::SlotIrManager irManager;
        irManager.setHostSampleRate(48000.0);
        juce::String error;
        expect(irManager.loadSlotIrFile(0, irFile.getFile(), error), "Should load slot IR for routing test");
        expect(error.isEmpty(), "Slot IR should load without errors");

        bsr::mch::MchBinauralRenderer renderer;
        renderer.prepareToPlay(48000.0, numSamples);
        renderer.applyAllSlotIrSnapshots(irManager);

        auto inputs = makeSilentInputs();
        inputs[0][0] = 1.0f;
        inputs[1][0] = 1.0f;
        auto inputPointers = makeInputPointers(inputs);

        auto pairParams = makeDefaultPairParams();
        const auto& stereo = bsr::mch::getLayoutPresetByChoiceIndex(0);

        std::vector<float> outputL(numSamples, 0.0f);
        std::vector<float> outputR(numSamples, 0.0f);

        auto stats = renderer.render(stereo,
                                     inputPointers,
                                     2,
                                     pairParams,
                                     1.0f,
                                     1.0f,
                                     outputL.data(),
                                     outputR.data(),
                                     numSamples);

        expectWithinAbsoluteError(outputL[0], 1.0f, 0.0001f, "Out L should be A*LL + B*RL");
        expectWithinAbsoluteError(outputR[0], 1.5f, 0.0001f, "Out R should be A*LR + B*RR");
        expectEquals(stats.activeConvolutionPaths, 4, "Centered pan should use all 4 paths");

        std::fill(outputL.begin(), outputL.end(), 0.0f);
        std::fill(outputR.begin(), outputR.end(), 0.0f);
        pairParams[0].pan = -1.0f;

        stats = renderer.render(stereo,
                                inputPointers,
                                2,
                                pairParams,
                                1.0f,
                                1.0f,
                                outputL.data(),
                                outputR.data(),
                                numSamples);

        expectWithinAbsoluteError(outputL[0], 0.25f, 0.0001f, "Hard A pan should keep only A LL");
        expectWithinAbsoluteError(outputR[0], 0.5f, 0.0001f, "Hard A pan should keep only A LR");
        expectEquals(stats.activeConvolutionPaths, 2, "Hard A pan should use 2 paths");

        std::fill(outputL.begin(), outputL.end(), 0.0f);
        std::fill(outputR.begin(), outputR.end(), 0.0f);
        pairParams[0].pan = 1.0f;

        stats = renderer.render(stereo,
                                inputPointers,
                                2,
                                pairParams,
                                1.0f,
                                1.0f,
                                outputL.data(),
                                outputR.data(),
                                numSamples);

        expectWithinAbsoluteError(outputL[0], 0.75f, 0.0001f, "Hard B pan should keep only B RL");
        expectWithinAbsoluteError(outputR[0], 1.0f, 0.0001f, "Hard B pan should keep only B RR");
        expectEquals(stats.activeConvolutionPaths, 2, "Hard B pan should use 2 paths");

        renderer.releaseResources();
    }

    void testSoloMuteAndSumming()
    {
        juce::TemporaryFile irFile(".wav");
        expect(writeQuadImpulseIr(irFile.getFile(), 48000.0, { 1.0f, 0.0f, 0.0f, 1.0f }), "Should write summing IR");

        bsr::mch::SlotIrManager irManager;
        irManager.setHostSampleRate(48000.0);
        juce::String error;
        expect(irManager.loadSlotIrFile(0, irFile.getFile(), error), "Slot 1 IR should load");
        expect(irManager.loadSlotIrFile(2, irFile.getFile(), error), "Slot 3 IR should load");

        bsr::mch::MchBinauralRenderer renderer;
        renderer.prepareToPlay(48000.0, numSamples);
        renderer.applyAllSlotIrSnapshots(irManager);

        auto inputs = makeSilentInputs();
        inputs[0][0] = 1.0f;
        inputs[1][0] = 2.0f;
        inputs[4][0] = 3.0f;
        inputs[5][0] = 4.0f;
        auto inputPointers = makeInputPointers(inputs);

        const auto& preset = bsr::mch::getLayoutPresetByChoiceIndex(1); // 5.1 uses slots 1..3
        auto pairParams = makeDefaultPairParams();

        pairParams[1].enabled = false;
        pairParams[2].enabled = true;

        std::vector<float> outputL(numSamples, 0.0f);
        std::vector<float> outputR(numSamples, 0.0f);

        auto stats = renderer.render(preset,
                                     inputPointers,
                                     6,
                                     pairParams,
                                     1.0f,
                                     1.0f,
                                     outputL.data(),
                                     outputR.data(),
                                     numSamples);

        expectWithinAbsoluteError(outputL[0], 4.0f, 0.0001f, "Two active loaded pairs should sum on left");
        expectWithinAbsoluteError(outputR[0], 6.0f, 0.0001f, "Two active loaded pairs should sum on right");
        expectEquals(stats.activePairs, 2, "Two pairs should be audible before mute/solo");
        expectEquals(stats.activeConvolutionPaths, 8, "Two pairs should use 8 paths");

        std::fill(outputL.begin(), outputL.end(), 0.0f);
        std::fill(outputR.begin(), outputR.end(), 0.0f);
        pairParams[2].muted = true;

        stats = renderer.render(preset,
                                inputPointers,
                                6,
                                pairParams,
                                1.0f,
                                1.0f,
                                outputL.data(),
                                outputR.data(),
                                numSamples);

        expectWithinAbsoluteError(outputL[0], 1.0f, 0.0001f, "Muted pair should be excluded from left sum");
        expectWithinAbsoluteError(outputR[0], 2.0f, 0.0001f, "Muted pair should be excluded from right sum");
        expectEquals(stats.activePairs, 1, "Mute should reduce audible pairs");

        std::fill(outputL.begin(), outputL.end(), 0.0f);
        std::fill(outputR.begin(), outputR.end(), 0.0f);
        pairParams[2].muted = false;
        pairParams[2].solo = true;

        stats = renderer.render(preset,
                                inputPointers,
                                6,
                                pairParams,
                                1.0f,
                                1.0f,
                                outputL.data(),
                                outputR.data(),
                                numSamples);

        expectWithinAbsoluteError(outputL[0], 3.0f, 0.0001f, "Solo should isolate target pair on left");
        expectWithinAbsoluteError(outputR[0], 4.0f, 0.0001f, "Solo should isolate target pair on right");
        expectEquals(stats.activePairs, 1, "Solo should isolate one audible pair");
        expectEquals(stats.activeConvolutionPaths, 4, "Soloed single pair should use 4 paths");

        renderer.releaseResources();
    }

    void testWetModel()
    {
        juce::TemporaryFile irFile(".wav");
        expect(writeQuadImpulseIr(irFile.getFile(), 48000.0, { 0.0f, 0.0f, 0.0f, 0.0f }), "Should write wet-model IR");

        bsr::mch::SlotIrManager irManager;
        irManager.setHostSampleRate(48000.0);
        juce::String error;
        expect(irManager.loadSlotIrFile(0, irFile.getFile(), error), "IR should load for wet-model test");

        bsr::mch::MchBinauralRenderer renderer;
        renderer.prepareToPlay(48000.0, numSamples);
        renderer.applyAllSlotIrSnapshots(irManager);

        auto inputs = makeSilentInputs();
        inputs[0][0] = 2.0f;
        inputs[1][0] = 6.0f;
        auto inputPointers = makeInputPointers(inputs);

        const auto& stereo = bsr::mch::getLayoutPresetByChoiceIndex(0);
        auto pairParams = makeDefaultPairParams();

        std::vector<float> outputL(numSamples, 0.0f);
        std::vector<float> outputR(numSamples, 0.0f);

        pairParams[0].pairWet = 0.5f;
        auto stats = renderer.render(stereo,
                                     inputPointers,
                                     2,
                                     pairParams,
                                     0.5f,
                                     1.0f,
                                     outputL.data(),
                                     outputR.data(),
                                     numSamples);

        expectWithinAbsoluteError(outputL[0], 1.5f, 0.0001f, "Effective wet 25% should leave 75% dry on left");
        expectWithinAbsoluteError(outputR[0], 4.5f, 0.0001f, "Effective wet 25% should leave 75% dry on right");
        expectEquals(stats.activeConvolutionPaths, 4, "Non-zero effective wet should process wet paths");

        std::fill(outputL.begin(), outputL.end(), 0.0f);
        std::fill(outputR.begin(), outputR.end(), 0.0f);
        pairParams[0].pairWet = 0.0f;

        stats = renderer.render(stereo,
                                inputPointers,
                                2,
                                pairParams,
                                1.0f,
                                1.0f,
                                outputL.data(),
                                outputR.data(),
                                numSamples);

        expectWithinAbsoluteError(outputL[0], 2.0f, 0.0001f, "Pair wet 0% should be fully dry on left");
        expectWithinAbsoluteError(outputR[0], 6.0f, 0.0001f, "Pair wet 0% should be fully dry on right");
        expectEquals(stats.activeConvolutionPaths, 0, "Pair wet 0% should skip wet processing");

        renderer.releaseResources();
    }

    void testPerformanceMatrixScheduling()
    {
        juce::TemporaryFile shortIrFile(".wav");
        juce::TemporaryFile longIrFile(".wav");
        expect(writeQuadImpulseIr(shortIrFile.getFile(), 48000.0, { 0.5f, 0.5f, 0.5f, 0.5f }, 64), "Should write short IR");
        expect(writeQuadImpulseIr(longIrFile.getFile(), 48000.0, { 0.2f, 0.4f, 0.6f, 0.8f }, 16384), "Should write long-LFE placeholder IR");

        const std::array<double, 3> sampleRates { 44100.0, 48000.0, 96000.0 };
        const std::array<int, 3> blockSizes { 128, 256, 512 };
        const std::array<int, 4> activePairTargets { 1, 3, 6, 8 };

        for (auto sampleRate : sampleRates)
        {
            for (auto blockSize : blockSizes)
            {
                bsr::mch::SlotIrManager irManager;
                irManager.setHostSampleRate(sampleRate);
                juce::String error;

                for (int slot = 0; slot < bsr::mch::maxPairSlots; ++slot)
                {
                    const auto& irFile = slot == 1 ? longIrFile : shortIrFile;
                    expect(irManager.loadSlotIrFile(slot, irFile.getFile(), error), "Each slot IR should load for matrix run");
                }

                bsr::mch::MchBinauralRenderer renderer;
                renderer.prepareToPlay(sampleRate, blockSize);
                renderer.applyAllSlotIrSnapshots(irManager);

                auto inputs = std::array<std::vector<float>, bsr::mch::maxRendererInputChannels> {};
                for (auto& channel : inputs)
                    channel.assign(blockSize, 0.0f);

                for (int ch = 0; ch < 16; ++ch)
                    inputs[static_cast<size_t>(ch)][0] = 1.0f + static_cast<float>(ch) * 0.1f;

                auto inputPointers = makeInputPointers(inputs);
                const auto& preset = bsr::mch::getLayoutPresetByChoiceIndex(6); // 9.1.6

                std::vector<float> outputL(blockSize, 0.0f);
                std::vector<float> outputR(blockSize, 0.0f);

                for (int activePairs : activePairTargets)
                {
                    auto pairParams = makeDefaultPairParams();
                    for (int slot = 0; slot < bsr::mch::maxPairSlots; ++slot)
                    {
                        pairParams[static_cast<size_t>(slot)].enabled = slot < activePairs;
                        pairParams[static_cast<size_t>(slot)].pairWet = slot == 0 ? 0.0f : 1.0f;
                        pairParams[static_cast<size_t>(slot)].pan = slot == 2 ? -1.0f : 0.0f;
                    }

                    std::fill(outputL.begin(), outputL.end(), 0.0f);
                    std::fill(outputR.begin(), outputR.end(), 0.0f);

                    const auto stats = renderer.render(preset,
                                                       inputPointers,
                                                       16,
                                                       pairParams,
                                                       1.0f,
                                                       1.0f,
                                                       outputL.data(),
                                                       outputR.data(),
                                                       blockSize);

                    expectEquals(stats.activePairs, activePairs, "Active pair count should match enabled slots");
                    expect(stats.activeConvolutionPaths <= stats.maxConvolutionPaths,
                           "Active convolution paths should never exceed max visible paths");
                    expect(stats.activeWetPairs <= stats.activePairs, "Wet pair count should not exceed active pairs");
                    expect(stats.activeDryPairs <= stats.activePairs, "Dry pair count should not exceed active pairs");
                    expect(std::abs(outputL[0]) > 0.0f || std::abs(outputR[0]) > 0.0f,
                           "Renderer should produce non-zero output in matrix runs");
                }

                renderer.releaseResources();
            }
        }
    }

    void testHotReloadVersionedUpdate()
    {
        juce::TemporaryFile firstIr(".wav");
        juce::TemporaryFile secondIr(".wav");
        expect(writeQuadImpulseIr(firstIr.getFile(), 48000.0, { 0.25f, 0.5f, 0.75f, 1.0f }), "Should write first IR");
        expect(writeQuadImpulseIr(secondIr.getFile(), 48000.0, { 0.6f, 0.7f, 0.8f, 0.9f }), "Should write second IR");

        bsr::mch::SlotIrManager irManager;
        irManager.setHostSampleRate(48000.0);
        juce::String error;
        expect(irManager.loadSlotIrFile(0, firstIr.getFile(), error), "Should load first slot IR");

        bsr::mch::MchBinauralRenderer renderer;
        renderer.prepareToPlay(48000.0, numSamples);
        renderer.applyAllSlotIrSnapshots(irManager);

        auto inputs = makeSilentInputs();
        inputs[0][0] = 1.0f;
        inputs[1][0] = 1.0f;
        auto inputPointers = makeInputPointers(inputs);
        auto pairParams = makeDefaultPairParams();
        const auto& stereo = bsr::mch::getLayoutPresetByChoiceIndex(0);

        std::vector<float> outputL(numSamples, 0.0f);
        std::vector<float> outputR(numSamples, 0.0f);

        renderer.render(stereo,
                        inputPointers,
                        2,
                        pairParams,
                        1.0f,
                        1.0f,
                        outputL.data(),
                        outputR.data(),
                        numSamples);

        expectWithinAbsoluteError(outputL[0], 1.0f, 0.0001f, "Baseline output should match first IR");
        expectWithinAbsoluteError(outputR[0], 1.5f, 0.0001f, "Baseline output should match first IR");

        expect(irManager.loadSlotIrFile(0, secondIr.getFile(), error), "Should hot-reload second slot IR");
        renderer.applySlotIrSnapshot(0, irManager.getSlotSnapshot(0));

        std::fill(outputL.begin(), outputL.end(), 0.0f);
        std::fill(outputR.begin(), outputR.end(), 0.0f);

        renderer.render(stereo,
                        inputPointers,
                        2,
                        pairParams,
                        1.0f,
                        1.0f,
                        outputL.data(),
                        outputR.data(),
                        numSamples);

        expectWithinAbsoluteError(outputL[0], 1.4f, 0.0001f, "Hot reload should update left routing response");
        expectWithinAbsoluteError(outputR[0], 1.6f, 0.0001f, "Hot reload should update right routing response");

        renderer.releaseResources();
    }

    void test5Point1FullChainWithRealIr()
    {
        const auto quadIrFile = juce::File(BSR_PROJECT_ROOT).getChildFile("_quad_nr3_sm.wav");
        if (!quadIrFile.existsAsFile())
        {
            logMessage("Skipping: _quad_nr3_sm.wav not found at " + quadIrFile.getFullPathName());
            return;
        }

        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 512;

        const auto& preset = bsr::mch::getLayoutPresetByChoiceIndex(1); // 5.1
        expectEquals(preset.visibleSlotCount, 3, "5.1 preset should have 3 visible slots");
        expectEquals(preset.channelCount, 6, "5.1 preset should use 6 input channels");

        // Load the real quad IR into all 3 visible 5.1 slots
        bsr::mch::SlotIrManager irManager;
        irManager.setHostSampleRate(sampleRate);
        juce::String error;
        for (int slot = 0; slot < preset.visibleSlotCount; ++slot)
        {
            const bool loaded = irManager.loadSlotIrFile(slot, quadIrFile, error);
            expect(loaded, "Slot " + juce::String(slot) + " should load _quad_nr3_sm.wav: " + error);
            if (!loaded)
                return;
        }

        // Prepare renderer and push IRs
        bsr::mch::MchBinauralRenderer renderer;
        renderer.prepareToPlay(sampleRate, blockSize);
        renderer.applyAllSlotIrSnapshots(irManager);

        // White noise on all 6 input channels
        juce::Random rng(12345);
        std::array<std::vector<float>, bsr::mch::maxRendererInputChannels> inputs;
        for (auto& ch : inputs)
            ch.assign(blockSize, 0.0f);
        for (int ch = 0; ch < preset.channelCount; ++ch)
            for (int s = 0; s < blockSize; ++s)
                inputs[static_cast<size_t>(ch)][s] = rng.nextFloat() * 2.0f - 1.0f;

        std::array<const float*, bsr::mch::maxRendererInputChannels> inputPointers {};
        for (int i = 0; i < bsr::mch::maxRendererInputChannels; ++i)
            inputPointers[static_cast<size_t>(i)] = inputs[static_cast<size_t>(i)].data();

        auto pairParams = makeDefaultPairParams();
        std::vector<float> outputL(blockSize, 0.0f);
        std::vector<float> outputR(blockSize, 0.0f);

        // First render primes the zero-latency convolution engine
        renderer.render(preset, inputPointers, preset.channelCount, pairParams,
                        1.0f, 1.0f, outputL.data(), outputR.data(), blockSize);

        // Refill with fresh noise and measure the second block
        for (int ch = 0; ch < preset.channelCount; ++ch)
            for (int s = 0; s < blockSize; ++s)
                inputs[static_cast<size_t>(ch)][s] = rng.nextFloat() * 2.0f - 1.0f;
        std::fill(outputL.begin(), outputL.end(), 0.0f);
        std::fill(outputR.begin(), outputR.end(), 0.0f);

        const auto stats = renderer.render(preset, inputPointers, preset.channelCount, pairParams,
                                           1.0f, 1.0f, outputL.data(), outputR.data(), blockSize);

        // Render-stats assertions
        expectEquals(stats.visiblePairs,     3, "5.1 should count 3 visible pairs");
        expectEquals(stats.activePairs,      3, "All 3 pairs should be active (IRs loaded + enabled)");
        expectEquals(stats.skippedPairs,     0, "No pairs should be skipped when all IRs are present");
        expectEquals(stats.activeWetPairs,   3, "All 3 pairs should use the wet convolution path");
        expect(stats.activeConvolutionPaths > 0, "At least some convolution paths should be active");

        // Output RMS must be non-zero on both stereo channels
        double sumSqL = 0.0, sumSqR = 0.0;
        for (int s = 0; s < blockSize; ++s)
        {
            sumSqL += static_cast<double>(outputL[static_cast<size_t>(s)]) * outputL[static_cast<size_t>(s)];
            sumSqR += static_cast<double>(outputR[static_cast<size_t>(s)]) * outputR[static_cast<size_t>(s)];
        }
        const float rmsL = static_cast<float>(std::sqrt(sumSqL / blockSize));
        const float rmsR = static_cast<float>(std::sqrt(sumSqR / blockSize));

        expect(rmsL > 1.0e-6f, "Output L RMS should be non-zero after 5.1 binaural render (got " + juce::String(rmsL) + ")");
        expect(rmsR > 1.0e-6f, "Output R RMS should be non-zero after 5.1 binaural render (got " + juce::String(rmsR) + ")");

        renderer.releaseResources();
    }
};

static MchBinauralRendererTests mchBinauralRendererTests;
