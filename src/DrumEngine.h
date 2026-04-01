#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Onset-detection engine for drum-trigger mode.
// Watches the block RMS for sudden energy jumps and fires a fixed MIDI note.
class DrumEngine
{
public:
    void prepare(double sampleRate, int samplesPerBlock);

    // rms         – block RMS from AudioEngine
    // numSamples  – block size, used to advance the retrigger timer
    // midiOut     – note-on / note-off events are appended here
    // drumNote    – MIDI note 0-127 fired on each detected onset
    // noiseGateDb – floor below which no onset can be declared (dBFS)
    // sensitivity – 0=hard to trigger (ratio 5×), 1=easy (ratio 1.5×)
    void process(float rms, int numSamples, juce::MidiBuffer& midiOut,
                 int drumNote, float noiseGateDb, float sensitivity);

private:
    static constexpr int kRetriggerMs = 80; // minimum ms between consecutive hits

    double sampleRate_       = 44100.0;
    float  smoothedEnergy_   = 0.0f;   // asymmetric envelope follower
    int    retriggerSamples_ = 0;      // countdown; onset blocked while > 0
    int    minRetrigger_     = 3528;   // samples equivalent of kRetriggerMs
};
