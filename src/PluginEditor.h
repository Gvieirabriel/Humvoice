#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class PluginEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit PluginEditor(PluginProcessor&);
    ~PluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void paintPitchDisplay(juce::Graphics&, juce::Rectangle<int> area);
    void stopCalibration();

    PluginProcessor& processorRef;

    // ── Mode selector ─────────────────────────────────────────────────────────
    juce::ComboBox modeBox;
    juce::Label    modeLabel;

    // ── Pitch-mode controls ───────────────────────────────────────────────────
    juce::Slider sensitivitySlider;
    juce::Slider noiseGateSlider;
    juce::Slider smoothingSlider;
    juce::Slider minNoteLengthSlider;

    juce::Label sensitivityLabel;
    juce::Label noiseGateLabel;
    juce::Label smoothingLabel;
    juce::Label minNoteLengthLabel;

    juce::ComboBox scaleTypeBox;
    juce::ComboBox rootNoteBox;
    juce::Label    scaleTypeLabel;
    juce::Label    rootNoteLabel;

    juce::TextButton calibrateButton { "Calibrate" };

    // ── Drum-mode controls ────────────────────────────────────────────────────
    juce::Slider drumNoteSlider;
    juce::Label  drumNoteLabel;

    // ── APVTS attachments ─────────────────────────────────────────────────────
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    ComboAttachment  modeAttachment;
    SliderAttachment sensitivityAttachment;
    SliderAttachment noiseGateAttachment;
    SliderAttachment smoothingAttachment;
    SliderAttachment minNoteLengthAttachment;
    ComboAttachment  scaleTypeAttachment;
    ComboAttachment  rootNoteAttachment;
    SliderAttachment drumNoteAttachment;

    // ── Pitch display state ───────────────────────────────────────────────────
    juce::String displayNote     { "--" };
    juce::String displayCents    { "" };
    float        stabilityScore  { 0.0f };
    float        prevHz          { 0.0f };
    float        smoothedDelta   { 0.0f };

    // ── Calibration UI state ──────────────────────────────────────────────────
    bool         calibrating_        = false;
    int          calibTicks_         = 0;
    int          resultDisplayTicks_ = 0;
    juce::String calibResultText_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
