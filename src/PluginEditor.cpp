#include "PluginEditor.h"

PluginEditor::PluginEditor(PluginProcessor& p)
    : juce::AudioProcessorEditor(&p), processorRef(p)
{
    setSize(400, 300);
}

PluginEditor::~PluginEditor()
{
}

void PluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawFittedText("Humvoice", getLocalBounds(), juce::Justification::centredTop, 1);

    g.setFont(16.0f);
    g.drawFittedText("Minimal VST3 Plugin", getLocalBounds().withTrimmedTop(60), juce::Justification::centred, 1);
}

void PluginEditor::resized()
{
}
