#pragma once

#include <JuceHeader.h>
#include "../plugin/Logger.h"
#include "core/Parameters.h"
#include "core/Voice.h"
#include "core/VoiceManager.h"
#include "core/ModulationMatrix.h"
#include "modulators/LFO.h"
#include "modulators/Orbit.h"
#include "oscillators/NoiseGenerator.h"
#include "effects/Delay.h"
#include "effects/Chorus.h"
#include "effects/Distortion.h"
#include "effects/Reverb.h"
#include "effects/DCBlocker.h"
#include "effects/OTT.h"
#include "effects/Wavefolder.h"
#include "effects/SafetyLimiter.h"

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
    float filterResonance = 0.0f;
    
    // Envelopes
    float ampEnvValue = 0.0f;
    float filterEnvValue = 0.0f;
    
    // LFOs
    float lfo1Value = 0.0f;
    float lfo2Value = 0.0f;
    
    // Orbit
    float orbitA = 0.0f;
    float orbitB = 0.0f;
    float orbitC = 0.0f;
    float orbitD = 0.0f;
    
    // Output
    float voiceOutput = 0.0f;
    float masterOutput = 0.0f;
    
    // Limiter
    float gainReductionDb = 0.0f;
    bool isLimiting = false;
    
    // Noise (for mod source)
    float noiseModValue = 0.0f;
    
    // Stereo pan position (-1 to 1)
    float panPosition = 0.0f;
    
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
    
    // Modulation matrix access
    ModulationMatrix& getModMatrix() { return modMatrix; }
    
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
    Orbit orbit;
    ModulationMatrix modMatrix;
    
    juce::SmoothedValue<float> masterGain;
    float modWheelValue = 0.0f;
    
    // Parameter pointers (cached for performance)
    std::atomic<float>* osc1EnableParam = nullptr;
    std::atomic<float>* osc1WaveformParam = nullptr;
    std::atomic<float>* osc1LevelParam = nullptr;
    std::atomic<float>* osc1DetuneParam = nullptr;
    std::atomic<float>* osc1OctaveParam = nullptr;
    
    std::atomic<float>* osc2EnableParam = nullptr;
    std::atomic<float>* osc2WaveformParam = nullptr;
    std::atomic<float>* osc2LevelParam = nullptr;
    std::atomic<float>* osc2DetuneParam = nullptr;
    std::atomic<float>* osc2OctaveParam = nullptr;
    
    std::atomic<float>* subEnableParam = nullptr;
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
    
    // Advanced filter params
    std::atomic<float>* filterModeParam = nullptr;
    std::atomic<float>* formantVowelParam = nullptr;
    
    // Orbit params
    std::atomic<float>* orbitShapeParam = nullptr;
    std::atomic<float>* orbitRateParam = nullptr;
    std::atomic<float>* orbitSyncParam = nullptr;
    std::atomic<float>* orbitNumOutputsParam = nullptr;
    
    // Noise params
    std::atomic<float>* noiseTypeParam = nullptr;
    std::atomic<float>* noiseLevelParam = nullptr;
    
    // Ring Mod params
    std::atomic<float>* ringModMixParam = nullptr;
    
    // Unison params
    std::atomic<float>* unisonVoicesParam = nullptr;
    std::atomic<float>* unisonDetuneParam = nullptr;
    
    // Stereo params
    std::atomic<float>* stereoWidthParam = nullptr;
    
    // Mod matrix params (8 slots × 3)
    std::array<std::atomic<float>*, 8> modSrcParams {};
    std::array<std::atomic<float>*, 8> modDstParams {};
    std::array<std::atomic<float>*, 8> modAmtParams {};
    
    // Noise mod source generator (global, not per-voice, for S&H mod)
    NoiseGenerator noiseModSource;
    
    // Effects
    Wavefolder wavefolder;
    Distortion distortion;
    Chorus chorus;
    Delay delay;
    Reverb reverb;
    OTT ott;
    
    // DC Blocker (removes DC offset from output)
    DCBlocker dcBlockerL;
    DCBlocker dcBlockerR;
    
    // Safety limiter (protects speakers from excessive levels)
    SafetyLimiter safetyLimiter;
    
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
    
    std::atomic<float>* ottEnableParam = nullptr;
    std::atomic<float>* ottDepthParam = nullptr;
    std::atomic<float>* ottTimeParam = nullptr;
    std::atomic<float>* ottMixParam = nullptr;
    
    std::atomic<float>* wfoldEnableParam = nullptr;
    std::atomic<float>* wfoldAmountParam = nullptr;
    std::atomic<float>* wfoldMixParam = nullptr;
    
    // Stereo width: allpass decorrelation for R channel
    std::vector<float> widthDelayBuffer;
    size_t widthDelayWriteIdx = 0;
    size_t widthDelaySamples = 0;
    
    // Debug info (updated each sample for display)
    DebugInfo debugInfo;
};

} // namespace kndl
