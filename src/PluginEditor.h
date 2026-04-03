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
    void paintDisplay(juce::Graphics&, juce::Rectangle<int> area);
    void updateVisibility(bool isDrum);
    void stopCalibration();

    PluginProcessor& processorRef;

    // ── Header ────────────────────────────────────────────────────────────────
    juce::ComboBox modeBox;
    juce::Label    modeLabel;

    // ── Section headers ───────────────────────────────────────────────────────
    juce::Label sectionDetection, sectionMusical, sectionExpression, sectionDrum;

    // ── Detection controls (shared) ───────────────────────────────────────────
    juce::Slider sensitivitySlider, noiseGateSlider;
    juce::Label  sensitivityLabel,  noiseGateLabel;

    // ── Pitch-mode controls ───────────────────────────────────────────────────
    juce::Slider   smoothingSlider, minNoteLengthSlider;
    juce::Label    smoothingLabel,  minNoteLengthLabel;
    juce::ComboBox scaleTypeBox,    rootNoteBox;
    juce::Label    scaleTypeLabel,  rootNoteLabel;

    // ── Expression ───────────────────────────────────────────────────────────
    juce::ToggleButton pitchBendToggle;

    // ── Calibration ───────────────────────────────────────────────────────────
    juce::TextButton calibrateButton { "Calibrate" };
    bool             calibrating_        = false;
    int              calibTicks_         = 0;
    int              resultDisplayTicks_ = 0;
    juce::String     calibResultText_;

    // ── Drum-mode controls ────────────────────────────────────────────────────
    juce::Slider drumNoteSlider;
    juce::Label  drumNoteLabel;

    // ── APVTS attachments ─────────────────────────────────────────────────────
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    ComboAttachment  modeAttachment;
    SliderAttachment sensitivityAttachment;
    SliderAttachment noiseGateAttachment;
    SliderAttachment smoothingAttachment;
    SliderAttachment minNoteLengthAttachment;
    ComboAttachment  scaleTypeAttachment;
    ComboAttachment  rootNoteAttachment;
    ButtonAttachment pitchBendAttachment;
    SliderAttachment drumNoteAttachment;

    // ── Display state ─────────────────────────────────────────────────────────
    juce::String displayNote    { "--" };
    juce::String displayCents   { "" };
    float        stabilityScore { 0.0f };
    float        prevHz         { 0.0f };
    float        smoothedDelta  { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
