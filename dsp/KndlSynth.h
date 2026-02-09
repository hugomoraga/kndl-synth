#pragma once

#include <JuceHeader.h>
#include "../plugin/Logger.h"
#include "core/Parameters.h"
#include "core/Voice.h"
#include "core/VoiceManager.h"
#include "core/ModulationMatrix.h"
#include "modulators/LFO.h"
#include "effects/Delay.h"
#include "effects/Chorus.h"
#include "effects/Distortion.h"
#include "effects/Reverb.h"
#include "effects/DCBlocker.h"

namespace kndl {

/**
 * Estructura para debug - valores de cada componente
 */
struct DebugInfo
{
    // Oscillators (de la primera voz activa)
    float osc1Value = 0.0f;
    float osc2Value = 0.0f;
    float subValue = 0.0f;
    float mixedOsc = 0.0f;
    
    // Filter
    float filterInput = 0.0f;
    float filterOutput = 0.0f;
    float filterCutoff = 0.0f;
    
    // Envelopes
    float ampEnvValue = 0.0f;
    float filterEnvValue = 0.0f;
    
    // LFOs
    float lfo1Value = 0.0f;
    float lfo2Value = 0.0f;
    
    // Output
    float voiceOutput = 0.0f;
    float masterOutput = 0.0f;
    
    // Status flags
    bool hasNaN = false;
    bool hasInf = false;
};

/**
 * Motor principal del sintetizador.
 * Orquesta todos los componentes: voces, LFOs, modulación, etc.
 */
class KndlSynth
{
public:
    explicit KndlSynth(juce::AudioProcessorValueTreeState& apvts);
    
    void prepare(double newSampleRate, int newSamplesPerBlock);
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    void handleMidiMessage(const juce::MidiMessage& message);
    
    int getActiveVoiceCount() const { return voiceManager.getActiveVoiceCount(); }
    
    // Debug info
    const DebugInfo& getDebugInfo() const { return debugInfo; }
    
private:
    float processSample();
    void cacheParameterPointers();
    void updateParametersFromAPVTS();
    
    juce::AudioProcessorValueTreeState& parameters;
    
    double sampleRate = 44100.0;
    int samplesPerBlock = 512;
    
    VoiceManager voiceManager;
    LFO lfo1;
    LFO lfo2;
    ModulationMatrix modMatrix;
    
    juce::SmoothedValue<float> masterGain;
    float modWheelValue = 0.0f;
    
    // Parameter pointers (cached for performance)
    std::atomic<float>* osc1WaveformParam = nullptr;
    std::atomic<float>* osc1LevelParam = nullptr;
    std::atomic<float>* osc1DetuneParam = nullptr;
    std::atomic<float>* osc1OctaveParam = nullptr;
    
    std::atomic<float>* osc2WaveformParam = nullptr;
    std::atomic<float>* osc2LevelParam = nullptr;
    std::atomic<float>* osc2DetuneParam = nullptr;
    std::atomic<float>* osc2OctaveParam = nullptr;
    
    std::atomic<float>* subLevelParam = nullptr;
    std::atomic<float>* subOctaveParam = nullptr;
    
    std::atomic<float>* filterCutoffParam = nullptr;
    std::atomic<float>* filterResonanceParam = nullptr;
    std::atomic<float>* filterTypeParam = nullptr;
    std::atomic<float>* filterDriveParam = nullptr;
    std::atomic<float>* filterEnvAmountParam = nullptr;
    
    std::atomic<float>* ampAttackParam = nullptr;
    std::atomic<float>* ampDecayParam = nullptr;
    std::atomic<float>* ampSustainParam = nullptr;
    std::atomic<float>* ampReleaseParam = nullptr;
    
    std::atomic<float>* filterAttackParam = nullptr;
    std::atomic<float>* filterDecayParam = nullptr;
    std::atomic<float>* filterSustainParam = nullptr;
    std::atomic<float>* filterReleaseParam = nullptr;
    
    std::atomic<float>* lfo1RateParam = nullptr;
    std::atomic<float>* lfo1WaveformParam = nullptr;
    std::atomic<float>* lfo1SyncParam = nullptr;
    
    std::atomic<float>* lfo2RateParam = nullptr;
    std::atomic<float>* lfo2WaveformParam = nullptr;
    std::atomic<float>* lfo2SyncParam = nullptr;
    
    std::atomic<float>* masterGainParam = nullptr;
    
    // Effects
    Distortion distortion;
    Chorus chorus;
    Delay delay;
    Reverb reverb;
    
    // DC Blocker (removes DC offset from output)
    DCBlocker dcBlockerL;
    DCBlocker dcBlockerR;
    
    // Effect parameter pointers
    std::atomic<float>* distortionEnableParam = nullptr;
    std::atomic<float>* distortionDriveParam = nullptr;
    std::atomic<float>* distortionMixParam = nullptr;
    
    std::atomic<float>* chorusEnableParam = nullptr;
    std::atomic<float>* chorusRateParam = nullptr;
    std::atomic<float>* chorusDepthParam = nullptr;
    std::atomic<float>* chorusMixParam = nullptr;
    
    std::atomic<float>* delayEnableParam = nullptr;
    std::atomic<float>* delayTimeParam = nullptr;
    std::atomic<float>* delayFeedbackParam = nullptr;
    std::atomic<float>* delayMixParam = nullptr;
    
    std::atomic<float>* reverbEnableParam = nullptr;
    std::atomic<float>* reverbSizeParam = nullptr;
    std::atomic<float>* reverbDampParam = nullptr;
    std::atomic<float>* reverbMixParam = nullptr;
    
    // Debug info (updated each sample for display)
    DebugInfo debugInfo;
};

} // namespace kndl
