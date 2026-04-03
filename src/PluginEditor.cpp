#include "PluginEditor.h"
#include <cmath>

// ── Layout constants ──────────────────────────────────────────────────────────

static constexpr int kW        = 480;
static constexpr int kH        = 400;
static constexpr int kPad      = 14;
static constexpr int kLabelW   = 110;   // left-column label width
static constexpr int kSectionH = 18;
static constexpr int kRowH     = 24;
static constexpr int kGap      = 8;
static constexpr int kHeaderH  = 40;
static constexpr int kDisplayH = 56;

// Derived x coords
static constexpr int kCtrlX = kPad + kLabelW + 4;          // 128
static constexpr int kCtrlW = kW - kCtrlX - kPad;          // 338

// Detection section (shared pitch + drum)
static constexpr int kDetSecY  = kHeaderH + kDisplayH + kGap;  // 104
static constexpr int kRow1Y    = kDetSecY + kSectionH;          // 122  sensitivity
static constexpr int kRow2Y    = kRow1Y   + kRowH;              // 146  noise gate

// Pitch-only rows
static constexpr int kRow3Y    = kRow2Y + kRowH;                // 170  smoothing
static constexpr int kRow4Y    = kRow3Y + kRowH;                // 194  min note
static constexpr int kMusSecY  = kRow4Y + kRowH + kGap;         // 226  "MUSICAL"
static constexpr int kRow5Y    = kMusSecY + kSectionH;          // 244  scale
static constexpr int kRow6Y    = kRow5Y  + kRowH;              // 268  root note
static constexpr int kExprSecY = kRow6Y  + kRowH + kGap;       // 300  "EXPRESSION"
static constexpr int kRow7Y    = kExprSecY + kSectionH;         // 318  pitch bend toggle
static constexpr int kCalibY   = kRow7Y  + 28 + kGap;          // 354  calibrate button

// Drum-only rows
static constexpr int kDrumSecY  = kRow2Y + kRowH + kGap;        // 178  "DRUM"
static constexpr int kDrumNoteY = kDrumSecY + kSectionH;         // 196  drum note slider

// ── Helpers ───────────────────────────────────────────────────────────────────

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

static juce::String hzToNoteName(float hz)  { return hzToDisplay(hz).first; }
static juce::String midiToNoteName(int note)
{
    if (note < 0 || note > 127) return "--";
    return juce::String(kNoteNames[note % 12]) + juce::String(note / 12 - 1);
}

// ── PluginEditor ──────────────────────────────────────────────────────────────

