#include "PluginEditor.h"
#include <cmath>

// ── helpers ──────────────────────────────────────────────────────────────────

static const char* kNoteNames[] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

static std::pair<juce::String, float> hzToDisplay(float hz)
{
    if (hz <= 0.0f)
        return { "--", 0.0f };
    const float exactMidi = 69.0f + 12.0f * std::log2(hz / 440.0f);
    const int   midiNote  = static_cast<int>(std::round(exactMidi));
    if (midiNote < 0 || midiNote > 127)
        return { "--", 0.0f };
    const float cents  = (exactMidi - static_cast<float>(midiNote)) * 100.0f;
    const int   octave = midiNote / 12 - 1;
    return { juce::String(kNoteNames[midiNote % 12]) + juce::String(octave), cents };
}

static juce::String hzToNoteName(float hz)   { return hzToDisplay(hz).first; }
static juce::String midiToNoteName(int note)
{
    if (note < 0 || note > 127) return "--";
    return juce::String(kNoteNames[note % 12]) + juce::String(note / 12 - 1);
}

// ── setup helpers ─────────────────────────────────────────────────────────────

static void setupSlider(juce::Slider& s, juce::Label& l, const juce::String& name,
                        juce::Component* parent)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    parent->addAndMakeVisible(s);
    l.setText(name, juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centred);
    l.setFont(juce::Font(12.0f));
    parent->addAndMakeVisible(l);
}

static void setupCombo(juce::ComboBox& c, juce::Label& l, const juce::String& name,
                       juce::Component* parent)
{
    parent->addAndMakeVisible(c);
    l.setText(name, juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centred);
    l.setFont(juce::Font(12.0f));
    parent->addAndMakeVisible(l);
}

// ── PluginEditor ──────────────────────────────────────────────────────────────

PluginEditor::PluginEditor(PluginProcessor& p)
    : juce::AudioProcessorEditor(&p), processorRef(p),
      modeAttachment          (p.apvts, "pluginMode",   modeBox),
      sensitivityAttachment   (p.apvts, "sensitivity",   sensitivitySlider),
      noiseGateAttachment     (p.apvts, "noiseGate",     noiseGateSlider),
      smoothingAttachment     (p.apvts, "smoothing",     smoothingSlider),
      minNoteLengthAttachment (p.apvts, "minNoteLength", minNoteLengthSlider),
      scaleTypeAttachment     (p.apvts, "scaleType",     scaleTypeBox),
      rootNoteAttachment      (p.apvts, "rootNote",      rootNoteBox),
      drumNoteAttachment      (p.apvts, "drumNote",      drumNoteSlider)
{
    // Mode selector (header)
    modeBox.addItem("Pitch", 1);
    modeBox.addItem("Drum",  2);
    modeLabel.setText("Mode", juce::dontSendNotification);
    modeLabel.setFont(juce::Font(12.0f));
    modeLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(modeBox);
    addAndMakeVisible(modeLabel);

    // Pitch-mode knobs
    setupSlider(sensitivitySlider,   sensitivityLabel,   "Sensitivity", this);
    setupSlider(noiseGateSlider,     noiseGateLabel,     "Noise Gate",  this);
    setupSlider(smoothingSlider,     smoothingLabel,     "Smoothing",   this);
    setupSlider(minNoteLengthSlider, minNoteLengthLabel, "Min Note",    this);

    scaleTypeBox.addItem("Off",   1);
    scaleTypeBox.addItem("Major", 2);
    scaleTypeBox.addItem("Minor", 3);
    setupCombo(scaleTypeBox, scaleTypeLabel, "Scale", this);

    rootNoteBox.addItem("C",  1);  rootNoteBox.addItem("C#", 2);
    rootNoteBox.addItem("D",  3);  rootNoteBox.addItem("D#", 4);
    rootNoteBox.addItem("E",  5);  rootNoteBox.addItem("F",  6);
    rootNoteBox.addItem("F#", 7);  rootNoteBox.addItem("G",  8);
    rootNoteBox.addItem("G#", 9);  rootNoteBox.addItem("A",  10);
    rootNoteBox.addItem("A#", 11); rootNoteBox.addItem("B",  12);
    setupCombo(rootNoteBox, rootNoteLabel, "Root", this);

    calibrateButton.onClick = [this]
    {
        if (!calibrating_)
        {
            calibrating_ = true;
            calibTicks_  = 0;
            processorRef.startCalibration();
            calibrateButton.setButtonText("Stop  (0/5s)");
        }
        else
        {
            stopCalibration();
        }
    };
    addAndMakeVisible(calibrateButton);

    // Drum-mode controls
    drumNoteSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    drumNoteSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
    drumNoteSlider.setTextValueSuffix("");
    addAndMakeVisible(drumNoteSlider);

    drumNoteLabel.setText("Drum Note", juce::dontSendNotification);
    drumNoteLabel.setFont(juce::Font(12.0f));
    drumNoteLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(drumNoteLabel);

    startTimerHz(20);
    setSize(480, 310);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
}

