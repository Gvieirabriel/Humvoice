#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

class AudioEngine
{
public:
    static constexpr int kBufferSize = 2048;

    void prepare(double sampleRate, int samplesPerBlock);
    void process(const juce::AudioBuffer<float>& buffer);

    const float* getBuffer()     const { return monoBuffer.data(); }
    int          getWriteIndex() const { return writeIndex; }
    float        getRms()        const { return blockRms; }

private:
    std::array<float, kBufferSize> monoBuffer{};
    int   writeIndex = 0;
    float blockRms   = 0.0f;
};
