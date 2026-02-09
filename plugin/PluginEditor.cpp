#include "PluginEditor.h"

KndlSynthAudioProcessorEditor::KndlSynthAudioProcessorEditor (KndlSynthAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (420, 240);
}

void KndlSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    g.setColour (juce::Colours::white);
    g.setFont (18.0f);
    g.drawText ("KndlSynth (MVP) — VST3 loaded ✅", getLocalBounds(), juce::Justification::centred, true);
}

void KndlSynthAudioProcessorEditor::resized()
{
}
