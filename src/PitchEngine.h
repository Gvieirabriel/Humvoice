#pragma once

#include <array>

class PitchEngine
{
public:
    void  prepare(double sampleRate, int samplesPerBlock);

    // Runs pitch detection, applies median filter, updates smoothed display state.
    // Returns the median-filtered Hz for use by MidiEngine.
    // Call getSmoothedHz() for the EMA-smoothed display value.
    // rms         – block RMS from AudioEngine; compared against gate threshold.
    // noiseGateDb – gate threshold in dBFS (e.g. -40); signals below it return 0.
    // smoothingMs – EMA time constant in ms for the display pitch (0 = no smoothing).
    float process(const float* buf, int bufSize, int writeIndex,
                  float sensitivity, float rms, float noiseGateDb, float smoothingMs);

    float getSmoothedHz() const { return smoothedPitchHz; }

    // Constrains pitch search and octave continuity to the given frequency range.
    // Safe to call from the UI thread between audio blocks (MVP: no mutex).
    void setVocalRange(float minHz, float maxHz);

private:
    // Number of frames kept for the median filter.
    // At a typical 128-sample block / 44100 Hz this covers ~14 ms of history.
    static constexpr int kHistorySize = 5;

    double sampleRate_      = 44100.0;
    int    samplesPerBlock_ = 512;
    float  smoothedPitchHz  = 0.0f;

    float vocalMinHz_ = 80.0f;
    float vocalMaxHz_ = 1000.0f;

    std::array<float, kHistorySize> pitchHistory{};
    int   historyIndex  = 0;
    float lastStableHz  = 0.0f; // previous output, used for octave continuity

    // Returns the median of a copy of history (zeros included so brief silence
    // can suppress a faint detection, and a brief gap mid-note is masked).
    static float median(std::array<float, kHistorySize> h);

    // Among hz, hz/2 and hz*2, returns the candidate closest to lastHz
    // (in cents) that is still within [minHz, maxHz].
    // Falls back to hz when lastHz is unknown (0).
    static float nearestOctave(float hz, float lastHz, float minHz, float maxHz);

    static float detectPitch(const float* buf, int bufSize, int writeIndex,
                             double sampleRate, float sensitivity,
                             float minHz, float maxHz);
};
