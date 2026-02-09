#pragma once

#include "Oscillator.h"
#include <cmath>

namespace kndl {

/**
 * Oscilador básico con formas de onda clásicas.
 * Usa PolyBLEP para reducir aliasing en saw y square.
 */
class BasicOscillator : public Oscillator
{
public:
    void prepare(double newSampleRate) override
    {
        sampleRate = newSampleRate;
        phase = 0.0;
        phaseIncrement = 0.0;
    }
    
    void setFrequency(float freq) override
    {
        frequency = freq;
        phaseIncrement = frequency / static_cast<float>(sampleRate);
    }
    
    float process() override
    {
        float output = 0.0f;
        
        switch (currentWaveform)
        {
            case Waveform::Sine:
                output = processSine();
                break;
            case Waveform::Triangle:
                output = processTriangle();
                break;
            case Waveform::Saw:
                output = processSaw();
                break;
            case Waveform::Square:
                output = processSquare();
                break;
        }
        
        // Avanzar fase
        phase += phaseIncrement;
        if (phase >= 1.0)
            phase -= 1.0;
        
        return output;
    }
    
    void reset() override
    {
        phase = 0.0;
    }
    
private:
    double phase = 0.0;
    double phaseIncrement = 0.0;
    
    // PolyBLEP para anti-aliasing
    float polyBlep(double t) const
    {
        double dt = phaseIncrement;
        
        if (t < dt)
        {
            t /= dt;
            return static_cast<float>(t + t - t * t - 1.0);
        }
        else if (t > 1.0 - dt)
        {
            t = (t - 1.0) / dt;
            return static_cast<float>(t * t + t + t + 1.0);
        }
        return 0.0f;
    }
    
    float processSine() const
    {
        return std::sin(static_cast<float>(phase * juce::MathConstants<double>::twoPi));
    }
    
    float processTriangle() const
    {
        // Triangle from integrated square
        float value = static_cast<float>(phase) * 4.0f;
        if (phase < 0.25)
            value = static_cast<float>(phase) * 4.0f;
        else if (phase < 0.75)
            value = 2.0f - static_cast<float>(phase) * 4.0f;
        else
            value = static_cast<float>(phase) * 4.0f - 4.0f;
        return value;
    }
    
    float processSaw()
    {
        // Saw: rampa de -1 a 1
        float value = static_cast<float>(2.0 * phase - 1.0);
        value -= polyBlep(phase);
        return value;
    }
    
    float processSquare()
    {
        // Square con duty cycle 50%
        float value = phase < 0.5 ? 1.0f : -1.0f;
        value += polyBlep(phase);
        value -= polyBlep(std::fmod(phase + 0.5, 1.0));
        return value;
    }
};

} // namespace kndl
