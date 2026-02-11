#pragma once

#include "Filter.h"
#include <vector>
#include <cmath>

namespace kndl {

/**
 * Formant Filter - Simula formantes vocales usando múltiples resonadores.
 */
class FormantFilter : public Filter
{
public:
    void prepare(double newSampleRate, int /*samplesPerBlock*/) override
    {
        sampleRate = newSampleRate;
        reset();
    }
    
    void setCutoff(float frequency) override
    {
        // Formant frequency (principal)
        formantFreq = juce::jlimit(200.0f, 4000.0f, frequency);
        updateCoefficients();
    }
    
    void setResonance(float res) override
    {
        resonance = juce::jlimit(0.1f, 0.99f, res);
        updateCoefficients();
    }
    
    void setType(FilterType /*type*/) override
    {
        // Formant filter doesn't use FilterType
    }
    
    void setFormantVowel(int vowel) // 0-4: A, E, I, O, U
    {
        vowelIndex = juce::jlimit(0, 4, vowel);
        updateFormantFrequencies();
    }
    
    float process(float input) override
    {
        if (!std::isfinite(input))
            input = 0.0f;
        
        float output = input;
        
        // Apply each formant resonator
        for (size_t i = 0; i < 3; ++i)
        {
            float freq = formantFreqs[i];
            float bw = formantBWs[i];
            
            // Simple resonator (bandpass)
            float w = 2.0f * juce::MathConstants<float>::pi * freq / static_cast<float>(sampleRate);
            float r = std::exp(-juce::MathConstants<float>::pi * bw / static_cast<float>(sampleRate));
            
            float cosw = std::cos(w);
            float a1 = -2.0f * r * cosw;
            float a2 = r * r;
            float b0 = r * std::sin(w);
            
            // Process resonator
            float y = b0 * input + a1 * x1[i] + a2 * x2[i];
            x2[i] = x1[i];
            x1[i] = y;
            
            output += y * formantGains[i];
        }
        
        if (!std::isfinite(output))
            output = 0.0f;
        
        return output;
    }
    
    void reset() override
    {
        for (size_t i = 0; i < 3; ++i)
        {
            x1[i] = 0.0f;
            x2[i] = 0.0f;
        }
    }
    
private:
    void updateFormantFrequencies()
    {
        // Vowel formants (Hz): F1, F2, F3
        static const float vowels[5][3] = {
            { 730, 1090, 2440 }, // A
            { 270, 2290, 3010 }, // E
            { 390, 1990, 2550 }, // I
            { 570, 840, 2410 },  // O
            { 300, 870, 2240 }   // U
        };
        
        for (size_t i = 0; i < 3; ++i)
        {
            formantFreqs[i] = vowels[vowelIndex][i];
            formantBWs[i] = formantFreqs[i] * 0.1f; // 10% bandwidth
            formantGains[i] = 1.0f / 3.0f; // Equal gain
        }
    }
    
    void updateCoefficients()
    {
        // Coefficients updated in process()
    }
    
    float formantFreq = 1000.0f;
    std::array<float, 3> formantFreqs { 730.0f, 1090.0f, 2440.0f };
    std::array<float, 3> formantBWs { 73.0f, 109.0f, 244.0f };
    std::array<float, 3> formantGains { 0.33f, 0.33f, 0.33f };
    std::array<float, 3> x1 { 0.0f, 0.0f, 0.0f };
    std::array<float, 3> x2 { 0.0f, 0.0f, 0.0f };
    int vowelIndex = 0;
};

/**
 * Comb Filter - Delay con feedback para efectos resonantes.
 */
class CombFilter : public Filter
{
public:
    void prepare(double newSampleRate, int /*samplesPerBlock*/) override
    {
        sampleRate = newSampleRate;
        maxDelaySamples = static_cast<int>(sampleRate * 0.1); // Max 100ms
        buffer.resize(static_cast<size_t>(maxDelaySamples), 0.0f);
        writeIndex = 0;
        reset();
    }
    
    void setCutoff(float frequency) override
    {
        // Delay time based on frequency (period)
        float period = 1.0f / (frequency + 1.0f);
        delayTime = period;
        delaySamples = static_cast<int>(delayTime * static_cast<float>(sampleRate));
        delaySamples = juce::jlimit(1, maxDelaySamples - 1, delaySamples);
    }
    
    void setResonance(float res) override
    {
        feedback = juce::jlimit(-0.99f, 0.99f, res);
    }
    
    void setType(FilterType /*type*/) override
    {
        // Comb filter doesn't use FilterType
    }
    
    float process(float input) override
    {
        if (!std::isfinite(input))
            input = 0.0f;
        
        // Read delayed sample
        int readIndex = writeIndex - delaySamples;
        while (readIndex < 0)
            readIndex += maxDelaySamples;
        
        float delayed = buffer[static_cast<size_t>(readIndex)];
        
        // Write input + feedback
        buffer[static_cast<size_t>(writeIndex)] = input + delayed * feedback;
        
        writeIndex = (writeIndex + 1) % maxDelaySamples;
        
        float output = input + delayed;
        
        if (!std::isfinite(output))
            output = 0.0f;
        
        return output;
    }
    
    void reset() override
    {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writeIndex = 0;
    }
    
private:
    std::vector<float> buffer;
    int maxDelaySamples = 0;
    int writeIndex = 0;
    int delaySamples = 0;
    float delayTime = 0.001f;
    float feedback = 0.5f;
};

/**
 * Notch Filter - Elimina una frecuencia específica.
 */
class NotchFilter : public Filter
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
        notchFreq = juce::jlimit(20.0f, static_cast<float>(sampleRate * 0.45), frequency);
        updateCoefficients();
    }
    
    void setResonance(float res) override
    {
        Q = 0.5f + res * 49.5f; // 0.5 to 50
        updateCoefficients();
    }
    
    void setType(FilterType /*type*/) override
    {
        // Notch filter doesn't use FilterType
    }
    
    float process(float input) override
    {
        if (!std::isfinite(input))
            input = 0.0f;
        
        // Biquad notch filter
        float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        
        x2 = x1;
        x1 = input;
        y2 = y1;
        y1 = output;
        
        if (!std::isfinite(output))
            output = 0.0f;
        
        return output;
    }
    
    void reset() override
    {
        x1 = x2 = 0.0f;
        y1 = y2 = 0.0f;
    }
    
private:
    void updateCoefficients()
    {
        float w = 2.0f * juce::MathConstants<float>::pi * notchFreq / static_cast<float>(sampleRate);
        float cosw = std::cos(w);
        float sinw = std::sin(w);
        float alpha = sinw / (2.0f * Q);
        
        float nb0 = 1.0f;
        float nb1 = -2.0f * cosw;
        float nb2 = 1.0f;
        float na0 = 1.0f + alpha;
        float na1 = -2.0f * cosw;
        float na2 = 1.0f - alpha;
        
        // Normalize
        b0 = nb0 / na0;
        b1 = nb1 / na0;
        b2 = nb2 / na0;
        a1 = na1 / na0;
        a2 = na2 / na0;
    }
    
    float notchFreq = 1000.0f;
    float Q = 10.0f;
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;
    float x1 = 0.0f, x2 = 0.0f;
    float y1 = 0.0f, y2 = 0.0f;
};

} // namespace kndl
