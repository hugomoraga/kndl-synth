#include "PluginProcessor.h"
#include "PluginEditor.h"

KndlSynthAudioProcessor::KndlSynthAudioProcessor()
    : juce::AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", kndl::createParameterLayout()),
      synth(apvts),
      presetManager(apvts)
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

void KndlSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.prepare(sampleRate, samplesPerBlock);
    sequencer.setSampleRate(sampleRate);
}

void KndlSynthAudioProcessor::releaseResources() {}

#if ! JucePlugin_PreferredChannelConfigurations
bool KndlSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}
#endif

void KndlSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    // Inject internal sequencer notes (before everything else)
    sequencer.processBlock(midiMessages, buffer.getNumSamples());
    
    // Track MIDI activity
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();
        if (message.isNoteOn())
        {
            lastMidiNote.store(message.getNoteNumber());
            midiActivity.store(true);
        }
    }
    
    // Clear buffer before processing
    buffer.clear();
    
    // Process synth
    synth.processBlock(buffer, midiMessages);
    
    // Calculate RMS level for debug display
    float rms = 0.0f;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        rms += buffer.getRMSLevel(channel, 0, buffer.getNumSamples());
    }
    if (buffer.getNumChannels() > 0)
        rms /= static_cast<float>(buffer.getNumChannels());
    
    currentLevel.store(rms);
}

bool KndlSynthAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* KndlSynthAudioProcessor::createEditor()
{
    return new KndlSynthAudioProcessorEditor(*this);
}

void KndlSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void KndlSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KndlSynthAudioProcessor();
}