// ── Calibration control ───────────────────────────────────────────────────────

void PluginEditor::stopCalibration()
{
    processorRef.stopCalibration();
    calibrating_ = false;
    calibTicks_  = 0;
    calibrateButton.setButtonText("Calibrate");

    const auto [minHz, maxHz] = processorRef.getCalibrationRange();
    if (minHz < maxHz)
    {
        calibResultText_    = hzToNoteName(minHz) + " \xe2\x80\x93 " + hzToNoteName(maxHz);
        resultDisplayTicks_ = 60;
    }
    else
    {
        calibResultText_    = "No voice detected";
        resultDisplayTicks_ = 40;
    }
}

// ── Timer ─────────────────────────────────────────────────────────────────────

void PluginEditor::timerCallback()
{
    // Toggle pitch/drum controls based on current mode parameter
    const bool isDrum = (processorRef.apvts.getRawParameterValue("pluginMode")->load() >= 0.5f);

    smoothingSlider    .setVisible(!isDrum);
    smoothingLabel     .setVisible(!isDrum);
    minNoteLengthSlider.setVisible(!isDrum);
    minNoteLengthLabel .setVisible(!isDrum);
    scaleTypeBox       .setVisible(!isDrum);
    scaleTypeLabel     .setVisible(!isDrum);
    rootNoteBox        .setVisible(!isDrum);
    rootNoteLabel      .setVisible(!isDrum);
    calibrateButton    .setVisible(!isDrum);
    drumNoteSlider     .setVisible(isDrum);
    drumNoteLabel      .setVisible(isDrum);

    // Calibration countdown (pitch mode only)
    if (calibrating_)
    {
        ++calibTicks_;
        calibrateButton.setButtonText("Stop  (" + juce::String(calibTicks_ / 20) + "/5s)");
        displayNote    = "...";
        displayCents   = "Listening";
        stabilityScore = 0.0f;
        if (calibTicks_ >= 100)
            stopCalibration();
        repaint();
        return;
    }

    if (resultDisplayTicks_ > 0)
        --resultDisplayTicks_;

    if (isDrum)
    {
        const int note = static_cast<int>(
            processorRef.apvts.getRawParameterValue("drumNote")->load());
        displayNote    = midiToNoteName(note);
        displayCents   = "drum";
        stabilityScore = 0.0f;
    }
    else
    {
        const float hz = processorRef.getDetectedPitchHz();

        if (prevHz > 0.0f && hz > 0.0f)
        {
            const float delta = std::abs(1200.0f * std::log2(hz / prevHz));
            smoothedDelta = 0.75f * smoothedDelta + 0.25f * delta;
        }
        else
        {
            smoothedDelta *= 0.75f;
        }
        prevHz = hz;

        stabilityScore = juce::jlimit(0.0f, 1.0f, 1.0f - smoothedDelta / 100.0f);

        auto [note, cents] = hzToDisplay(hz);
        displayNote  = note;
        displayCents = (hz > 0.0f)
                           ? (cents >= 0 ? "+" : "") + juce::String(static_cast<int>(cents)) + "c"
                           : juce::String("");
    }

    repaint();
}

// ── paint ─────────────────────────────────────────────────────────────────────

void PluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2b2b2b));

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(18.0f, juce::Font::bold));
    // Title in the left portion of the header
    g.drawText("Humvoice", juce::Rectangle<int>(0, 0, getWidth() - 160, 40),
               juce::Justification::centred);

    const int panelY = 45 + 90 + 16 + 4;
    paintPitchDisplay(g, { 0, panelY, getWidth(), 58 });
}

