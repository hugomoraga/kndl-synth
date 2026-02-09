#pragma once

#include <JuceHeader.h>

namespace kndl {

/**
 * Envelope ADSR clásica.
 * Genera valores de 0 a 1 para modular amplitud, filtro, etc.
 */
class Envelope
{
public:
    enum class State
    {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };
    
    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        reset();
        recalculateRates();
    }
    
    void setParameters(float attackSec, float decaySec, float sustainLevel, float releaseSec)
    {
        attack = juce::jmax(0.001f, attackSec);   // Mínimo 1ms
        decay = juce::jmax(0.001f, decaySec);
        sustain = juce::jlimit(0.0f, 1.0f, sustainLevel);
        release = juce::jmax(0.001f, releaseSec);
        
        recalculateRates();
    }
    
    void noteOn()
    {
        state = State::Attack;
        // No reseteamos currentValue para evitar clicks en retrigger
    }
    
    void noteOff()
    {
        if (state != State::Idle)
        {
            state = State::Release;
            releaseStartValue = currentValue;
            
            // Si el valor actual es muy bajo, ir directo a idle
            if (releaseStartValue < 0.001f)
            {
                currentValue = 0.0f;
                state = State::Idle;
            }
        }
    }
    
    float process()
    {
        switch (state)
        {
            case State::Idle:
                currentValue = 0.0f;
                break;
                
            case State::Attack:
                currentValue += attackRate;
                if (currentValue >= 1.0f)
                {
                    currentValue = 1.0f;
                    state = State::Decay;
                }
                break;
                
            case State::Decay:
                currentValue -= decayRate;
                if (currentValue <= sustain)
                {
                    currentValue = sustain;
                    state = State::Sustain;
                }
                break;
                
            case State::Sustain:
                currentValue = sustain;
                break;
                
            case State::Release:
                // Release siempre usa el rate base, independiente del valor inicial
                currentValue -= releaseRate;
                if (currentValue <= 0.0f)
                {
                    currentValue = 0.0f;
                    state = State::Idle;
                }
                break;
        }
        
        // Asegurar que siempre esté en rango válido
        currentValue = juce::jlimit(0.0f, 1.0f, currentValue);
        
        return currentValue;
    }
    
    void reset()
    {
        state = State::Idle;
        currentValue = 0.0f;
        releaseStartValue = 0.0f;
    }
    
    bool isActive() const { return state != State::Idle; }
    State getState() const { return state; }
    float getCurrentValue() const { return currentValue; }
    
private:
    void recalculateRates()
    {
        float sr = static_cast<float>(sampleRate);
        
        // Attack: tiempo para ir de 0 a 1
        attackRate = 1.0f / (attack * sr);
        
        // Decay: tiempo para ir de 1 a sustain
        float decayRange = 1.0f - sustain;
        decayRate = decayRange > 0.0f ? decayRange / (decay * sr) : 0.0f;
        
        // Release: tiempo para ir de 1 a 0 (el rate es fijo)
        releaseRate = 1.0f / (release * sr);
    }
    
    double sampleRate = 44100.0;
    State state = State::Idle;
    
    // Parámetros ADSR
    float attack = 0.01f;
    float decay = 0.1f;
    float sustain = 0.8f;
    float release = 0.3f;
    
    // Rates calculados (por sample)
    float attackRate = 0.0f;
    float decayRate = 0.0f;
    float releaseRate = 0.0f;
    
    // Estado
    float currentValue = 0.0f;
    float releaseStartValue = 0.0f;
};

} // namespace kndl
