#pragma once

#include "Filter.h"
#include <vector>
#include <cmath>

namespace kndl {

/**
 * Formant Filter - Simula formantes vocales usando 3 biquad bandpass paralelos.
 * 
 * Cada vocal (A, E, I, O, U) tiene 3 frecuencias formantes características.
 * El cutoff desplaza todas las frecuencias proporcionalmente.
 * La resonancia controla el Q (ancho de banda) de los resonadores.
 */
class FormantFilter : public Filter
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
        // Use cutoff as a shift factor: ratio relative to 1000 Hz center
        float newShift = juce::jlimit(0.25f, 4.0f, frequency / 1000.0f);
        if (std::abs(newShift - cutoffShift) > 0.001f)
        {
            cutoffShift = newShift;
            dirty = true;
        }
    }
    
    void setResonance(float res) override
    {
        float newRes = juce::jlimit(0.0f, 1.0f, res);
        if (std::abs(newRes - resonance) > 0.001f)
        {
            resonance = newRes;
            dirty = true;
        }
    }
    
    void setType(FilterType /*type*/) override {}
    
    void setFormantVowel(int vowel)
    {
        int v = juce::jlimit(0, 4, vowel);
        if (v != vowelIndex)
        {
            vowelIndex = v;
            dirty = true;
        }
    }
    
    float process(float input) override
    {
        if (!std::isfinite(input))
            input = 0.0f;
        
        if (dirty)
        {
            updateCoefficients();
            dirty = false;
        }
        
        // Sum of 3 parallel bandpass filters (formant resonators)
        float output = 0.0f;
        
        for (size_t i = 0; i < 3; ++i)
        {
            // Biquad direct form II transposed
            float y = coeffs[i].b0 * input + state[i].s1;
            state[i].s1 = coeffs[i].b1 * input - coeffs[i].a1 * y + state[i].s2;
            state[i].s2 = coeffs[i].b2 * input - coeffs[i].a2 * y;
            
            // Safety clamp per-resonator
            y = juce::jlimit(-4.0f, 4.0f, y);
            
            output += y * formantGains[i];
        }
        
        if (!std::isfinite(output))
        {
            output = 0.0f;
            reset();
        }
        
        return output;
    }
    
    void reset() override
    {
        for (size_t i = 0; i < 3; ++i)
        {
            state[i].s1 = 0.0f;
            state[i].s2 = 0.0f;
        }
    }
    
private:
    struct BiquadCoeffs { float b0=0, b1=0, b2=0, a1=0, a2=0; };
    struct BiquadState  { float s1=0, s2=0; };
    
    void updateCoefficients()
    {
        if (sampleRate <= 0.0) return;
        
        // Vowel formant frequencies (Hz): F1, F2, F3
        static const float vowelFreqs[5][3] = {
            { 730,  1090, 2440 }, // A
            { 270,  2290, 3010 }, // E
            { 390,  1990, 2550 }, // I
            { 570,  840,  2410 }, // O
            { 300,  870,  2240 }  // U
        };
        
        // Vowel gains (relative amplitude of each formant, from speech data)
        static const float vowelAmps[5][3] = {
            { 1.0f, 0.5f,  0.3f  }, // A - strong F1
            { 0.6f, 0.8f,  0.3f  }, // E - strong F2
            { 0.5f, 0.7f,  0.35f }, // I - strong F2
            { 0.8f, 0.4f,  0.25f }, // O - strong F1
            { 0.7f, 0.35f, 0.2f  }  // U - strong F1, weak F2/F3
        };
        
        float nyquist = static_cast<float>(sampleRate) * 0.45f;
        
        // Q range: resonance 0 = wide (Q=2), resonance 1 = narrow/peaky (Q=12)
        float baseQ = 2.0f + resonance * 10.0f;
        
        for (size_t i = 0; i < 3; ++i)
        {
            // Apply cutoff shift to formant frequency
            float freq = vowelFreqs[vowelIndex][i] * cutoffShift;
            freq = juce::jlimit(80.0f, nyquist, freq);
            
            formantGains[i] = vowelAmps[vowelIndex][i];
            
            // Q increases slightly for higher formants
            float Q = baseQ * (1.0f + static_cast<float>(i) * 0.2f);
            
            // Compute biquad bandpass coefficients (constant 0 dB peak gain)
            float w0 = 2.0f * juce::MathConstants<float>::pi * freq / static_cast<float>(sampleRate);
            float cosw = std::cos(w0);
            float sinw = std::sin(w0);
            float alpha = sinw / (2.0f * Q);
            
            float a0 = 1.0f + alpha;
            
            // Bandpass (constant skirt gain, peak at 0 dB)
            coeffs[i].b0 =  (sinw * 0.5f) / a0;
            coeffs[i].b1 =  0.0f;
            coeffs[i].b2 = -(sinw * 0.5f) / a0;
            coeffs[i].a1 = (-2.0f * cosw)  / a0;
            coeffs[i].a2 = (1.0f - alpha)  / a0;
        }
    }
    
    float cutoffShift = 1.0f;
    int vowelIndex = 0;
    bool dirty = true;
    
    std::array<BiquadCoeffs, 3> coeffs;
    std::array<BiquadState, 3> state;
    std::array<float, 3> formantGains { 1.0f, 0.5f, 0.3f };
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
