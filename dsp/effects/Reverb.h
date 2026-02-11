#pragma once

#include "Effect.h"
#include <array>
#include <vector>
#include <cmath>

namespace kndl {

/**
 * Reverb basado en Freeverb (Jezar's algorithm).
 * 
 * Arquitectura:
 * - 8 comb filters en paralelo (con damping LP en feedback)
 * - 4 allpass filters en serie (difusión)
 * - Pre-delay configurable
 * - Tiempos basados en números mutuamente primos para evitar resonancias
 * 
 * Mejoras sobre Schroeder básico:
 * - 8 combs (vs 4) = mayor densidad de reflexiones, menos metalicidad
 * - 4 allpass (vs 2) = mejor difusión, textura más suave
 * - Tiempos de delay optimizados (Freeverb research)
 * - Saturación suave correcta (sin amplificación accidental)
 */
class Reverb : public Effect
{
public:
    void prepare(double newSampleRate, int /*samplesPerBlock*/) override
    {
        sampleRate = newSampleRate;
        
        // Freeverb delay times (en samples a 44100Hz, escalados al sampleRate actual)
        // Estos valores están optimizados para sonar bien (primos entre sí)
        const float scaleRatio = static_cast<float>(sampleRate) / 44100.0f;
        
        // 8 Comb filters - tiempos del Freeverb original
        static constexpr int combTunings[NUM_COMBS] = {
            1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617
        };
        
        for (size_t i = 0; i < NUM_COMBS; ++i)
        {
            size_t size = static_cast<size_t>(static_cast<float>(combTunings[i]) * scaleRatio);
            if (size < 1) size = 1;
            combBuffers[i].resize(size, 0.0f);
            combIndices[i] = 0;
            combFilterStates[i] = 0.0f;
        }
        
        // 4 Allpass filters - tiempos del Freeverb original
        static constexpr int allpassTunings[NUM_ALLPASS] = {
            556, 441, 341, 225
        };
        
        for (size_t i = 0; i < NUM_ALLPASS; ++i)
        {
            size_t size = static_cast<size_t>(static_cast<float>(allpassTunings[i]) * scaleRatio);
            if (size < 1) size = 1;
            allpassBuffers[i].resize(size, 0.0f);
            allpassIndices[i] = 0;
        }
        
        // Pre-delay buffer (hasta 100ms)
        size_t preDelayMax = static_cast<size_t>(sampleRate * 0.1);
        preDelayBuffer.resize(preDelayMax, 0.0f);
        preDelayWriteIdx = 0;
        
        // Recalculate pre-delay samples for new sample rate
        setPreDelay(preDelayMs);
        
        updateParameters();
    }
    
    void setRoomSize(float size)
    {
        roomSize = juce::jlimit(0.0f, 1.0f, size);
        updateParameters();
    }
    
    void setDamping(float damp)
    {
        damping = juce::jlimit(0.0f, 1.0f, damp);
        updateParameters();
    }
    
    void setPreDelay(float ms)
    {
        preDelayMs = juce::jlimit(0.0f, 100.0f, ms);
        preDelaySamples = static_cast<size_t>((preDelayMs / 1000.0f) * static_cast<float>(sampleRate));
        if (!preDelayBuffer.empty())
            preDelaySamples = std::min(preDelaySamples, preDelayBuffer.size() - 1);
    }
    
