#include "DrumEngine.h"

#include <cmath>
#include <algorithm>

void DrumEngine::prepare(double sampleRate, int /*samplesPerBlock*/)
{
    sampleRate_       = sampleRate;
    smoothedEnergy_   = 0.0f;
    retriggerSamples_ = 0;
    minRetrigger_     = static_cast<int>(sampleRate * kRetriggerMs / 1000.0);
}

void DrumEngine::process(float rms, int numSamples, juce::MidiBuffer& midiOut,
                         int drumNote, float noiseGateDb, float sensitivity)
{
    retriggerSamples_ = std::max(0, retriggerSamples_ - numSamples);

    // Save energy BEFORE updating so the onset ratio is computed against the
    // background level, not the partially-tracked transient level.
    const float prevEnergy = smoothedEnergy_;

    // Asymmetric envelope follower: rise fast, fall slow.
    if (rms > smoothedEnergy_)
        smoothedEnergy_ = 0.4f * rms  + 0.6f * smoothedEnergy_;
    else
        smoothedEnergy_ = 0.01f * rms + 0.99f * smoothedEnergy_;

    const float gateLinear = std::pow(10.0f, noiseGateDb / 20.0f);

    // sensitivity 0→1 maps onset-ratio threshold 5.0→1.5
    const float onsetRatio = 5.0f - sensitivity * 3.5f;

    // Use prevEnergy so the transient spike isn't partially absorbed by the
    // smoother before we check it.
    const bool onset = (rms > gateLinear)
                    && (prevEnergy > 1e-6f)
                    && (rms / prevEnergy > onsetRatio)
                    && (retriggerSamples_ == 0);

    if (onset)
    {
        const int note = juce::jlimit(0, 127, drumNote);

        const float dB         = 20.0f * std::log10(std::max(rms, 1e-6f));
        const float normalized = (dB + 60.0f) / 60.0f;
        const auto  velocity   = static_cast<juce::uint8>(
            std::max(1, std::min(127, static_cast<int>(normalized * 126.0f + 1.0f))));

        midiOut.addEvent(juce::MidiMessage::noteOn (1, note, velocity), 0);
        midiOut.addEvent(juce::MidiMessage::noteOff(1, note),           numSamples - 1);

        retriggerSamples_ = minRetrigger_;
    }
}