void PluginEditor::paintPitchDisplay(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(juce::Colour(0xff1e1e1e));
    g.fillRoundedRectangle(area.reduced(8, 2).toFloat(), 6.0f);

    auto inner = area.reduced(16, 6);

    // Calibration result override
    if (!calibrating_ && resultDisplayTicks_ > 0 && calibResultText_.isNotEmpty())
    {
        g.setColour(juce::Colour(0xff44cc66));
        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.drawText("Range: " + calibResultText_, inner, juce::Justification::centred);
        return;
    }

    const bool isDrum = (displayCents == "drum");

    // Note name
    g.setColour(displayNote == "--" ? juce::Colour(0xff555555)
                                    : isDrum ? juce::Colour(0xffff9944)
                                             : juce::Colours::white);
    g.setFont(juce::Font(28.0f, juce::Font::bold));
    g.drawText(displayNote, inner.removeFromLeft(90), juce::Justification::centredLeft);

    if (isDrum)
    {
        g.setColour(juce::Colour(0xffff9944));
        g.setFont(juce::Font(12.0f));
        g.drawText("DRUM MODE", inner, juce::Justification::centredLeft);
        return;
    }

    // Pitch mode: cents + stability bar
    g.setColour(juce::Colour(0xffaaaaaa));
    g.setFont(juce::Font(13.0f));
    g.drawText(displayCents, inner.removeFromLeft(40), juce::Justification::centredLeft);

    const auto barBounds = inner.withTrimmedLeft(inner.getWidth() / 4)
                                .withSizeKeepingCentre(inner.getWidth() * 3 / 4, 10);
    g.setColour(juce::Colour(0xff444444));
    g.fillRoundedRectangle(barBounds.toFloat(), 4.0f);

    if (stabilityScore > 0.0f)
    {
        const auto filled = barBounds.withWidth(
            static_cast<int>(barBounds.getWidth() * stabilityScore));
        g.setColour(stabilityScore > 0.6f ? juce::Colour(0xff44cc66) : juce::Colour(0xffcc8833));
        g.fillRoundedRectangle(filled.toFloat(), 4.0f);
    }

    g.setColour(juce::Colour(0xff888888));
    g.setFont(juce::Font(10.0f));
    g.drawText("stability", inner.withTrimmedLeft(inner.getWidth() / 4)
                                  .translated(0, 10), juce::Justification::centredLeft);
}

// ── resized ───────────────────────────────────────────────────────────────────

void PluginEditor::resized()
{
    const int w       = getWidth();
    const int knobTop = 45;
    const int knobH   = 90;
    const int labelH  = 16;
    const int knobW   = w / 4;

    // Mode selector in header (right side)
    modeLabel.setBounds(w - 160, 8, 50,  24);
    modeBox  .setBounds(w - 108, 8, 100, 24);

    // Knobs
    auto knobArea  = [&](int col) { return juce::Rectangle<int>(col * knobW, knobTop, knobW, knobH); };
    auto labelArea = [&](int col) { return juce::Rectangle<int>(col * knobW, knobTop + knobH, knobW, labelH); };

    sensitivitySlider  .setBounds(knobArea(0));
    noiseGateSlider    .setBounds(knobArea(1));
    smoothingSlider    .setBounds(knobArea(2));
    minNoteLengthSlider.setBounds(knobArea(3));

    sensitivityLabel  .setBounds(labelArea(0));
    noiseGateLabel    .setBounds(labelArea(1));
    smoothingLabel    .setBounds(labelArea(2));
    minNoteLengthLabel.setBounds(labelArea(3));

    // Pitch panel painted in paint() — no child bounds needed

    // Combos
    const int comboY  = knobTop + knobH + labelH + 4 + 58 + 8;
    const int comboW  = (w - 20) / 2 - 10;

    scaleTypeLabel.setBounds(10,         comboY,           comboW, labelH);
    scaleTypeBox  .setBounds(10,         comboY + labelH,  comboW, 24);
    rootNoteLabel .setBounds(w / 2 + 10, comboY,           comboW, labelH);
    rootNoteBox   .setBounds(w / 2 + 10, comboY + labelH,  comboW, 24);

    // Bottom row: calibrate button (pitch) OR drum note slider (drum)
    const int bottomY = comboY + labelH + 24 + 10;

    calibrateButton.setBounds(10, bottomY, w - 20, 28);

    // Drum note: label on left, slider fills the rest
    drumNoteLabel .setBounds(10,  bottomY,     90,       28);
    drumNoteSlider.setBounds(105, bottomY,     w - 115,  28);
}