    float process(float input) override
    {
        if (!enabled)
            return input;
        
        // Pre-delay
        float preDelayed = input;
        if (!preDelayBuffer.empty() && preDelaySamples > 0)
        {
            size_t readIdx = (preDelayWriteIdx + preDelayBuffer.size() - preDelaySamples) 
                             % preDelayBuffer.size();
            preDelayed = preDelayBuffer[readIdx];
            preDelayBuffer[preDelayWriteIdx] = input;
            preDelayWriteIdx = (preDelayWriteIdx + 1) % preDelayBuffer.size();
        }
        
        // === 8 Comb filters en paralelo ===
        float combOut = 0.0f;
        
        for (size_t i = 0; i < NUM_COMBS; ++i)
        {
            if (combBuffers[i].empty()) continue;
            
            // Leer del buffer
            float delayed = combBuffers[i][combIndices[i]];
            
            // Filtro LP en feedback (damping) - cada repetición se oscurece
            // damp1 = cuánto del nuevo sample pasa, damp2 = cuánto del estado anterior se mantiene
            combFilterStates[i] = delayed * damp1 + combFilterStates[i] * damp2;
            
            // Feedback con room size
            float fbSignal = combFilterStates[i] * combFeedback;
            
            // Saturación suave - solo actúa en señales grandes
            // Para |x| < 1.0: transparente; para |x| > 1.0: comprime suavemente
            if (std::abs(fbSignal) > 1.0f)
                fbSignal = std::tanh(fbSignal);
            
            // Escribir al buffer
            combBuffers[i][combIndices[i]] = preDelayed + fbSignal;
            
            // Anti-denormal
            if (std::abs(combFilterStates[i]) < 1e-15f)
                combFilterStates[i] = 0.0f;
            
            combIndices[i] = (combIndices[i] + 1) % combBuffers[i].size();
            
            combOut += delayed;
        }
        
        // Promedio de los 8 combs (normalización)
        combOut *= (1.0f / static_cast<float>(NUM_COMBS));
        
        // === 4 Allpass filters en serie (difusión) ===
        float output = combOut;
        
        for (size_t i = 0; i < NUM_ALLPASS; ++i)
        {
            if (allpassBuffers[i].empty()) continue;
            
            float delayed = allpassBuffers[i][allpassIndices[i]];
            
            // Allpass: y = -g*x + delayed; buffer = x + g*delayed
            float temp = -output * allpassFeedback + delayed;
            allpassBuffers[i][allpassIndices[i]] = output + delayed * allpassFeedback;
            
            allpassIndices[i] = (allpassIndices[i] + 1) % allpassBuffers[i].size();
            
            output = temp;
        }
        
        // NaN/Inf protection
        if (!std::isfinite(output))
        {
            output = 0.0f;
            reset();
        }
        
        // Mix dry/wet
        return input * (1.0f - mix) + output * mix;
    }
    
    void reset() override
    {
        for (size_t i = 0; i < NUM_COMBS; ++i)
        {
            std::fill(combBuffers[i].begin(), combBuffers[i].end(), 0.0f);
            combIndices[i] = 0;
            combFilterStates[i] = 0.0f;
        }
        
        for (size_t i = 0; i < NUM_ALLPASS; ++i)
        {
            std::fill(allpassBuffers[i].begin(), allpassBuffers[i].end(), 0.0f);
            allpassIndices[i] = 0;
        }
        
        std::fill(preDelayBuffer.begin(), preDelayBuffer.end(), 0.0f);
        preDelayWriteIdx = 0;
    }
    
private:
    static constexpr size_t NUM_COMBS = 8;
    static constexpr size_t NUM_ALLPASS = 4;
    
    void updateParameters()
    {
        // Feedback del comb: roomSize 0 -> 0.7 (short), roomSize 1 -> 0.98 (long)
        combFeedback = 0.7f + roomSize * 0.28f;
        
        // Damping filter coefficients
        // damping 0 = bright (LP cutoff alto), damping 1 = very dark (LP cutoff bajo)
        damp1 = 1.0f - damping * 0.4f;  // 1.0 to 0.6
        damp2 = damping * 0.4f;          // 0.0 to 0.4
    }
    
    // Comb filters
    std::array<std::vector<float>, NUM_COMBS> combBuffers;
    std::array<size_t, NUM_COMBS> combIndices {};
    std::array<float, NUM_COMBS> combFilterStates {};
    float combFeedback = 0.84f;
    float damp1 = 0.8f;
    float damp2 = 0.2f;
    
    // Allpass filters
    std::array<std::vector<float>, NUM_ALLPASS> allpassBuffers;
    std::array<size_t, NUM_ALLPASS> allpassIndices {};
    static constexpr float allpassFeedback = 0.5f;
    
    // Pre-delay
    std::vector<float> preDelayBuffer;
    size_t preDelayWriteIdx = 0;
    size_t preDelaySamples = 0;
    float preDelayMs = 0.0f;
    
    float roomSize = 0.5f;
    float damping = 0.5f;
};

} // namespace kndl
