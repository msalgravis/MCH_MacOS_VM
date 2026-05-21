#include "MchSlotIrManager.h"

#include <cmath>

namespace bsr::mch
{
namespace
{
juce::String normaliseStoredPathString(const juce::String& rawPath)
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

juce::File makeFileFromStoredPath(const juce::String& rawPath)
{
    const auto path = normaliseStoredPathString(rawPath);
    if (path.isEmpty())
        return {};

    juce::File file(path);
    if (juce::File::isAbsolutePath(path))
        return file;

    return juce::File::getCurrentWorkingDirectory().getChildFile(path);
}
} // namespace

SlotIrManager::SlotIrManager()
{
    formatManager.registerBasicFormats();
}

void SlotIrManager::setHostSampleRate(double sampleRateHz) noexcept
{
    hostSampleRateHz = sampleRateHz;

    for (int slot = 0; slot < maxPairSlots; ++slot)
        if (slots[static_cast<size_t>(slot)].status == SlotIrStatus::loaded)
        {
            ++slots[static_cast<size_t>(slot)].version;
            resampleSlotIfNeeded(slot);
        }
}

bool SlotIrManager::loadSlotIrFile(int slotIndex, const juce::File& filePath, juce::String& errorMessage) noexcept
{
    if (!isValidSlotIndex(slotIndex))
    {
        errorMessage = "Invalid pair slot index";
        return false;
    }

    if (!filePath.existsAsFile())
    {
        errorMessage = "IR file not found: " + filePath.getFullPathName();
        setSlotFailure(slotIndex, SlotIrStatus::error, errorMessage, filePath.getFullPathName());
        return false;
    }

    juce::AudioBuffer<float> sourceBuffer;
    int sourceSampleRate = 0;
    if (!readQuadWav(filePath, sourceBuffer, sourceSampleRate, errorMessage))
    {
        setSlotFailure(slotIndex, SlotIrStatus::invalid, errorMessage, filePath.getFullPathName());
        return false;
    }

    auto& slot = slots[static_cast<size_t>(slotIndex)];
    ++slot.version;
    slot.status = SlotIrStatus::loaded;
    slot.lastError.clear();
    slot.sourcePath = filePath.getFullPathName();
    slot.sourceSampleRate = sourceSampleRate;
    slot.sourceNumSamples = sourceBuffer.getNumSamples();
    slot.sourceBuffer = std::move(sourceBuffer);

    resampleSlotIfNeeded(slotIndex);
    errorMessage.clear();
    return true;
}

bool SlotIrManager::restoreSlotFromPath(int slotIndex, const juce::String& storedPath, juce::String& errorMessage) noexcept
{
    if (!isValidSlotIndex(slotIndex))
    {
        errorMessage = "Invalid pair slot index";
        return false;
    }

    const auto normalisedPath = normaliseStoredPathString(storedPath);
    if (normalisedPath.isEmpty())
    {
        clearSlot(slotIndex);
        errorMessage.clear();
        return true;
    }

    const auto file = makeFileFromStoredPath(normalisedPath);
    if (!file.existsAsFile())
    {
        errorMessage = "Missing saved IR file: " + file.getFullPathName();
        setSlotFailure(slotIndex, SlotIrStatus::error, errorMessage, file.getFullPathName());
        return false;
    }

    return loadSlotIrFile(slotIndex, file, errorMessage);
}

void SlotIrManager::clearSlot(int slotIndex) noexcept
{
    if (!isValidSlotIndex(slotIndex))
        return;

    auto& slot = slots[static_cast<size_t>(slotIndex)];
    ++slot.version;
    slot.status = SlotIrStatus::missing;
    slot.sourcePath.clear();
    slot.lastError.clear();
    slot.sourceSampleRate = 0;
    slot.sourceNumSamples = 0;
    slot.sourceBuffer.setSize(0, 0, false, false, true);
    slot.processedBuffer.setSize(0, 0, false, false, true);
    slot.processedSnapshot.reset();
}

void SlotIrManager::clearAll() noexcept
{
    for (int slot = 0; slot < maxPairSlots; ++slot)
        clearSlot(slot);
}

bool SlotIrManager::isSlotLoaded(int slotIndex) const noexcept
{
    if (!isValidSlotIndex(slotIndex))
        return false;

    return slots[static_cast<size_t>(slotIndex)].status == SlotIrStatus::loaded;
}

SlotIrStatus SlotIrManager::getSlotStatus(int slotIndex) const noexcept
{
    if (!isValidSlotIndex(slotIndex))
        return SlotIrStatus::error;

    return slots[static_cast<size_t>(slotIndex)].status;
}

juce::String SlotIrManager::getSlotPath(int slotIndex) const
{
    if (!isValidSlotIndex(slotIndex))
        return {};

    return slots[static_cast<size_t>(slotIndex)].sourcePath;
}

juce::String SlotIrManager::getSlotError(int slotIndex) const
{
    if (!isValidSlotIndex(slotIndex))
        return {};

    return slots[static_cast<size_t>(slotIndex)].lastError;
}

SlotIrManager::SlotIrSnapshot SlotIrManager::getSlotSnapshot(int slotIndex) const noexcept
{
    SlotIrSnapshot snapshot;

    if (!isValidSlotIndex(slotIndex))
        return snapshot;

    const auto& slot = slots[static_cast<size_t>(slotIndex)];
    snapshot.isLoaded = slot.status == SlotIrStatus::loaded;
    snapshot.version = slot.version;
    snapshot.processedIr = slot.processedSnapshot;
    return snapshot;
}

juce::String SlotIrManager::getSlotMetadataText(int slotIndex) const noexcept
{
    if (!isValidSlotIndex(slotIndex))
        return "Invalid slot";

    const auto& slot = slots[static_cast<size_t>(slotIndex)];
    if (slot.status != SlotIrStatus::loaded)
        return "Not loaded";

    const auto filename = juce::File(slot.sourcePath).getFileName();
    const auto durationMs = slot.sourceSampleRate > 0 
        ? (static_cast<double>(slot.sourceNumSamples) / static_cast<double>(slot.sourceSampleRate)) * 1000.0
        : 0.0;
    const auto durationSec = durationMs / 1000.0;

    return filename + juce::String::formatted(
        " (%d-ch, %d Hz, %.2fs)",
        slot.sourceBuffer.getNumChannels(),
        slot.sourceSampleRate,
        durationSec);
}

bool SlotIrManager::isValidSlotIndex(int slotIndex) noexcept
{
    return slotIndex >= 0 && slotIndex < maxPairSlots;
}

bool SlotIrManager::readQuadWav(const juce::File& filePath,
                                juce::AudioBuffer<float>& outBuffer,
                                int& outSampleRate,
                                juce::String& errorMessage) noexcept
{
    if (!filePath.hasFileExtension(".wav"))
    {
        errorMessage = "IR file must be a WAV file";
        return false;
    }

    auto stream = filePath.createInputStream();
    if (stream == nullptr)
    {
        errorMessage = "Unable to open IR file";
        return false;
    }

    auto reader = std::unique_ptr<juce::AudioFormatReader>(formatManager.createReaderFor(std::move(stream)));
    if (reader == nullptr)
    {
        errorMessage = "Unable to parse WAV file";
        return false;
    }

    if (reader->numChannels != 4)
    {
        errorMessage = juce::String::formatted("IR must have exactly 4 channels, but file has %d", reader->numChannels);
        return false;
    }

    const int numSamples = static_cast<int>(reader->lengthInSamples);
    if (numSamples <= 0)
    {
        errorMessage = "IR contains no audio samples";
        return false;
    }

    outBuffer.setSize(4, numSamples, true, false, true);
    if (!reader->read(&outBuffer, 0, numSamples, 0, true, true))
    {
        errorMessage = "Failed to read IR sample data";
        return false;
    }

    outSampleRate = static_cast<int>(std::lround(reader->sampleRate));
    if (outSampleRate <= 0)
    {
        errorMessage = "Invalid sample rate in IR file";
        return false;
    }

    return true;
}

void SlotIrManager::setSlotFailure(int slotIndex,
                                   SlotIrStatus status,
                                   const juce::String& message,
                                   const juce::String& sourcePath) noexcept
{
    if (!isValidSlotIndex(slotIndex))
        return;

    auto& slot = slots[static_cast<size_t>(slotIndex)];
    ++slot.version;
    slot.status = status;
    slot.lastError = message;
    slot.sourcePath = sourcePath;
    slot.sourceSampleRate = 0;
    slot.sourceNumSamples = 0;
    slot.sourceBuffer.setSize(0, 0, false, false, true);
    slot.processedBuffer.setSize(0, 0, false, false, true);
    slot.processedSnapshot.reset();
}

void SlotIrManager::resampleSlotIfNeeded(int slotIndex) noexcept
{
    if (!isValidSlotIndex(slotIndex))
        return;

    auto& slot = slots[static_cast<size_t>(slotIndex)];
    if (slot.status != SlotIrStatus::loaded)
        return;

    const int srcSamples = slot.sourceBuffer.getNumSamples();
    if (slot.sourceSampleRate <= 0 || srcSamples <= 0)
    {
        setSlotFailure(slotIndex, SlotIrStatus::error, "Loaded IR data is invalid", slot.sourcePath);
        return;
    }

    if (hostSampleRateHz <= 0.0 || std::abs(hostSampleRateHz - static_cast<double>(slot.sourceSampleRate)) <= 0.5)
    {
        slot.processedBuffer = slot.sourceBuffer;
        slot.processedSnapshot = std::make_shared<juce::AudioBuffer<float>>(slot.processedBuffer);
        return;
    }

    const double ratio = hostSampleRateHz / static_cast<double>(slot.sourceSampleRate);
    const int dstSamples = juce::jmax(1, static_cast<int>(std::lround(static_cast<double>(srcSamples) * ratio)));

    slot.processedBuffer.setSize(4, dstSamples, true, false, true);
    for (int channel = 0; channel < 4; ++channel)
    {
        const auto* src = slot.sourceBuffer.getReadPointer(channel);
        auto* dst = slot.processedBuffer.getWritePointer(channel);
        resampleChannelLinear(src, srcSamples, dst, dstSamples);
    }

    slot.processedSnapshot = std::make_shared<juce::AudioBuffer<float>>(slot.processedBuffer);
}

void SlotIrManager::resampleChannelLinear(const float* src, int srcSamples, float* dst, int dstSamples) noexcept
{
    if (src == nullptr || dst == nullptr || srcSamples <= 0 || dstSamples <= 0)
        return;

    if (srcSamples == 1)
    {
        juce::FloatVectorOperations::fill(dst, src[0], dstSamples);
        return;
    }

    if (dstSamples == 1)
    {
        dst[0] = src[0];
        return;
    }

    const double step = static_cast<double>(srcSamples - 1) / static_cast<double>(dstSamples - 1);

    for (int i = 0; i < dstSamples; ++i)
    {
        const double position = step * static_cast<double>(i);
        const int index = static_cast<int>(position);
        const int nextIndex = juce::jmin(index + 1, srcSamples - 1);
        const float frac = static_cast<float>(position - static_cast<double>(index));
        const float a = src[index];
        const float b = src[nextIndex];
        dst[i] = a + (b - a) * frac;
    }
}

} // namespace bsr::mch
