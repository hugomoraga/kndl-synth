#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class KndlSynthAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit KndlSynthAudioProcessorEditor (KndlSynthAudioProcessor&);
    ~KndlSynthAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    KndlSynthAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KndlSynthAudioProcessorEditor)
};
