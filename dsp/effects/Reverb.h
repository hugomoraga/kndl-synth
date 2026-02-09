#pragma once

#include "Effect.h"
#include <array>
#include <cmath>

namespace kndl {

/**
 * Reverb simple basado en Schroeder.
 * Usa comb filters + allpass filters.
 */
class Reverb : public Effect
{
public:
    void prepare(double newSampleRate, int /*samplesPerBlock*/) override
    {
        sampleRate = newSampleRate;
        
        // Inicializar comb filters con diferentes tiempos
        float baseTime = static_cast<float>(sampleRate);
        combDelays[0] = static_cast<size_t>(baseTime * 0.0297f);
        combDelays[1] = static_cast<size_t>(baseTime * 0.0371f);
        combDelays[2] = static_cast<size_t>(baseTime * 0.0411f);
        combDelays[3] = static_cast<size_t>(baseTime * 0.0437f);
        
        for (size_t i = 0; i < 4; ++i)
        {
            combBuffers[i].resize(combDelays[i], 0.0f);
            combIndices[i] = 0;
        }
        
        // Allpass filters
        allpassDelays[0] = static_cast<size_t>(baseTime * 0.005f);
        allpassDelays[1] = static_cast<size_t>(baseTime * 0.0017f);
        
        for (size_t i = 0; i < 2; ++i)
        {
            allpassBuffers[i].resize(allpassDelays[i], 0.0f);
            allpassIndices[i] = 0;
        }
        
        updateDecay();
    }
    
    void setRoomSize(float size)
    {
        roomSize = juce::jlimit(0.0f, 1.0f, size);
        updateDecay();
    }
    
    void setDamping(float damp)
    {
        damping = juce::jlimit(0.0f, 1.0f, damp);
    }
    
    float process(float input) override
    {
        if (!enabled)
            return input;
        
        float output = 0.0f;
        
        // Parallel comb filters
        for (size_t i = 0; i < 4; ++i)
        {
            if (combBuffers[i].empty()) continue;
            
            float delayed = combBuffers[i][static_cast<size_t>(combIndices[i])];
            
            // Low-pass filter in feedback (damping)
            combFilterStates[i] = delayed * (1.0f - damping) + combFilterStates[i] * damping;
            
            // Soft clip to prevent buildup
            float feedbackSignal = combFilterStates[i] * combFeedback;
            feedbackSignal = std::tanh(feedbackSignal * 0.7f) / 0.7f; // Gentle saturation
            
            // Write to buffer
            combBuffers[i][static_cast<size_t>(combIndices[i])] = input + feedbackSignal;
            
            // Prevent denormals
            if (std::abs(combFilterStates[i]) < 1e-15f)
                combFilterStates[i] = 0.0f;
            
            combIndices[i] = (combIndices[i] + 1) % combBuffers[i].size();
            
            output += delayed;
        }
        
        output *= 0.25f;  // Average of 4 combs
        
        // Series allpass filters
        for (size_t i = 0; i < 2; ++i)
        {
            if (allpassBuffers[i].empty()) continue;
            
            float delayed = allpassBuffers[i][static_cast<size_t>(allpassIndices[i])];
            float temp = -output * allpassFeedback + delayed;
            
            allpassBuffers[i][static_cast<size_t>(allpassIndices[i])] = output + delayed * allpassFeedback;
            
            allpassIndices[i] = (allpassIndices[i] + 1) % allpassBuffers[i].size();
            
            output = temp;
        }
        
        // Mix
        return input * (1.0f - mix) + output * mix;
    }
    
    void reset() override
    {
        for (size_t i = 0; i < 4; ++i)
        {
            std::fill(combBuffers[i].begin(), combBuffers[i].end(), 0.0f);
            combIndices[i] = 0;
            combFilterStates[i] = 0.0f;
        }
        
        for (size_t i = 0; i < 2; ++i)
        {
            std::fill(allpassBuffers[i].begin(), allpassBuffers[i].end(), 0.0f);
            allpassIndices[i] = 0;
        }
    }
    
private:
    void updateDecay()
    {
        combFeedback = 0.7f + roomSize * 0.28f;  // 0.7 to 0.98
    }
    
    // Comb filters
    std::array<std::vector<float>, 4> combBuffers;
    std::array<size_t, 4> combDelays = {0, 0, 0, 0};
    std::array<size_t, 4> combIndices = {0, 0, 0, 0};
    std::array<float, 4> combFilterStates = {0.0f, 0.0f, 0.0f, 0.0f};
    float combFeedback = 0.84f;
    
    // Allpass filters
    std::array<std::vector<float>, 2> allpassBuffers;
    std::array<size_t, 2> allpassDelays = {0, 0};
    std::array<size_t, 2> allpassIndices = {0, 0};
    float allpassFeedback = 0.5f;
    
    float roomSize = 0.5f;
    float damping = 0.5f;
};

} // namespace kndl
