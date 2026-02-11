#pragma once

#include "Effect.h"
#include "../filters/SVFFilter.h"
#include <cmath>

namespace kndl {

/**
 * OTT (Over The Top) - Compresor multibanda agresivo.
 * Divide la señal en 3 bandas (low, mid, high) y aplica compresión
 * tanto upward como downward en cada banda.
 */
class OTT : public Effect
{
public:
    void prepare(double newSampleRate, int /*samplesPerBlock*/) override
    {
        sampleRate = newSampleRate;
        
        // Filtros de crossover: 200Hz (low/mid), 4000Hz (mid/high)
        lowFilter.prepare(newSampleRate, 512);
        lowFilter.setType(FilterType::LowPass);
        lowFilter.setCutoff(200.0f);
        lowFilter.setResonance(0.5f);
        
        midLowFilter.prepare(newSampleRate, 512);
        midLowFilter.setType(FilterType::LowPass);
        midLowFilter.setCutoff(4000.0f);
        midLowFilter.setResonance(0.5f);
        
        midHighFilter.prepare(newSampleRate, 512);
        midHighFilter.setType(FilterType::HighPass);
        midHighFilter.setCutoff(200.0f);
        midHighFilter.setResonance(0.5f);
        
        highFilter.prepare(newSampleRate, 512);
        highFilter.setType(FilterType::HighPass);
        highFilter.setCutoff(4000.0f);
        highFilter.setResonance(0.5f);
        
        reset();
    }
    
    void setDepth(float depth) // 0.0 to 1.0
    {
        depthAmount = juce::jlimit(0.0f, 1.0f, depth);
    }
    
    void setTime(float timeMs) // Attack/Release time
    {
        timeConstant = juce::jlimit(0.1f, 100.0f, timeMs);
        attackCoeff = std::exp(-1.0f / (timeConstant * 0.001f * static_cast<float>(sampleRate) * 0.1f));
        releaseCoeff = std::exp(-1.0f / (timeConstant * 0.01f * static_cast<float>(sampleRate)));
    }
    
    float process(float input) override
    {
        if (!enabled)
            return input;
        
        // Split into bands
        float low = lowFilter.process(input);
        float mid = midLowFilter.process(midHighFilter.process(input));
        float high = highFilter.process(input);
        
        // Compress each band (simple envelope follower + gain reduction)
        low = compressBand(low, lowEnv);
        mid = compressBand(mid, midEnv);
        high = compressBand(high, highEnv);
        
        // Recombine
        float output = low + mid + high;
        
        // Mix
        return input * (1.0f - mix) + output * mix;
    }
    
    void reset() override
    {
        lowFilter.reset();
        midLowFilter.reset();
        midHighFilter.reset();
        highFilter.reset();
        lowEnv = 0.0f;
        midEnv = 0.0f;
        highEnv = 0.0f;
    }
    
private:
    float compressBand(float input, float& envelope)
    {
        // Envelope follower
        float absInput = std::abs(input);
        if (absInput > envelope)
            envelope = absInput + (envelope - absInput) * attackCoeff;
        else
            envelope = absInput + (envelope - absInput) * releaseCoeff;
        
        // OTT compression: both upward and downward
        float threshold = 0.3f - depthAmount * 0.25f; // 0.3 to 0.05
        float ratio = 1.0f + depthAmount * 19.0f; // 1:1 to 20:1
        
        float gain = 1.0f;
        if (envelope > threshold)
        {
            // Downward compression
            float overThreshold = envelope - threshold;
            float compressed = threshold + overThreshold / ratio;
            gain = compressed / (envelope + 1e-10f);
        }
        else
        {
            // Upward compression (boost quiet parts)
            float underThreshold = threshold - envelope;
            float boosted = threshold - underThreshold / (2.0f - depthAmount);
            gain = boosted / (envelope + 1e-10f);
            gain = juce::jlimit(0.1f, 10.0f, gain); // Limitar boost
        }
        
        return input * gain;
    }
    
    SVFFilter lowFilter;
    SVFFilter midLowFilter;
    SVFFilter midHighFilter;
    SVFFilter highFilter;
    
    float lowEnv = 0.0f;
    float midEnv = 0.0f;
    float highEnv = 0.0f;
    
    float depthAmount = 0.5f;
    float timeConstant = 10.0f;
    float attackCoeff = 0.99f;
    float releaseCoeff = 0.99f;
};

} // namespace kndl
