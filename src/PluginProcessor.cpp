#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    return {
        std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID { "pluginMode", 1 },
            "Mode",
            juce::StringArray { "Pitch", "Drum" },
            0),
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "sensitivity", 1 },
            "Sensitivity",
            juce::NormalisableRange<float> (0.0f, 1.0f),
            0.5f),
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "noiseGate", 1 },
            "Noise Gate",
            juce::NormalisableRange<float> (-80.0f, 0.0f),
            -40.0f),
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "smoothing", 1 },
            "Smoothing",
            juce::NormalisableRange<float> (0.0f, 200.0f),
            20.0f),
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "minNoteLength", 1 },
            "Min Note Length",
            juce::NormalisableRange<float> (0.0f, 500.0f),
            100.0f),
        std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID { "scaleType", 1 },
            "Scale",
            juce::StringArray { "Off", "Major", "Minor" },
            0),
        std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID { "rootNote", 1 },
            "Root Note",
            juce::StringArray { "C", "C#", "D", "D#", "E", "F",
                                "F#", "G", "G#", "A", "A#", "B" },
            0),
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "drumNote", 1 },
            "Drum Note",
            juce::NormalisableRange<float> (0.0f, 127.0f, 1.0f),
            38.0f)   // default: snare (GM)
    };
}

PluginProcessor::PluginProcessor()
    : juce::AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo())
        .withOutput("Output", juce::AudioChannelSet::stereo())),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

PluginProcessor::~PluginProcessor()
{
}

void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    audioEngine.prepare(sampleRate, samplesPerBlock);
    pitchEngine.prepare(sampleRate, samplesPerBlock);
    midiEngine.prepare(sampleRate, samplesPerBlock);
    drumEngine.prepare(sampleRate, samplesPerBlock);
    currentPitchHz.store(0.0f);
}

void PluginProcessor::releaseResources()
{
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    audioEngine.process(buffer);

    const int   pluginMode     = static_cast<int>(apvts.getRawParameterValue("pluginMode")->load());
    const float sensitivity    = apvts.getRawParameterValue("sensitivity")->load();
    const float noiseGateDb    = apvts.getRawParameterValue("noiseGate")->load();

    if (pluginMode == 1) // Drum mode
    {
        const int drumNote = static_cast<int>(apvts.getRawParameterValue("drumNote")->load());
        drumEngine.process(audioEngine.getRms(), buffer.getNumSamples(),
                           midiMessages, drumNote, noiseGateDb, sensitivity);
        currentPitchHz.store(0.0f);
    }
    else // Pitch mode
    {
        const float smoothingMs     = apvts.getRawParameterValue("smoothing")->load();
        const float minNoteLengthMs = apvts.getRawParameterValue("minNoteLength")->load();
        const int   scaleType       = static_cast<int>(apvts.getRawParameterValue("scaleType")->load());
        const int   rootNote        = static_cast<int>(apvts.getRawParameterValue("rootNote")->load());

        const float hz = pitchEngine.process(audioEngine.getBuffer(),
                                             AudioEngine::kBufferSize,
                                             audioEngine.getWriteIndex(),
                                             sensitivity,
                                             audioEngine.getRms(),
                                             noiseGateDb,
                                             smoothingMs);
        currentPitchHz.store(pitchEngine.getSmoothedHz());

        if (calibrating_.load() && hz > 0.0f)
        {
            const float curMin = calibMinHz_.load();
            const float curMax = calibMaxHz_.load();
            if (hz < curMin) calibMinHz_.store(hz);
            if (hz > curMax) calibMaxHz_.store(hz);
        }

        midiEngine.process(hz, pitchEngine.getSmoothedHz(), audioEngine.getRms(),
                           buffer.getNumSamples(), midiMessages, scaleType, rootNote,
                           minNoteLengthMs);
    }
}

// ── Calibration ───────────────────────────────────────────────────────────────

void PluginProcessor::startCalibration()
{
    calibMinHz_.store(9999.0f);
    calibMaxHz_.store(0.0f);
    calibrating_.store(true);
}

void PluginProcessor::stopCalibration()
{
    calibrating_.store(false);

    const float minHz = calibMinHz_.load();
    const float maxHz = calibMaxHz_.load();

    if (minHz < maxHz)
        pitchEngine.setVocalRange(juce::jmax(50.0f,   minHz * 0.75f),
                                  juce::jmin(1200.0f, maxHz * 1.25f));
}

// ── State persistence ─────────────────────────────────────────────────────────

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor(*this);
}

void PluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
