#include "Voice.h"
#include <cmath>

namespace kndl {

void Voice::prepare(double newSampleRate, int blockSize)
{
    sampleRate = newSampleRate;
    
    osc1.prepare(newSampleRate);
    osc2.prepare(newSampleRate);
    subOsc.prepare(newSampleRate);
    filter.prepare(newSampleRate, blockSize);
    formantFilter.prepare(newSampleRate, blockSize);
    combFilter.prepare(newSampleRate, blockSize);
    notchFilter.prepare(newSampleRate, blockSize);
    ampEnvelope.prepare(newSampleRate);
    filterEnvelope.prepare(newSampleRate);
}

void Voice::noteOn(int midiNote, float vel)
{
    currentNote = midiNote;
    velocity = vel;
    
    // Reset modulation offsets before computing frequency (prevents voice-steal pitch glitch)
    pitchModulation = 0.0f;
    osc2PitchModulation = 0.0f;
    filterCutoffMod = 0.0f;
    filterResoMod = 0.0f;
    osc1LevelMod = 0.0f;
    osc2LevelMod = 0.0f;
    subLevelMod = 0.0f;
    ampLevelMod = 0.0f;
    
    baseFrequency = static_cast<float>(440.0 * std::pow(2.0, (midiNote - 69) / 12.0));
    
    updateOscillatorFrequencies();
    
    ampEnvelope.noteOn();
    filterEnvelope.noteOn();
    
    isActive = true;
}

void Voice::noteOff()
{
    ampEnvelope.noteOff();
    filterEnvelope.noteOff();
}

float Voice::process()
{
    if (!isActive)
        return 0.0f;
    
    float osc1Out = osc1Enabled ? osc1.process() * juce::jlimit(0.0f, 1.0f, osc1Level + osc1LevelMod) : 0.0f;
    float osc2Out = osc2Enabled ? osc2.process() * juce::jlimit(0.0f, 1.0f, osc2Level + osc2LevelMod) : 0.0f;
    float subOut  = subEnabled  ? subOsc.process() * juce::jlimit(0.0f, 1.0f, subLevel + subLevelMod) : 0.0f;
    
    float mixed = osc1Out + osc2Out + subOut;
    
    // Protección contra valores inválidos del oscilador
    if (!std::isfinite(mixed))
        mixed = 0.0f;
    
    float ampEnvValue = ampEnvelope.process();
    float filterEnvValue = filterEnvelope.process();
    
    float modulatedCutoff = baseCutoff + (filterEnvValue * filterEnvAmount * 10000.0f)
                          + filterCutoffMod * 10000.0f;
    modulatedCutoff = juce::jlimit(20.0f, 20000.0f, modulatedCutoff);
    
    float modulatedReso = juce::jlimit(0.0f, 1.0f, filterResonance + filterResoMod);
    
    float filtered;
    switch (filterMode)
    {
        case FilterMode::Formant:
            formantFilter.setCutoff(modulatedCutoff);
            formantFilter.setResonance(modulatedReso);
            filtered = formantFilter.process(mixed);
            break;
            
        case FilterMode::Comb:
            combFilter.setCutoff(modulatedCutoff);
            combFilter.setResonance(modulatedReso);
            filtered = combFilter.process(mixed);
            break;
            
        case FilterMode::Notch:
            notchFilter.setCutoff(modulatedCutoff);
            notchFilter.setResonance(modulatedReso);
            filtered = notchFilter.process(mixed);
            break;
            
        case FilterMode::SVF:
        default:
            filter.setCutoff(modulatedCutoff);
            filter.setResonance(modulatedReso);
            filtered = filter.process(mixed);
            break;
    }
    
    // Protección contra valores inválidos del filtro
    if (!std::isfinite(filtered))
    {
        filtered = 0.0f;
        filter.reset();
        formantFilter.reset();
        combFilter.reset();
        notchFilter.reset();
    }
    
    float ampMod = juce::jlimit(0.0f, 2.0f, 1.0f + ampLevelMod);
    float output = filtered * ampEnvValue * velocity * ampMod;
    
    // Limitar output para evitar explosiones
    output = juce::jlimit(-2.0f, 2.0f, output);
    
    // Update debug info
    debugInfo.osc1Value = osc1Out;
    debugInfo.osc2Value = osc2Out;
    debugInfo.subValue = subOut;
    debugInfo.mixedOsc = mixed;
    debugInfo.filterInput = mixed;
    debugInfo.filterOutput = filtered;
    debugInfo.filterCutoff = modulatedCutoff;
    debugInfo.ampEnvValue = ampEnvValue;
    debugInfo.filterEnvValue = filterEnvValue;
    debugInfo.output = output;
    
    if (!ampEnvelope.isActive())
    {
        isActive = false;
        reset();
    }
    
    return output;
}

void Voice::reset()
{
    osc1.reset();
    osc2.reset();
    subOsc.reset();
    filter.reset();
    formantFilter.reset();
    combFilter.reset();
    notchFilter.reset();
    ampEnvelope.reset();
    filterEnvelope.reset();
    isActive = false;
    currentNote = -1;
    
    // Reset modulation offsets to prevent stale values on voice reuse
    pitchModulation = 0.0f;
    osc2PitchModulation = 0.0f;
    filterCutoffMod = 0.0f;
    filterResoMod = 0.0f;
    osc1LevelMod = 0.0f;
    osc2LevelMod = 0.0f;
    subLevelMod = 0.0f;
    ampLevelMod = 0.0f;
}

void Voice::setOsc1Detune(float cents)
{
    osc1Detune = cents;
    updateOscillatorFrequencies();
}

void Voice::setOsc1Octave(int oct)
{
    osc1Octave = oct;
    updateOscillatorFrequencies();
}

void Voice::setOsc2Detune(float cents)
{
    osc2Detune = cents;
    updateOscillatorFrequencies();
}

void Voice::setOsc2Octave(int oct)
{
    osc2Octave = oct;
    updateOscillatorFrequencies();
}

void Voice::setSubOctave(int oct)
{
    subOsc.setOctave(oct);
    updateOscillatorFrequencies();
}

void Voice::applyPitchMod(float semitones)
{
    pitchModulation = semitones;
    updateOscillatorFrequencies();
}

void Voice::applyOsc2PitchMod(float semitones)
{
    osc2PitchModulation = semitones;
    updateOscillatorFrequencies();
}

void Voice::updateOscillatorFrequencies()
{
    if (baseFrequency <= 0.0f || !std::isfinite(baseFrequency))
        return;
    
    // Clamp pitch modulation to sane range to prevent frequency overflow
    float clampedPitchMod = juce::jlimit(-48.0f, 48.0f, pitchModulation);
    float clampedOsc2PitchMod = juce::jlimit(-48.0f, 48.0f, osc2PitchModulation);
    
    float pitchMod1 = std::pow(2.0f, clampedPitchMod / 12.0f);
    float pitchMod2 = std::pow(2.0f, (clampedPitchMod + clampedOsc2PitchMod) / 12.0f);
    
    float osc1Freq = baseFrequency * std::pow(2.0f, static_cast<float>(osc1Octave));
    osc1Freq *= std::pow(2.0f, osc1Detune / 1200.0f);
    osc1Freq *= pitchMod1;
    osc1Freq = juce::jlimit(1.0f, 20000.0f, osc1Freq);
    osc1.setFrequency(osc1Freq);
    
    float osc2Freq = baseFrequency * std::pow(2.0f, static_cast<float>(osc2Octave));
    osc2Freq *= std::pow(2.0f, osc2Detune / 1200.0f);
    osc2Freq *= pitchMod2;
    osc2Freq = juce::jlimit(1.0f, 20000.0f, osc2Freq);
    osc2.setFrequency(osc2Freq);
    
    float subFreq = baseFrequency * pitchMod1;
    subFreq = juce::jlimit(1.0f, 20000.0f, subFreq);
    subOsc.setFrequency(subFreq);
}

} // namespace kndl