PluginEditor::PluginEditor(PluginProcessor& p)
    : juce::AudioProcessorEditor(&p), processorRef(p),
      modeAttachment          (p.apvts, "pluginMode",      modeBox),
      sensitivityAttachment   (p.apvts, "sensitivity",      sensitivitySlider),
      noiseGateAttachment     (p.apvts, "noiseGate",        noiseGateSlider),
      smoothingAttachment     (p.apvts, "smoothing",        smoothingSlider),
      minNoteLengthAttachment (p.apvts, "minNoteLength",    minNoteLengthSlider),
      scaleTypeAttachment     (p.apvts, "scaleType",        scaleTypeBox),
      rootNoteAttachment      (p.apvts, "rootNote",         rootNoteBox),
      pitchBendAttachment     (p.apvts, "enablePitchBend",  pitchBendToggle),
      drumNoteAttachment      (p.apvts, "drumNote",         drumNoteSlider)
{
    // ── Section labels ──────────────────────────────────────────────────────
    auto setupSection = [this](juce::Label& l, const juce::String& text)
    {
        l.setText(text, juce::dontSendNotification);
        l.setFont(juce::Font(10.0f, juce::Font::bold));
        l.setColour(juce::Label::textColourId, juce::Colour(0xff8e8e93));
        l.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(l);
    };
    setupSection(sectionDetection,  "DETECTION");
    setupSection(sectionMusical,    "MUSICAL");
    setupSection(sectionExpression, "EXPRESSION");
    setupSection(sectionDrum,       "DRUM");

    // ── Horizontal sliders ──────────────────────────────────────────────────
    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& text)
    {
        s.setSliderStyle(juce::Slider::LinearHorizontal);
        s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, kRowH);
        addAndMakeVisible(s);
        l.setText(text, juce::dontSendNotification);
        l.setFont(juce::Font(12.0f));
        l.setColour(juce::Label::textColourId, juce::Colour(0xffebebf5));
        l.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(l);
    };
    setupSlider(sensitivitySlider,   sensitivityLabel,   "Sensitivity");
    setupSlider(noiseGateSlider,     noiseGateLabel,     "Noise Gate");
    setupSlider(smoothingSlider,     smoothingLabel,     "Smoothing");
    setupSlider(minNoteLengthSlider, minNoteLengthLabel, "Min Note");
    setupSlider(drumNoteSlider,      drumNoteLabel,      "Drum Note");

    // ── Combo boxes ─────────────────────────────────────────────────────────
    auto setupCombo = [this](juce::ComboBox& c, juce::Label& l, const juce::String& text)
    {
        addAndMakeVisible(c);
        l.setText(text, juce::dontSendNotification);
        l.setFont(juce::Font(12.0f));
        l.setColour(juce::Label::textColourId, juce::Colour(0xffebebf5));
        l.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(l);
    };

    modeBox.addItem("Pitch", 1);
    modeBox.addItem("Drum",  2);
    modeLabel.setText("Mode", juce::dontSendNotification);
    modeLabel.setFont(juce::Font(12.0f));
    modeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8e8e93));
    modeLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(modeBox);
    addAndMakeVisible(modeLabel);

    scaleTypeBox.addItem("Off",   1);
    scaleTypeBox.addItem("Major", 2);
    scaleTypeBox.addItem("Minor", 3);
    setupCombo(scaleTypeBox, scaleTypeLabel, "Scale");

    rootNoteBox.addItem("C",  1);  rootNoteBox.addItem("C#", 2);
    rootNoteBox.addItem("D",  3);  rootNoteBox.addItem("D#", 4);
    rootNoteBox.addItem("E",  5);  rootNoteBox.addItem("F",  6);
    rootNoteBox.addItem("F#", 7);  rootNoteBox.addItem("G",  8);
    rootNoteBox.addItem("G#", 9);  rootNoteBox.addItem("A",  10);
    rootNoteBox.addItem("A#", 11); rootNoteBox.addItem("B",  12);
    setupCombo(rootNoteBox, rootNoteLabel, "Root Note");

    // ── Pitch bend toggle ───────────────────────────────────────────────────
    pitchBendToggle.setButtonText("Pitch Bend");
    pitchBendToggle.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffebebf5));
    addAndMakeVisible(pitchBendToggle);

    // ── Calibrate button ────────────────────────────────────────────────────
    calibrateButton.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff2c2c2e));
    calibrateButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff0a84ff));
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

    startTimerHz(20);
    setSize(kW, kH);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
}

// ── Calibration ───────────────────────────────────────────────────────────────

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

// ── Visibility ────────────────────────────────────────────────────────────────

void PluginEditor::updateVisibility(bool isDrum)
{
    const bool pitch = !isDrum;

    smoothingSlider    .setVisible(pitch);
    smoothingLabel     .setVisible(pitch);
    minNoteLengthSlider.setVisible(pitch);
    minNoteLengthLabel .setVisible(pitch);
    scaleTypeBox       .setVisible(pitch);
    scaleTypeLabel     .setVisible(pitch);
    rootNoteBox        .setVisible(pitch);
    rootNoteLabel      .setVisible(pitch);
    sectionMusical     .setVisible(pitch);
    sectionExpression  .setVisible(pitch);
    pitchBendToggle    .setVisible(pitch);
    calibrateButton    .setVisible(pitch);

    sectionDrum   .setVisible(isDrum);
    drumNoteSlider.setVisible(isDrum);
    drumNoteLabel .setVisible(isDrum);
}

// ── Timer ─────────────────────────────────────────────────────────────────────

void PluginEditor::timerCallback()
{
    const bool isDrum =
        processorRef.apvts.getRawParameterValue("pluginMode")->load() >= 0.5f;

    updateVisibility(isDrum);

    if (calibrating_)
    {
        ++calibTicks_;
        calibrateButton.setButtonText(
            "Stop  (" + juce::String(calibTicks_ / 20) + "/5s)");
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
    // Background
    g.fillAll(juce::Colour(0xff1c1c1e));

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(15.0f, juce::Font::bold));
    g.drawText("HUMVOICE", kPad, 0, 200, kHeaderH, juce::Justification::centredLeft);

    // Display panel
    const auto displayRect = juce::Rectangle<int>(kPad, kHeaderH, kW - 2 * kPad, kDisplayH);
    g.setColour(juce::Colour(0xff2c2c2e));
    g.fillRoundedRectangle(displayRect.toFloat(), 8.0f);
    paintDisplay(g, displayRect);
}

