#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Humvoice"; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    float getDetectedPitchHz() const { return currentPitchHz.load(); }

    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    static constexpr int   kMonoBufferSize       = 2048;
    static constexpr int   kVelocity             = 100;
    static constexpr int   kCandidateBlocks      = 5;     // consecutive blocks before committing
    static constexpr float kPitchHysteresisCents = 60.0f; // ~half-semitone deadband
    static constexpr float kMinNoteHoldMs        = 100.0f; // minimum hold time before note change

    std::array<float, kMonoBufferSize> monoCircularBuffer{};
    int monoWriteIndex = 0;

    double currentSampleRate   = 44100.0;
    int    minNoteHoldSamples  = 0;      // derived from kMinNoteHoldMs in prepareToPlay
    float  smoothingAlpha      = 0.2f;   // per-block EMA coefficient (set in prepareToPlay)
    float  smoothedPitchHz     = 0.0f;   // exponentially smoothed pitch for display
    std::atomic<float> currentPitchHz { 0.0f };

    int lastMidiNote    = -1;  // currently sounding note; -1 = silent
    int candidateNote   = -1;  // proposed next note
    int candidateBlocks = 0;   // consecutive blocks the candidate has held
    int noteHoldSamples = 0;   // samples elapsed since last note change

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
