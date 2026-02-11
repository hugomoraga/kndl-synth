#include "Voice.h"
#include <cmath>

namespace kndl {

void Voice::prepare(double newSampleRate, int blockSize)
{
    sampleRate = newSampleRate;
    
    osc1.prepare(newSampleRate);
    osc2.prepare(newSampleRate);
    subOsc.prepare(newSampleRate);
    noiseGen.prepare(newSampleRate);
    
    // Prepare unison oscillators
    for (auto& u : unisonOsc1) u.prepare(newSampleRate);
    for (auto& u : unisonOsc2) u.prepare(newSampleRate);
    
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
    noiseLevelMod = 0.0f;
    ringModMixMod = 0.0f;
    
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
    
    // === Process oscillators with unison ===
    float osc1Raw = 0.0f;
    float osc2Raw = 0.0f;
    
    if (osc1Enabled)
    {
        osc1Raw = osc1.process();
        if (unisonVoices > 1)
        {
            for (int i = 0; i < unisonVoices - 1; ++i)
                osc1Raw += unisonOsc1[static_cast<size_t>(i)].process();
            osc1Raw /= static_cast<float>(unisonVoices); // Normalize
        }
    }
    
    if (osc2Enabled)
    {
        osc2Raw = osc2.process();
        if (unisonVoices > 1)
        {
            for (int i = 0; i < unisonVoices - 1; ++i)
                osc2Raw += unisonOsc2[static_cast<size_t>(i)].process();
            osc2Raw /= static_cast<float>(unisonVoices); // Normalize
        }
    }
    
    float osc1Out = osc1Raw * juce::jlimit(0.0f, 1.0f, osc1Level + osc1LevelMod);
    float osc2Out = osc2Raw * juce::jlimit(0.0f, 1.0f, osc2Level + osc2LevelMod);
    float subOut  = subEnabled ? subOsc.process() * juce::jlimit(0.0f, 1.0f, subLevel + subLevelMod) : 0.0f;
    
    // === Noise oscillator ===
    float modulatedNoiseLevel = juce::jlimit(0.0f, 1.0f, noiseLevel + noiseLevelMod);
    float noiseOut = (modulatedNoiseLevel > 0.001f) ? noiseGen.process() * modulatedNoiseLevel : 0.0f;
    
    // === Ring Modulation: OSC1 * OSC2 ===
    float modulatedRingMix = juce::jlimit(0.0f, 1.0f, ringModMix + ringModMixMod);
    float normalMix = osc1Out + osc2Out;
    float ringMod = osc1Raw * osc2Raw; // Raw ring mod signal (no level scaling, just multiplication)
    
    // Blend between normal mix and ring mod
    float oscMix;
    if (modulatedRingMix > 0.001f)
        oscMix = normalMix * (1.0f - modulatedRingMix) + ringMod * modulatedRingMix;
    else
        oscMix = normalMix;
    
    // Add sub and noise
    float mixed = oscMix + subOut + noiseOut;
    
    // === Source normalization ===
    // Count active sources to prevent volume explosion when stacking layers
    int activeSources = 0;
    if (osc1Enabled && osc1Level > 0.01f) activeSources++;
    if (osc2Enabled && osc2Level > 0.01f) activeSources++;
    if (subEnabled && subLevel > 0.01f)   activeSources++;
    if (modulatedNoiseLevel > 0.01f)      activeSources++;
    
    // Gentle normalization: 1 source = 1.0, 2 = 0.78, 3 = 0.67, 4 = 0.58
    if (activeSources > 1)
        mixed *= 1.0f / std::sqrt(static_cast<float>(activeSources));
    
    // Soft saturation to tame peaks (tanh instead of hard clip)
    if (std::abs(mixed) > 1.0f)
        mixed = std::tanh(mixed);
    
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
    
    // Soft saturation: tanh for peaks, hard limit as safety net
    if (std::abs(output) > 0.9f)
        output = std::tanh(output);
    output = juce::jlimit(-1.5f, 1.5f, output);
    
    // Update debug info
    debugInfo.osc1Value = osc1Out;
    debugInfo.osc2Value = osc2Out;
    debugInfo.subValue = subOut;
    debugInfo.noiseValue = noiseOut;
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
    noiseGen.reset();
    for (auto& u : unisonOsc1) u.reset();
    for (auto& u : unisonOsc2) u.reset();
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
    noiseLevelMod = 0.0f;
    ringModMixMod = 0.0f;
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
    
    // === OSC1 ===
    float osc1Freq = baseFrequency * std::pow(2.0f, static_cast<float>(osc1Octave));
    osc1Freq *= std::pow(2.0f, osc1Detune / 1200.0f);
    osc1Freq *= pitchMod1;
    osc1Freq = juce::jlimit(1.0f, 20000.0f, osc1Freq);
    osc1.setFrequency(osc1Freq);
    
    // === OSC2 ===
    float osc2Freq = baseFrequency * std::pow(2.0f, static_cast<float>(osc2Octave));
    osc2Freq *= std::pow(2.0f, osc2Detune / 1200.0f);
    osc2Freq *= pitchMod2;
    osc2Freq = juce::jlimit(1.0f, 20000.0f, osc2Freq);
    osc2.setFrequency(osc2Freq);
    
    // === SUB ===
    float subFreq = baseFrequency * pitchMod1;
    subFreq = juce::jlimit(1.0f, 20000.0f, subFreq);
    subOsc.setFrequency(subFreq);
    
    // === UNISON oscillators (symmetric detune spread around center) ===
    if (unisonVoices > 1)
    {
        for (int i = 0; i < unisonVoices - 1; ++i)
        {
            // Spread positions: for N extra voices, distribute from -spread to +spread
            // Voice index 0..N-2 maps to spread positions
            int numExtra = unisonVoices - 1;
            float position;
            if (numExtra == 1)
                position = (i == 0) ? -1.0f : 1.0f; // Won't happen, just -1
            else
                position = -1.0f + 2.0f * static_cast<float>(i) / static_cast<float>(numExtra - 1);
            
            // For odd number of extras (e.g., 2 extras for 3 voices), alternate sign
            if (numExtra == 1)
                position = (i % 2 == 0) ? 1.0f : -1.0f;
            
            float unisonCents = position * unisonDetuneCents;
            float unisonMul = std::pow(2.0f, unisonCents / 1200.0f);
            
            auto idx = static_cast<size_t>(i);
            
            // OSC1 unison
            float u1Freq = osc1Freq * unisonMul;
            u1Freq = juce::jlimit(1.0f, 20000.0f, u1Freq);
            unisonOsc1[idx].setFrequency(u1Freq);
            
            // OSC2 unison
            float u2Freq = osc2Freq * unisonMul;
            u2Freq = juce::jlimit(1.0f, 20000.0f, u2Freq);
            unisonOsc2[idx].setFrequency(u2Freq);
        }
    }
}

} // namespace kndl
