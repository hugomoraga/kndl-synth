#pragma once

#include <JuceHeader.h>
#include "../dsp/KndlSynth.h"
#include "../dsp/core/Parameters.h"
#include "PresetManager.h"

class KndlSynthAudioProcessor final : public juce::AudioProcessor
{
public:
    KndlSynthAudioProcessor();
    ~KndlSynthAudioProcessor() override = default;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #if ! JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    kndl::KndlSynth& getSynth() { return synth; }
    kndl::PresetManager& getPresetManager() { return presetManager; }
    
    // Debug info
    int getActiveVoiceCount() const { return synth.getActiveVoiceCount(); }
    float getCurrentLevel() const { return currentLevel.load(); }
    int getLastMidiNote() const { return lastMidiNote.load(); }
    bool hasMidiActivity() const { return midiActivity.load(); }
    void clearMidiActivity() { midiActivity.store(false); }
    const kndl::DebugInfo& getDebugInfo() const { return synth.getDebugInfo(); }

private:
    juce::AudioProcessorValueTreeState apvts;
    kndl::KndlSynth synth;
    kndl::PresetManager presetManager;
    
    // Debug state (atomic for thread safety)
    std::atomic<float> currentLevel { 0.0f };
    std::atomic<int> lastMidiNote { -1 };
    std::atomic<bool> midiActivity { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KndlSynthAudioProcessor)
};
