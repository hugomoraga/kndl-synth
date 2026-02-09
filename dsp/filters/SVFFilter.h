#pragma once

#include "Filter.h"
#include <cmath>

namespace kndl {

/**
 * State Variable Filter (SVF).
 * Filtro versátil que puede hacer LP, HP, BP simultáneamente.
 * Basado en el algoritmo de Andrew Simper (Cytomic).
 */
class SVFFilter : public Filter
{
public:
    void prepare(double newSampleRate, int /*samplesPerBlock*/) override
    {
        sampleRate = newSampleRate;
        reset();
        updateCoefficients();
    }
    
    void setCutoff(float frequency) override
    {
        cutoff = juce::jlimit(20.0f, static_cast<float>(sampleRate * 0.45), frequency);
        updateCoefficients();
    }
    
    void setResonance(float res) override
    {
        resonance = juce::jlimit(0.0f, 0.99f, res); // Limitar para evitar inestabilidad
        updateCoefficients();
    }
    
    void setType(FilterType type) override
    {
        filterType = type;
    }
    
    void setDrive(float driveAmount)
    {
        drive = 1.0f + driveAmount * 3.0f;  // 1x a 4x
    }
    
    float process(float input) override
    {
        // Protección contra NaN/Inf en input
        if (!std::isfinite(input))
            input = 0.0f;
        
        // Aplicar drive (saturación suave)
        if (drive > 1.0f)
        {
            input = std::tanh(input * drive) / drive;
        }
        
        // SVF algorithm (Cytomic/Andrew Simper)
        // https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf
        float v3 = input - ic2eq;
        float v1 = a1 * ic1eq + a2 * v3;
        float v2 = ic2eq + a2 * ic1eq + a3 * v3;
        
        ic1eq = 2.0f * v1 - ic1eq;
        ic2eq = 2.0f * v2 - ic2eq;
        
        // Protección contra estados inválidos
        if (!std::isfinite(ic1eq)) ic1eq = 0.0f;
        if (!std::isfinite(ic2eq)) ic2eq = 0.0f;
        
        // Seleccionar output según tipo
        float output = 0.0f;
        switch (filterType)
        {
            case FilterType::LowPass:
                output = v2;
                break;
            case FilterType::HighPass:
                output = input - k * v1 - v2;
                break;
            case FilterType::BandPass:
                output = v1;
                break;
        }
        
        // Protección final contra NaN
        if (!std::isfinite(output))
            output = 0.0f;
        
        return output;
    }
    
    void reset() override
    {
        ic1eq = 0.0f;
        ic2eq = 0.0f;
    }
    
private:
    void updateCoefficients()
    {
        // Prewarp cutoff frequency
        float w = std::tan(juce::MathConstants<float>::pi * cutoff / static_cast<float>(sampleRate));
        
        // Q from resonance (0-1 -> 0.5-10)
        float Q = 0.5f + (1.0f - resonance) * 9.5f;
        
        g = w;
        k = 1.0f / Q;
        
        // Precompute coefficients for optimized processing
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    
    // Filter state
    float ic1eq = 0.0f;
    float ic2eq = 0.0f;
    
    // Coefficients
    float g = 0.0f;
    float k = 2.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
    float a3 = 0.0f;
    
    // Drive
    float drive = 1.0f;
};

} // namespace kndl
