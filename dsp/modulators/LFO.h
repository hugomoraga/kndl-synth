#pragma once

#include <JuceHeader.h>
#include "../core/Parameters.h"
#include <cmath>

namespace kndl {

/**
 * LFO (Low Frequency Oscillator) para modulación.
 * Puede sincronizarse al tempo del DAW.
 * Output: -1 a 1
 */
class LFO
{
public:
    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        reset();
    }
    
    void setRate(float rateHz)
    {
        rate = rateHz;
        if (!syncToTempo)
            phaseIncrement = rate / static_cast<float>(sampleRate);
    }
    
    void setWaveform(Waveform waveform)
    {
        currentWaveform = waveform;
    }
    
    void setSyncEnabled(bool enabled)
    {
        syncToTempo = enabled;
    }
    
    // Llamar desde processBlock con info del host
    void setTempoSync(double bpm, double /*ppqPosition*/)
    {
        if (syncToTempo && bpm > 0)
        {
            // rate en este caso es divisiones de beat (ej: 1 = quarter note)
            double beatsPerSecond = bpm / 60.0;
            double cyclesPerSecond = beatsPerSecond / rate;
            phaseIncrement = static_cast<float>(cyclesPerSecond / sampleRate);
        }
    }
    
    float process()
    {
        float output = 0.0f;
        
        switch (currentWaveform)
        {
            case Waveform::Sine:
                output = std::sin(phase * juce::MathConstants<float>::twoPi);
                break;
                
            case Waveform::Triangle:
                output = 4.0f * std::abs(phase - 0.5f) - 1.0f;
                break;
                
            case Waveform::Saw:
                output = 2.0f * phase - 1.0f;
                break;
                
            case Waveform::Square:
                output = phase < 0.5f ? 1.0f : -1.0f;
                break;
        }
        
        phase += phaseIncrement;
        if (phase >= 1.0f)
            phase -= 1.0f;
        
        return output;
    }
    
    void reset()
    {
        phase = 0.0f;
    }
    
    float getCurrentValue() const
    {
        // Retorna el valor sin avanzar fase (para UI)
        switch (currentWaveform)
        {
            case Waveform::Sine:
                return std::sin(phase * juce::MathConstants<float>::twoPi);
            case Waveform::Triangle:
                return 4.0f * std::abs(phase - 0.5f) - 1.0f;
            case Waveform::Saw:
                return 2.0f * phase - 1.0f;
            case Waveform::Square:
                return phase < 0.5f ? 1.0f : -1.0f;
            default:
                return 0.0f;
        }
    }
    
private:
    double sampleRate = 44100.0;
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    float rate = 1.0f;
    Waveform currentWaveform = Waveform::Sine;
    bool syncToTempo = false;
};

} // namespace kndl
