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
    // Advance retrigger cooldown
    retriggerSamples_ = std::max(0, retriggerSamples_ - numSamples);

    // Asymmetric envelope follower: rise fast, fall slow.
    // "Background" tracks the sustain level; a transient spike sits well above it.
    if (rms > smoothedEnergy_)
        smoothedEnergy_ = 0.4f * rms  + 0.6f * smoothedEnergy_;
    else
        smoothedEnergy_ = 0.01f * rms + 0.99f * smoothedEnergy_;

    const float gateLinear = std::pow(10.0f, noiseGateDb / 20.0f);

    // Map sensitivity 0→1 to onset-ratio threshold 5.0→1.5
    const float onsetRatio = 5.0f - sensitivity * 3.5f;

    const bool onset = (rms > gateLinear)
                    && (smoothedEnergy_ > 1e-6f)
                    && (rms / smoothedEnergy_ > onsetRatio)
                    && (retriggerSamples_ == 0);

    if (onset)
    {
        const int note = juce::jlimit(0, 127, drumNote);

        // Velocity: logarithmic mapping of absolute amplitude, same curve as MidiEngine
        const float dB         = 20.0f * std::log10(std::max(rms, 1e-6f));
        const float normalized = (dB + 60.0f) / 60.0f;
        const auto  velocity   = static_cast<juce::uint8>(
            std::max(1, std::min(127, static_cast<int>(normalized * 126.0f + 1.0f))));

        // Note-on at block start, note-off at block end (drums need only a brief gate)
        midiOut.addEvent(juce::MidiMessage::noteOn (1, note, velocity), 0);
        midiOut.addEvent(juce::MidiMessage::noteOff(1, note),           numSamples - 1);

        retriggerSamples_ = minRetrigger_;
    }
}
