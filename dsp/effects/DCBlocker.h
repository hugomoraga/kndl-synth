#pragma once

#include <JuceHeader.h>

namespace kndl {

/**
 * DC Blocker - Elimina componente DC de la señal.
 * Usa un filtro high-pass de primer orden muy bajo (~5Hz).
 */
class DCBlocker
{
public:
    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        
        // Coeficiente para ~5Hz highpass
        // R = 1 - (2 * pi * fc / sr)
        coefficient = 1.0f - (2.0f * juce::MathConstants<float>::pi * 5.0f / static_cast<float>(sampleRate));
        coefficient = juce::jlimit(0.9f, 0.9999f, coefficient);
        
        reset();
    }
    
    float process(float input)
    {
        // y[n] = x[n] - x[n-1] + R * y[n-1]
        float output = input - previousInput + coefficient * previousOutput;
        
        previousInput = input;
        previousOutput = output;
        
        // Protección contra denormals
        if (std::abs(previousOutput) < 1e-15f)
            previousOutput = 0.0f;
        
        return output;
    }
    
    void reset()
    {
        previousInput = 0.0f;
        previousOutput = 0.0f;
    }
    
private:
    double sampleRate = 44100.0;
    float coefficient = 0.995f;
    float previousInput = 0.0f;
    float previousOutput = 0.0f;
};

} // namespace kndl
