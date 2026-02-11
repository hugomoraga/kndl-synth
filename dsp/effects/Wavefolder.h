#pragma once

#include "Effect.h"
#include <cmath>

namespace kndl {

/**
 * Wavefolder - dobla la señal sobre sí misma creando armónicos complejos.
 * 
 * El ingrediente secreto de sintetizadores como Buchla 259 y Make Noise.
 * A diferencia de la distorsión (que clipea), el wavefolder genera
 * armónicos pares e impares de manera musical conforme se incrementa
 * el amount. El resultado son timbres vivos, metálicos y orgánicos.
 * 
 * Implementa un wavefolder sinusoidal multi-etapa:
 * - Stage 1: sin(x * pi) - fold básico
 * - Stage 2: sin(sin(x) * pi) - fold doble para más complejidad
 * - Suavizado entre stages según amount
 */
class Wavefolder : public Effect
{
public:
    void prepare(double newSampleRate, int /*samplesPerBlock*/) override
    {
        sampleRate = newSampleRate;
    }
    
    void setAmount(float amt)
    {
        amount = juce::jlimit(0.0f, 1.0f, amt);
    }
    
    float process(float input) override
    {
        if (!enabled || amount < 0.001f)
            return input;
        
        // Soft-limit the input before driving to prevent harshness
        float limited = std::tanh(input);
        
        // Drive: amount 0 -> 1x gain, amount 1 -> 4x gain (reduced from 6x)
        float gain = 1.0f + amount * 3.0f;
        float driven = limited * gain;
        
        // Stage 1: sinusoidal fold (fundamental wavefolder)
        float folded = std::sin(driven * juce::MathConstants<float>::pi);
        
        // Stage 2: double-fold for higher amount (adds metallic complexity)
        if (amount > 0.4f)
        {
            float stage2Mix = (amount - 0.4f) / 0.6f; // 0 to 1
            float doubleFold = std::sin(folded * juce::MathConstants<float>::pi);
            folded = folded * (1.0f - stage2Mix) + doubleFold * stage2Mix;
        }
        
        // Gain compensation: the dry signal might be hot, so we tanh the mix
        float result = input * (1.0f - mix) + folded * mix;
        
        // Prevent the output from exceeding the input peak level
        if (std::abs(result) > 1.0f)
            result = std::tanh(result);
        
        return result;
    }
    
    void reset() override
    {
        // Stateless
    }
    
private:
    float amount = 0.0f;
};

} // namespace kndl
