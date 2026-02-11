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
        
        // Drive: amount 0 -> 1x gain, amount 1 -> 6x gain
        float gain = 1.0f + amount * 5.0f;
        float driven = input * gain;
        
        // Stage 1: sinusoidal fold (fundamental wavefolder)
        float folded = std::sin(driven * juce::MathConstants<float>::pi);
        
        // Stage 2: double-fold for higher amount (adds metallic complexity)
        if (amount > 0.4f)
        {
            float stage2Mix = (amount - 0.4f) / 0.6f; // 0 to 1
            float doubleFold = std::sin(folded * juce::MathConstants<float>::pi);
            folded = folded * (1.0f - stage2Mix) + doubleFold * stage2Mix;
        }
        
        // Compensar ganancia (wavefolder output bounded to [-1,1] by sin())
        // Pero mezclar con dry puede exceder, así que no amplificamos
        
        // Dry/Wet mix
        return input * (1.0f - mix) + folded * mix;
    }
    
    void reset() override
    {
        // Stateless
    }
    
private:
    float amount = 0.0f;
};

} // namespace kndl