void PluginEditor::paintDisplay(juce::Graphics& g, juce::Rectangle<int> area)
{
    // Calibration result override
    if (!calibrating_ && resultDisplayTicks_ > 0 && calibResultText_.isNotEmpty())
    {
        g.setColour(juce::Colour(0xff30d158));
        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.drawText("Range: " + calibResultText_, area.reduced(16, 0),
                   juce::Justification::centred);
        return;
    }

    auto inner = area.reduced(16, 8);
    const bool isDrum = (displayCents == "drum");

    // Note name
    g.setColour(displayNote == "--" ? juce::Colour(0xff48484a)
                                    : isDrum ? juce::Colour(0xffff9f0a)
                                             : juce::Colours::white);
    g.setFont(juce::Font(26.0f, juce::Font::bold));
    g.drawText(displayNote, inner.removeFromLeft(80), juce::Justification::centredLeft);

    if (isDrum)
    {
        g.setColour(juce::Colour(0xffff9f0a));
        g.setFont(juce::Font(11.0f));
        g.drawText("DRUM MODE", inner, juce::Justification::centredLeft);
        return;
    }

    // Listening state (calibration)
    if (displayCents == "Listening")
    {
        g.setColour(juce::Colour(0xff8e8e93));
        g.setFont(juce::Font(12.0f));
        g.drawText("Listening…", inner, juce::Justification::centredLeft);
        return;
    }

    // Cents deviation
    g.setColour(juce::Colour(0xff8e8e93));
    g.setFont(juce::Font(13.0f));
    g.drawText(displayCents, inner.removeFromLeft(44), juce::Justification::centredLeft);

    // Stability bar
    const auto barBounds = inner.withSizeKeepingCentre(
        juce::jmin(inner.getWidth(), 180), 8);
    g.setColour(juce::Colour(0xff3a3a3c));
    g.fillRoundedRectangle(barBounds.toFloat(), 3.0f);
    if (stabilityScore > 0.0f)
    {
        const auto filled = barBounds.withWidth(
            static_cast<int>(barBounds.getWidth() * stabilityScore));
        g.setColour(stabilityScore > 0.6f ? juce::Colour(0xff30d158)
                                          : juce::Colour(0xffff9f0a));
        g.fillRoundedRectangle(filled.toFloat(), 3.0f);
    }
}

// ── resized ───────────────────────────────────────────────────────────────────

void PluginEditor::resized()
{
    // Header: mode selector
    modeLabel.setBounds(kW - 160, 0,  52, kHeaderH);
    modeBox  .setBounds(kW - 106, 8, 100, 24);

    // Section headers
    sectionDetection .setBounds(kPad, kDetSecY,  kW - 2 * kPad, kSectionH);
    sectionMusical   .setBounds(kPad, kMusSecY,  kW - 2 * kPad, kSectionH);
    sectionExpression.setBounds(kPad, kExprSecY, kW - 2 * kPad, kSectionH);
    sectionDrum      .setBounds(kPad, kDrumSecY, kW - 2 * kPad, kSectionH);

    // Helper: position label + control as a single row
    auto setRow = [](juce::Label& lbl, juce::Component& ctrl, int y, int h = kRowH)
    {
        lbl .setBounds(kPad,   y, kLabelW, h);
        ctrl.setBounds(kCtrlX, y, kCtrlW,  h);
    };

    // Detection (shared)
    setRow(sensitivityLabel,   sensitivitySlider,   kRow1Y);
    setRow(noiseGateLabel,     noiseGateSlider,     kRow2Y);

    // Pitch-only
    setRow(smoothingLabel,     smoothingSlider,     kRow3Y);
    setRow(minNoteLengthLabel, minNoteLengthSlider, kRow4Y);
    setRow(scaleTypeLabel,     scaleTypeBox,        kRow5Y);
    setRow(rootNoteLabel,      rootNoteBox,         kRow6Y);

    // Expression: toggle button spans control column only, label is on the button
    pitchBendToggle.setBounds(kCtrlX, kRow7Y, kCtrlW, 26);

    // Calibrate
    calibrateButton.setBounds(kPad, kCalibY, kW - 2 * kPad, 28);

    // Drum note
    setRow(drumNoteLabel, drumNoteSlider, kDrumNoteY);
}
