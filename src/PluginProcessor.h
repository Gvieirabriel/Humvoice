#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "AudioEngine.h"
#include "PitchEngine.h"
#include "MidiEngine.h"
#include "DrumEngine.h"

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

    // ── Calibration ──────────────────────────────────────────────────────────
    void startCalibration();
    void stopCalibration();
    bool isCalibrating() const { return calibrating_.load(); }
    std::pair<float, float> getCalibrationRange() const
    {
        return { calibMinHz_.load(), calibMaxHz_.load() };
    }

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    AudioEngine audioEngine;
    PitchEngine pitchEngine;
    MidiEngine  midiEngine;
    DrumEngine  drumEngine;

    std::atomic<float> currentPitchHz { 0.0f };

    // Calibration state
    std::atomic<bool>  calibrating_  { false };
    std::atomic<float> calibMinHz_   { 9999.0f };
    std::atomic<float> calibMaxHz_   { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
