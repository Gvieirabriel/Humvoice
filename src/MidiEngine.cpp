#include "MidiEngine.h"

#include <cmath>
#include <algorithm>

int MidiEngine::rmsToVelocity(float rms)
{
    const float dB         = 20.0f * std::log10(std::max(rms, 1e-6f));
    const float normalized = (dB + 60.0f) / 60.0f;
    return std::max(1, std::min(127, static_cast<int>(normalized * 126.0f + 1.0f)));
}

int MidiEngine::hzToMidi(float hz)
{
    if (hz <= 0.0f)
        return -1;
    const int note = static_cast<int>(std::round(69.0f + 12.0f * std::log2(hz / 440.0f)));
    return (note >= 0 && note <= 127) ? note : -1;
}

float MidiEngine::centsBetween(float hz, int midiNote)
{
    if (hz <= 0.0f || midiNote < 0)
        return 9999.0f;
    const float noteHz = 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);
    return std::abs(1200.0f * std::log2(hz / noteHz));
}

int MidiEngine::quantizeToScale(int midiNote, int scaleType, int rootNote)
{
    if (scaleType == 0 || midiNote < 0)
        return midiNote;

    static constexpr int major[7] = { 0, 2, 4, 5, 7, 9, 11 };
    static constexpr int minor[7] = { 0, 2, 3, 5, 7, 8, 10 };
    const int* scale = (scaleType == 1) ? major : minor;

    const int degree = ((midiNote - rootNote) % 12 + 12) % 12;

    int bestDegree = scale[0];
    int bestDist   = 12;

    for (int i = 0; i < 7; ++i)
    {
        const int dist = std::min(std::abs(degree - scale[i]),
                                  12 - std::abs(degree - scale[i]));
        if (dist < bestDist)
        {
            bestDist   = dist;
            bestDegree = scale[i];
        }
    }

    int result = (midiNote - degree) + bestDegree;
    if (bestDegree < degree && (degree - bestDegree) > 6) result += 12;
    if (bestDegree > degree && (bestDegree - degree) > 6) result -= 12;

    return std::max(0, std::min(127, result));
}

void MidiEngine::prepare(double sampleRate, int samplesPerBlock)
{
    sampleRate_ = sampleRate;
    velocityAlpha = 1.0f - std::exp(-static_cast<float>(samplesPerBlock)
                                    / (static_cast<float>(sampleRate) * 0.050f));
    smoothedRms     = 0.0f;
    lastMidiNote    = -1;
    candidateNote   = -1;
    candidateBlocks = 0;
    noteHoldSamples = 0;
    lastPitchBend_  = 8192;
    smoothedBend_   = 8192.0f;
}

void MidiEngine::process(float hz, float smoothedHz, float rms, int numSamples,
                         juce::MidiBuffer& midiOut, int scaleType, int rootNote,
                         float minNoteLengthMs, bool enablePitchBend)
{
    noteHoldSamples += numSamples;
    smoothedRms = velocityAlpha * rms + (1.0f - velocityAlpha) * smoothedRms;

    // ── Note change logic ────────────────────────────────────────────────────

    const bool withinDeadband =
        (hz > 0.0f && lastMidiNote >= 0)
            ? centsBetween(hz, lastMidiNote) < kPitchHysteresisCents
            : (hz <= 0.0f && lastMidiNote < 0);

    if (withinDeadband)
    {
        candidateNote   = lastMidiNote;
        candidateBlocks = 0;
    }
    else
    {
        const int detectedNote = quantizeToScale(hzToMidi(hz), scaleType, rootNote);
        if (detectedNote == candidateNote)
            ++candidateBlocks;
        else
        {
            candidateNote   = detectedNote;
            candidateBlocks = 1;
        }

        const int minHoldSamples = static_cast<int>(sampleRate_ * minNoteLengthMs / 1000.0f);
        if (candidateBlocks >= kCandidateBlocks
            && candidateNote != lastMidiNote
            && noteHoldSamples >= minHoldSamples)
        {
            if (lastMidiNote >= 0)
                midiOut.addEvent(juce::MidiMessage::noteOff(1, lastMidiNote), 0);

            // Reset pitch bend to centre before the new note so the receiving
            // instrument doesn't inherit a stale bend value.
            midiOut.addEvent(juce::MidiMessage::pitchWheel(1, 8192), 0);
            lastPitchBend_ = 8192;
            smoothedBend_  = 8192.0f;

            if (candidateNote >= 0)
            {
                const auto velocity = static_cast<juce::uint8>(rmsToVelocity(smoothedRms));
                midiOut.addEvent(
                    juce::MidiMessage::noteOn(1, candidateNote, velocity), 0);
            }

            lastMidiNote    = candidateNote;
            noteHoldSamples = 0;
        }
    }

    // ── Pitch bend ───────────────────────────────────────────────────────────

    if (enablePitchBend && lastMidiNote >= 0 && smoothedHz > 0.0f)
    {
        // Deviation of the smoothed pitch from the held note's centre frequency
        const float noteHz = 440.0f * std::pow(2.0f, (lastMidiNote - 69) / 12.0f);
        const float cents  = 1200.0f * std::log2(smoothedHz / noteHz);

        // Target 14-bit bend value
        const float targetBend = 8192.0f + juce::jlimit(-8192.0f, 8191.0f,
                                     (cents / kPitchBendRangeCents) * 8192.0f);

        // Low-pass filter to suppress jitter from frame-to-frame pitch variation
        smoothedBend_ = kBendAlpha * targetBend + (1.0f - kBendAlpha) * smoothedBend_;

        const int bend = static_cast<int>(std::round(smoothedBend_));
        if (std::abs(bend - lastPitchBend_) >= kPitchBendDeadband)
        {
            midiOut.addEvent(juce::MidiMessage::pitchWheel(1, bend), 0);
            lastPitchBend_ = bend;
        }
    }
    else if (lastPitchBend_ != 8192)
    {
        // Pitch bend disabled, no active note, or no pitch detected — return to centre.
        midiOut.addEvent(juce::MidiMessage::pitchWheel(1, 8192), 0);
        lastPitchBend_ = 8192;
        smoothedBend_  = 8192.0f;
    }
}
