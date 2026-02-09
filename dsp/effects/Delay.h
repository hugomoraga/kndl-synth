#pragma once

#include "Effect.h"
#include <vector>

namespace kndl {

/**
 * Delay simple con feedback.
 */
class Delay : public Effect
{
public:
    void prepare(double newSampleRate, int /*samplesPerBlock*/) override
    {
        sampleRate = newSampleRate;
        
        // Buffer para hasta 2 segundos de delay
        int maxSamples = static_cast<int>(sampleRate * 2.0);
        buffer.resize(static_cast<size_t>(maxSamples), 0.0f);
        writeIndex = 0;
        
        updateDelayTime();
    }
    
    void setDelayTime(float timeMs)
    {
        delayTimeMs = juce::jlimit(1.0f, 2000.0f, timeMs);
        updateDelayTime();
    }
    
    void setFeedback(float fb)
    {
        feedback = juce::jlimit(0.0f, 0.95f, fb);
    }
    
    float process(float input) override
    {
        if (!enabled || buffer.empty())
            return input;
        
        // Leer del buffer con interpolación
        float readPos = static_cast<float>(writeIndex) - delaySamples;
        if (readPos < 0)
            readPos += static_cast<float>(buffer.size());
        
        int readIndex1 = static_cast<int>(readPos);
        int readIndex2 = (readIndex1 + 1) % static_cast<int>(buffer.size());
        float frac = readPos - static_cast<float>(readIndex1);
        
        float delayed = buffer[static_cast<size_t>(readIndex1)] * (1.0f - frac) + 
                        buffer[static_cast<size_t>(readIndex2)] * frac;
        
        // Soft clip the feedback to prevent runaway
        float feedbackSignal = delayed * feedback;
        feedbackSignal = std::tanh(feedbackSignal);
        
        // Escribir al buffer (input + feedback)
        buffer[static_cast<size_t>(writeIndex)] = input + feedbackSignal;
        
        // Avanzar write index
        writeIndex = (writeIndex + 1) % static_cast<int>(buffer.size());
        
        // Mix dry/wet
        return input * (1.0f - mix) + delayed * mix;
    }
    
    void reset() override
    {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writeIndex = 0;
    }
    
private:
    void updateDelayTime()
    {
        delaySamples = (delayTimeMs / 1000.0f) * static_cast<float>(sampleRate);
        delaySamples = juce::jlimit(1.0f, static_cast<float>(buffer.size() - 1), delaySamples);
    }
    
    std::vector<float> buffer;
    int writeIndex = 0;
    float delaySamples = 0.0f;
    float delayTimeMs = 250.0f;
    float feedback = 0.3f;
};

} // namespace kndl
