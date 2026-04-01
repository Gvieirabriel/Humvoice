#include "AudioEngine.h"

#include <cmath>

void AudioEngine::prepare(double /*sampleRate*/, int /*samplesPerBlock*/)
{
    monoBuffer.fill(0.0f);
    writeIndex = 0;
    blockRms   = 0.0f;
}

void AudioEngine::process(const juce::AudioBuffer<float>& buffer)
{
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    float sumSq = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float mono = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            mono += buffer.getSample(ch, i);
        if (numChannels > 0)
            mono /= static_cast<float>(numChannels);

        monoBuffer[writeIndex] = mono;
        writeIndex = (writeIndex + 1) & (kBufferSize - 1);
        sumSq += mono * mono;
    }

    blockRms = (numSamples > 0)
                   ? std::sqrt(sumSq / static_cast<float>(numSamples))
                   : 0.0f;
}
