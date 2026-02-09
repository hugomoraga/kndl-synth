#pragma once

#include <JuceHeader.h>
#include "Voice.h"
#include <array>

namespace kndl {

/**
 * Maneja la polifonía del sintetizador.
 * Asigna voces a notas MIDI y maneja voice stealing.
 */
class VoiceManager
{
public:
    static constexpr int MaxVoices = 16;
    
    void prepare(double sampleRate, int samplesPerBlock)
    {
        for (auto& voice : voices)
        {
            voice.prepare(sampleRate, samplesPerBlock);
        }
    }
    
    void noteOn(int midiNote, float velocity)
    {
        // Buscar si la nota ya está sonando (retrigger)
        for (auto& voice : voices)
        {
            if (voice.getIsActive() && voice.getCurrentNote() == midiNote)
            {
                voice.noteOn(midiNote, velocity);
                return;
            }
        }
        
        // Buscar voz libre
        for (auto& voice : voices)
        {
            if (!voice.getIsActive())
            {
                voice.noteOn(midiNote, velocity);
                return;
            }
        }
        
        // Voice stealing: robar la voz más antigua
        // Por ahora, simplemente usar la primera voz
        voices[0].noteOn(midiNote, velocity);
    }
    
    void noteOff(int midiNote)
    {
        for (auto& voice : voices)
        {
            if (voice.getIsActive() && voice.getCurrentNote() == midiNote)
            {
                voice.noteOff();
            }
        }
    }
    
    void allNotesOff()
    {
        for (auto& voice : voices)
        {
            voice.noteOff();
        }
    }
    
    float process()
    {
        float output = 0.0f;
        bool firstActive = true;
        
        for (auto& voice : voices)
        {
            if (voice.getIsActive())
            {
                output += voice.process();
                
                // Capturar debug de la primera voz activa
                if (firstActive)
                {
                    lastActiveVoiceDebug = voice.getDebugInfo();
                    firstActive = false;
                }
            }
        }
        
        return output;
    }
    
    int getActiveVoiceCount() const
    {
        int count = 0;
        for (const auto& voice : voices)
        {
            if (voice.getIsActive())
                count++;
        }
        return count;
    }
    
    const VoiceDebugInfo& getDebugInfo() const { return lastActiveVoiceDebug; }
    
    // === Aplicar parámetros a todas las voces ===
    
    void setOsc1Waveform(Waveform wf) { for (auto& v : voices) v.setOsc1Waveform(wf); }
    void setOsc1Level(float level) { for (auto& v : voices) v.setOsc1Level(level); }
    void setOsc1Detune(float cents) { for (auto& v : voices) v.setOsc1Detune(cents); }
    void setOsc1Octave(int oct) { for (auto& v : voices) v.setOsc1Octave(oct); }
    
    void setOsc2Waveform(Waveform wf) { for (auto& v : voices) v.setOsc2Waveform(wf); }
    void setOsc2Level(float level) { for (auto& v : voices) v.setOsc2Level(level); }
    void setOsc2Detune(float cents) { for (auto& v : voices) v.setOsc2Detune(cents); }
    void setOsc2Octave(int oct) { for (auto& v : voices) v.setOsc2Octave(oct); }
    
    void setSubLevel(float level) { for (auto& v : voices) v.setSubLevel(level); }
    void setSubOctave(int oct) { for (auto& v : voices) v.setSubOctave(oct); }
    
    void setFilterCutoff(float freq) { for (auto& v : voices) v.setFilterCutoff(freq); }
    void setFilterResonance(float res) { for (auto& v : voices) v.setFilterResonance(res); }
    void setFilterType(FilterType type) { for (auto& v : voices) v.setFilterType(type); }
    void setFilterDrive(float drive) { for (auto& v : voices) v.setFilterDrive(drive); }
    void setFilterEnvAmount(float amount) { for (auto& v : voices) v.setFilterEnvAmount(amount); }
    
    void setAmpEnvelope(float a, float d, float s, float r) { for (auto& v : voices) v.setAmpEnvelope(a, d, s, r); }
    void setFilterEnvelope(float a, float d, float s, float r) { for (auto& v : voices) v.setFilterEnvelope(a, d, s, r); }
    
    void applyPitchMod(float semitones) { for (auto& v : voices) v.applyPitchMod(semitones); }
    
private:
    std::array<Voice, MaxVoices> voices;
    VoiceDebugInfo lastActiveVoiceDebug;
};

} // namespace kndl
