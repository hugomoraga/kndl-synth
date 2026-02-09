#include "PluginProcessor.h"
#include "PluginEditor.h"

KndlSynthAudioProcessor::KndlSynthAudioProcessor()
    : juce::AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

const juce::String KndlSynthAudioProcessor::getName() const { return JucePlugin_Name; }

bool KndlSynthAudioProcessor::acceptsMidi() const { return true; }
bool KndlSynthAudioProcessor::producesMidi() const { return false; }
bool KndlSynthAudioProcessor::isMidiEffect() const { return false; }
double KndlSynthAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int KndlSynthAudioProcessor::getNumPrograms() { return 1; }
int KndlSynthAudioProcessor::getCurrentProgram() { return 0; }
void KndlSynthAudioProcessor::setCurrentProgram (int) {}
const juce::String KndlSynthAudioProcessor::getProgramName (int) { return {}; }
void KndlSynthAudioProcessor::changeProgramName (int, const juce::String&) {}

void KndlSynthAudioProcessor::prepareToPlay (double, int) {}
void KndlSynthAudioProcessor::releaseResources() {}

#if ! JucePlugin_PreferredChannelConfigurations
bool KndlSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // We only support stereo output in this MVP.
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}
#endif

void KndlSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear(); // silence for now (we'll add oscillator next)

    // Consume MIDI so hosts stop complaining, but do nothing yet.
    midi.clear();
}

bool KndlSynthAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* KndlSynthAudioProcessor::createEditor()
{
    return new KndlSynthAudioProcessorEditor (*this);
}

void KndlSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // MVP: no parameters yet.
    juce::MemoryOutputStream mos(destData, true);
    mos.writeString("kndl_synth_state_v0");
}

void KndlSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::MemoryInputStream mis(data, static_cast<size_t>(sizeInBytes), false);
    (void) mis.readString();
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KndlSynthAudioProcessor();
}
