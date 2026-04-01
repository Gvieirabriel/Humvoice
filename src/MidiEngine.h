#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// scaleType values (match APVTS "scaleType" parameter choices)
enum class ScaleType { Off = 0, Major = 1, Minor = 2 };

class MidiEngine
{
public:
    void prepare(double sampleRate, int samplesPerBlock);

    // hz             – median-filtered detected frequency (0 = silence); used for note decisions
    // smoothedHz     – EMA-smoothed frequency from PitchEngine; used for pitch bend
    // rms            – block RMS amplitude from AudioEngine
    // numSamples     – block size, used to advance the hold timer
    // midiOut        – note-on / note-off / pitch-bend events are appended here
    // scaleType      – 0=Off, 1=Major, 2=Minor
    // rootNote       – 0=C … 11=B
    // minNoteLengthMs – minimum ms a note must be held before switching
    void process(float hz, float smoothedHz, float rms, int numSamples,
                 juce::MidiBuffer& midiOut, int scaleType, int rootNote,
                 float minNoteLengthMs);

private:
    static constexpr int   kCandidateBlocks       = 5;
    static constexpr float kPitchHysteresisCents  = 60.0f;
    // Pitch bend range sent to the synth (standard MIDI = ±2 semitones = ±200 cents).
    // Must match the receiving instrument's pitch bend range setting.
    static constexpr float kPitchBendRangeCents   = 200.0f;
    // Minimum change in bend LSBs before a new pitch-bend message is emitted.
    static constexpr int   kPitchBendDeadband     = 4;

    static int   rmsToVelocity(float rms);
    static int   hzToMidi(float hz);
    static float centsBetween(float hz, int midiNote);
    static int   quantizeToScale(int midiNote, int scaleType, int rootNote);

    double sampleRate_   = 44100.0;
    float  velocityAlpha = 0.2f;
    float  smoothedRms   = 0.0f;

    int lastMidiNote       = -1;
    int candidateNote      = -1;
    int candidateBlocks    = 0;
    int noteHoldSamples    = 0;
    int lastPitchBend_     = 8192;  // last sent pitch-bend value; 8192 = centre
};
