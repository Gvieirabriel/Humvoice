#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

// Returns the detected fundamental frequency in Hz, or 0.0f if the signal is
// too weak or no clear pitch is found.  Uses normalised autocorrelation so the
// threshold is independent of signal level.
//
// buf       – circular buffer (power-of-two size)
// bufSize   – number of elements (must be a power of two)
// writeIndex – next-write position (oldest sample starts here)
// sampleRate – current sample rate
// Returns MIDI note number (0-127) for a given frequency, or -1 for silence/out-of-range.
static int hzToMidi(float hz)
{
    if (hz <= 0.0f)
        return -1;
    const int note = static_cast<int>(std::round(69.0f + 12.0f * std::log2(hz / 440.0f)));
    return (note >= 0 && note <= 127) ? note : -1;
}

// Returns the absolute deviation in cents between hz and the centre frequency
// of midiNote.  Returns a large sentinel if either argument is invalid.
static float centsBetween(float hz, int midiNote)
{
    if (hz <= 0.0f || midiNote < 0)
        return 9999.0f;
    const float noteHz = 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);
    return std::abs(1200.0f * std::log2(hz / noteHz));
}

// sensitivity [0,1]: 0 = detects very quiet signals, 1 = requires loud signals.
// Mapped logarithmically: threshold = 10^(-6 + sensitivity * 4), i.e. [1e-6, 1e-2].
static float detectPitch(const float* buf, int bufSize, int writeIndex,
                         double sampleRate, float sensitivity)
{
    // Search range: 80 Hz – 1 000 Hz (covers singing / humming voice)
    const int minLag = static_cast<int>(sampleRate / 1000.0);
    const int maxLag = std::min(static_cast<int>(sampleRate / 80.0), bufSize - 1);

    if (minLag >= maxLag)
        return 0.0f;

    // Autocorrelation at lag 0 == signal energy; gate on minimum level
    float corr0 = 0.0f;
    for (int i = 0; i < bufSize; ++i)
        corr0 += buf[i] * buf[i];

    const float rmsThreshold = std::pow(10.0f, -6.0f + sensitivity * 4.0f);
    if (corr0 / static_cast<float>(bufSize) < rmsThreshold)
        return 0.0f;

    // Find lag with highest autocorrelation in the search range
    float bestCorr = 0.0f;
    int   bestLag  = 0;

    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        float corr = 0.0f;
        for (int i = 0; i < bufSize - lag; ++i)
        {
            const int idxA = (writeIndex + i)       & (bufSize - 1);
            const int idxB = (writeIndex + i + lag) & (bufSize - 1);
            corr += buf[idxA] * buf[idxB];
        }
        if (corr > bestCorr)
        {
            bestCorr = corr;
            bestLag  = lag;
        }
    }

    // Require the best correlation to be at least 30 % of the zero-lag energy
    if (bestLag == 0 || bestCorr < 0.30f * corr0)
        return 0.0f;

    // Octave-error correction: if half the best lag is also in range and its
    // correlation is ≥ 85 % of the best, prefer it (true pitch is one octave up).
    // Repeat once more to catch double-octave errors.
    for (int pass = 0; pass < 2; ++pass)
    {
        const int halfLag = bestLag / 2;
        if (halfLag < minLag)
            break;
        float halfCorr = 0.0f;
        for (int i = 0; i < bufSize - halfLag; ++i)
        {
            const int idxA = (writeIndex + i)           & (bufSize - 1);
            const int idxB = (writeIndex + i + halfLag) & (bufSize - 1);
            halfCorr += buf[idxA] * buf[idxB];
        }
        if (halfCorr >= 0.85f * bestCorr)
        {
            bestLag  = halfLag;
            bestCorr = halfCorr;
        }
        else
            break;
    }

    return static_cast<float>(sampleRate / static_cast<double>(bestLag));
}

juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    return {
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "sensitivity", 1 },
            "Sensitivity",
            juce::NormalisableRange<float> (0.0f, 1.0f),
            0.5f)
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
    juce::ignoreUnused(samplesPerBlock);
    currentSampleRate  = sampleRate;
    minNoteHoldSamples = static_cast<int>(sampleRate * kMinNoteHoldMs / 1000.0f);
    // EMA coefficient: ~20 ms time constant, block-size-aware
    smoothingAlpha = 1.0f - std::exp(-static_cast<float>(samplesPerBlock)
                                     / (static_cast<float>(sampleRate) * 0.020f));
    smoothedPitchHz = 0.0f;
    monoCircularBuffer.fill(0.0f);
    monoWriteIndex = 0;
    currentPitchHz.store(0.0f);
    lastMidiNote    = -1;
    candidateNote   = -1;
    candidateBlocks = 0;
    noteHoldSamples = 0;
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
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Mix all input channels to mono and write into circular buffer
    for (int i = 0; i < numSamples; ++i)
    {
        float mono = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            mono += buffer.getSample(ch, i);
        if (numChannels > 0)
            mono /= static_cast<float>(numChannels);

        monoCircularBuffer[monoWriteIndex] = mono;
        monoWriteIndex = (monoWriteIndex + 1) & (kMonoBufferSize - 1);
    }

    // Pitch detection
    const float sensitivity = apvts.getRawParameterValue("sensitivity")->load();
    const float hz = detectPitch(monoCircularBuffer.data(),
                                 kMonoBufferSize,
                                 monoWriteIndex,
                                 currentSampleRate,
                                 sensitivity);
    // Smooth frequency for display; reset immediately on silence to avoid
    // the display lagging behind when the signal stops.
    if (hz > 0.0f)
        smoothedPitchHz = smoothingAlpha * hz + (1.0f - smoothingAlpha) * smoothedPitchHz;
    else
        smoothedPitchHz = 0.0f;
    currentPitchHz.store(smoothedPitchHz);

    // --- MIDI note generation ---
    noteHoldSamples += numSamples;

    // Hysteresis: if the detected Hz is within kPitchHysteresisCents of the
    // currently active note, treat it as unchanged and abort any transition.
    const bool withinDeadband =
        (hz > 0.0f && lastMidiNote >= 0)
            ? centsBetween(hz, lastMidiNote) < kPitchHysteresisCents
            : (hz <= 0.0f && lastMidiNote < 0); // both silent

    if (withinDeadband)
    {
        candidateNote   = lastMidiNote;
        candidateBlocks = 0;
    }
    else
    {
        const int detectedNote = hzToMidi(hz);
        if (detectedNote == candidateNote)
            ++candidateBlocks;
        else
        {
            candidateNote   = detectedNote;
            candidateBlocks = 1;
        }

        // Commit only after the candidate has held for kCandidateBlocks
        // AND the active note has been held for at least kMinNoteHoldMs.
        if (candidateBlocks >= kCandidateBlocks
            && candidateNote != lastMidiNote
            && noteHoldSamples >= minNoteHoldSamples)
        {
            if (lastMidiNote >= 0)
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, lastMidiNote), 0);

            if (candidateNote >= 0)
                midiMessages.addEvent(
                    juce::MidiMessage::noteOn(1, candidateNote, static_cast<juce::uint8>(kVelocity)), 0);

            lastMidiNote    = candidateNote;
            noteHoldSamples = 0;
        }
    }
}

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

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
