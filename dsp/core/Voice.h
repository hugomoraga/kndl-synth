#pragma once

#include <JuceHeader.h>
#include "../oscillators/BasicOscillator.h"
#include "../oscillators/SubOscillator.h"
#include "../oscillators/NoiseGenerator.h"
#include "../filters/SVFFilter.h"
#include "../filters/AdvancedFilters.h"
#include "../modulators/Envelope.h"
#include <array>

namespace kndl {

/**
 * Valores de debug de una voz
 */
struct VoiceDebugInfo
{
    float osc1Value = 0.0f;
    float osc2Value = 0.0f;
    float subValue = 0.0f;
    float noiseValue = 0.0f;
    float mixedOsc = 0.0f;
    float filterInput = 0.0f;
    float filterOutput = 0.0f;
    float filterCutoff = 0.0f;
    float ampEnvValue = 0.0f;
    float filterEnvValue = 0.0f;
    float output = 0.0f;
};

/**
 * Una voz polifónica del sintetizador.
 * Contiene todos los componentes necesarios para generar una nota.
 * 
 * Incluye: 2 osciladores + sub + noise, ring mod, unison (hasta 5 voces),
 * filtros avanzados, 2 envelopes, y modulación completa.
 */
class Voice
{
public:
    static constexpr int MAX_UNISON = 5;
    
    void prepare(double newSampleRate, int blockSize);
    void noteOn(int midiNote, float vel);
    void noteOff();
    float process();
    void reset();
    
    bool getIsActive() const { return isActive; }
    int getCurrentNote() const { return currentNote; }
    float getVelocity() const { return velocity; }
    
    // Debug info
    const VoiceDebugInfo& getDebugInfo() const { return debugInfo; }
    
    // === Setters para parámetros ===
    void setOsc1Enable(bool enabled) { osc1Enabled = enabled; }
    void setOsc1Waveform(Waveform wf) { osc1.setWaveform(wf); for (auto& u : unisonOsc1) u.setWaveform(wf); }
    void setOsc1Level(float level) { osc1Level = level; }
    void setOsc1Detune(float cents);
    void setOsc1Octave(int oct);
    
    void setOsc2Enable(bool enabled) { osc2Enabled = enabled; }
    void setOsc2Waveform(Waveform wf) { osc2.setWaveform(wf); for (auto& u : unisonOsc2) u.setWaveform(wf); }
    void setOsc2Level(float level) { osc2Level = level; }
    void setOsc2Detune(float cents);
    void setOsc2Octave(int oct);
    
    void setSubEnable(bool enabled) { subEnabled = enabled; }
    void setSubLevel(float level) { subLevel = level; }
    void setSubOctave(int oct);
    
    // Noise
    void setNoiseType(NoiseType type) { noiseGen.setType(type); }
    void setNoiseLevel(float level) { noiseLevel = level; }
    void setNoiseLevelMod(float mod) { noiseLevelMod = mod; }
    
    // Ring Modulation
    void setRingModMix(float mix) { ringModMix = mix; }
    void setRingModMixMod(float mod) { ringModMixMod = mod; }
    
    // Unison
    void setUnisonVoices(int voices) { unisonVoices = juce::jlimit(1, MAX_UNISON, voices); }
    void setUnisonDetune(float cents) { unisonDetuneCents = cents; updateOscillatorFrequencies(); }
    
    void setFilterCutoff(float freq) { baseCutoff = freq; }
    void setFilterResonance(float res) { filterResonance = res; filter.setResonance(res); }
    void setFilterType(FilterType type) { filter.setType(type); }
    void setFilterDrive(float drive) { filter.setDrive(drive); }
    void setFilterEnvAmount(float amount) { filterEnvAmount = amount; }
    void setFilterMode(FilterMode mode) { filterMode = mode; }
    void setFormantVowel(int vowel) { formantFilter.setFormantVowel(vowel); }
    
    void setAmpEnvelope(float a, float d, float s, float r) { ampEnvelope.setParameters(a, d, s, r); }
    void setFilterEnvelope(float a, float d, float s, float r) { filterEnvelope.setParameters(a, d, s, r); }
    
    void applyPitchMod(float semitones);
    void applyOsc2PitchMod(float semitones);
    
    // Modulation offsets (set per-sample from ModulationMatrix)
    void setFilterCutoffMod(float mod)  { filterCutoffMod = mod; }
    void setFilterResoMod(float mod)    { filterResoMod = mod; }
    void setOsc1LevelMod(float mod)     { osc1LevelMod = mod; }
    void setOsc2LevelMod(float mod)     { osc2LevelMod = mod; }
    void setSubLevelMod(float mod)      { subLevelMod = mod; }
    void setAmpLevelMod(float mod)      { ampLevelMod = mod; }
    
private:
    void updateOscillatorFrequencies();
    
    double sampleRate = 44100.0;
    
    // Osciladores principales
    BasicOscillator osc1;
    BasicOscillator osc2;
    SubOscillator subOsc;
    NoiseGenerator noiseGen;
    
    // Unison extra oscillators (MAX_UNISON - 1 extras per oscillator)
    std::array<BasicOscillator, MAX_UNISON - 1> unisonOsc1;
    std::array<BasicOscillator, MAX_UNISON - 1> unisonOsc2;
    
    // Filtros
    SVFFilter filter;
    FormantFilter formantFilter;
    CombFilter combFilter;
    NotchFilter notchFilter;
    FilterMode filterMode = FilterMode::SVF;
    float filterResonance = 0.0f;
    
    // Envelopes
    Envelope ampEnvelope;
    Envelope filterEnvelope;
    
    // Estado de la voz
    bool isActive = false;
    int currentNote = -1;
    float velocity = 1.0f;
    float baseFrequency = 440.0f;
    
    // Parámetros de osciladores
    bool osc1Enabled = true;
    float osc1Level = 0.8f;
    float osc1Detune = 0.0f;
    int osc1Octave = 0;
    
    bool osc2Enabled = false;
    float osc2Level = 0.0f;
    float osc2Detune = 0.0f;
    int osc2Octave = 0;
    
    bool subEnabled = false;
    float subLevel = 0.0f;
    
    // Noise
    float noiseLevel = 0.0f;
    float noiseLevelMod = 0.0f;
    
    // Ring Modulation
    float ringModMix = 0.0f;
    float ringModMixMod = 0.0f;
    
    // Unison
    int unisonVoices = 1;
    float unisonDetuneCents = 15.0f;
    
    // Parámetros de filtro
    float baseCutoff = 8000.0f;
    float filterEnvAmount = 0.0f;
    
    // Modulación
    float pitchModulation = 0.0f;
    float osc2PitchModulation = 0.0f;
    float filterCutoffMod = 0.0f;
    float filterResoMod = 0.0f;
    float osc1LevelMod = 0.0f;
    float osc2LevelMod = 0.0f;
    float subLevelMod = 0.0f;
    float ampLevelMod = 0.0f;
    
    // Debug info
    VoiceDebugInfo debugInfo;
};

} // namespace kndl
