#pragma once

#include "Effect.h"
#include <vector>
#include <cmath>

namespace kndl {

/**
 * Chorus simple con LFO modulando delay time.
 */
class Chorus : public Effect
{
public:
    void prepare(double newSampleRate, int /*samplesPerBlock*/) override
    {
        sampleRate = newSampleRate;
        
        // Buffer para delay modulado (hasta 50ms)
        int maxSamples = static_cast<int>(sampleRate * 0.05);
        buffer.resize(static_cast<size_t>(maxSamples), 0.0f);
        writeIndex = 0;
        lfoPhase = 0.0;
    }
    
    void setRate(float rateHz)
    {
        rate = juce::jlimit(0.1f, 5.0f, rateHz);
    }
    
    void setDepth(float depthAmount)
    {
        depth = juce::jlimit(0.0f, 1.0f, depthAmount);
    }
    
    float process(float input) override
    {
        if (!enabled || buffer.empty())
            return input;
        
        // LFO para modular el delay
        float lfo = std::sin(static_cast<float>(lfoPhase * juce::MathConstants<double>::twoPi));
        lfoPhase += rate / sampleRate;
        if (lfoPhase >= 1.0)
            lfoPhase -= 1.0;
        
        // Delay time modulado (5ms base + depth modulation)
        float baseDelay = 0.005f * static_cast<float>(sampleRate);  // 5ms
        float modAmount = depth * 0.003f * static_cast<float>(sampleRate);  // hasta 3ms de modulación
        float delaySamples = baseDelay + lfo * modAmount;
        delaySamples = juce::jlimit(1.0f, static_cast<float>(buffer.size() - 1), delaySamples);
        
        // Leer con interpolación
        float readPos = static_cast<float>(writeIndex) - delaySamples;
        if (readPos < 0)
            readPos += static_cast<float>(buffer.size());
        
        int readIndex1 = static_cast<int>(readPos);
        int readIndex2 = (readIndex1 + 1) % static_cast<int>(buffer.size());
        float frac = readPos - static_cast<float>(readIndex1);
        
        float delayed = buffer[static_cast<size_t>(readIndex1)] * (1.0f - frac) + 
                        buffer[static_cast<size_t>(readIndex2)] * frac;
        
        // Escribir al buffer
        buffer[static_cast<size_t>(writeIndex)] = input;
        writeIndex = (writeIndex + 1) % static_cast<int>(buffer.size());
        
        // Mix
        return input * (1.0f - mix) + delayed * mix;
    }
    
    void reset() override
    {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writeIndex = 0;
        lfoPhase = 0.0;
    }
    
private:
    std::vector<float> buffer;
    int writeIndex = 0;
    double lfoPhase = 0.0;
    float rate = 1.0f;
    float depth = 0.5f;
};

} // namespace kndl
