#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>

namespace kndl {

/**
 * Spellbook Modulator - Genera múltiples señales moduladoras
 * leyendo la posición de un punto que recorre formas geométricas.
 * 
 * Características:
 * - 7 formas geométricas (circle, triangle, square, pentagon, star, spiral, lemniscate)
 * - Master clock sync/free
 * - Speed multipliers por salida (8-16 salidas)
 * - Rango hasta audio-rate
 * - Outputs normalizados -1..+1 o 0..1 configurable
 */
class Spellbook
{
public:
    enum class Shape
    {
        Circle = 0,
        Triangle,
        Square,
        Pentagon,
        Star,
        Spiral,
        Lemniscate
    };
    
    enum class OutputRange
    {
        Bipolar,  // -1 to +1
        Unipolar  // 0 to 1
    };
    
    static constexpr int MAX_OUTPUTS = 16;
    
    Spellbook()
    {
        for (size_t i = 0; i < static_cast<size_t>(MAX_OUTPUTS); ++i)
        {
            speedMultipliers[i] = 1.0f + static_cast<float>(i) * 0.1f;
            outputRanges[i] = OutputRange::Bipolar;
        }
    }
    
    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        invSampleRate = 1.0 / sampleRate;
        reset();
    }
    
    void setShape(Shape newShape)
    {
        shape = newShape;
    }
    
    void setBaseRate(float rateHz) // Base frequency in Hz
    {
        baseRate = juce::jlimit(0.01f, static_cast<float>(sampleRate * 0.45f), rateHz);
    }
    
    void setSpeedMultiplier(int outputIndex, float multiplier)
    {
        if (outputIndex >= 0 && outputIndex < MAX_OUTPUTS)
        {
            speedMultipliers[static_cast<size_t>(outputIndex)] = juce::jlimit(0.01f, 100.0f, multiplier);
        }
    }
    
    void setOutputRange(int outputIndex, OutputRange range)
    {
        if (outputIndex >= 0 && outputIndex < MAX_OUTPUTS)
        {
            outputRanges[static_cast<size_t>(outputIndex)] = range;
        }
    }
    
    void setClockSync(bool sync, float bpm = 120.0f)
    {
        clockSync = sync;
        if (sync)
        {
            beatsPerSecond = bpm / 60.0f;
        }
    }
    
    void setNumOutputs(int num)
    {
        numOutputs = juce::jlimit(1, MAX_OUTPUTS, num);
    }
    
    void process()
    {
        // Update phase based on rate and sync
        float phaseIncrement;
        if (clockSync)
        {
            // Sync to tempo (1 cycle per beat)
            phaseIncrement = static_cast<float>(beatsPerSecond * invSampleRate);
        }
        else
        {
            phaseIncrement = baseRate * static_cast<float>(invSampleRate);
        }
        
        // Update master phase
        masterPhase += phaseIncrement;
        if (masterPhase >= 1.0f)
            masterPhase -= 1.0f;
        
        // Generate outputs
        for (int i = 0; i < numOutputs; ++i)
        {
            float phase = masterPhase * speedMultipliers[static_cast<size_t>(i)];
            phase = phase - std::floor(phase); // Wrap to 0-1
            
            float x, y;
            generateShape(phase, x, y);
            
            // Store outputs (can use x, y, or combination)
            outputs[static_cast<size_t>(i * 2)] = normalizeOutput(x, static_cast<size_t>(i));
            outputs[static_cast<size_t>(i * 2 + 1)] = normalizeOutput(y, static_cast<size_t>(i));
        }
    }
    
    float getOutput(int index) const
    {
        if (index >= 0 && index < numOutputs * 2)
            return outputs[static_cast<size_t>(index)];
        return 0.0f;
    }
    
    void reset()
    {
        masterPhase = 0.0f;
        std::fill(outputs.begin(), outputs.end(), 0.0f);
    }
    
private:
    void generateShape(float phase, float& x, float& y)
    {
        float angle = phase * 2.0f * juce::MathConstants<float>::pi;
        
        switch (shape)
        {
            case Shape::Circle:
            {
                x = std::cos(angle);
                y = std::sin(angle);
                break;
            }
            
            case Shape::Triangle:
            {
                // Equilateral triangle
                float localAngle = std::fmod(angle, 2.0f * juce::MathConstants<float>::pi / 3.0f);
                
                float r = 1.0f / std::cos(localAngle - juce::MathConstants<float>::pi / 6.0f);
                x = r * std::cos(angle);
                y = r * std::sin(angle);
                break;
            }
            
            case Shape::Square:
            {
                // Square using superellipse approximation
                float n = 100.0f; // High power for sharp corners
                float cosA = std::cos(angle);
                float sinA = std::sin(angle);
                float signX = cosA >= 0.0f ? 1.0f : -1.0f;
                float signY = sinA >= 0.0f ? 1.0f : -1.0f;
                x = signX * std::pow(std::abs(cosA), 2.0f / n);
                y = signY * std::pow(std::abs(sinA), 2.0f / n);
                break;
            }
            
            case Shape::Pentagon:
            {
                // Regular pentagon
                float localAngle = std::fmod(angle, 2.0f * juce::MathConstants<float>::pi / 5.0f);
                
                float r = 1.0f / std::cos(localAngle - juce::MathConstants<float>::pi / 5.0f);
                x = r * std::cos(angle);
                y = r * std::sin(angle);
                break;
            }
            
            case Shape::Star:
            {
                // 5-pointed star
                float starAngle = angle * 2.5f; // 5 points, but we traverse faster
                float r = 0.5f + 0.5f * std::sin(starAngle * 2.0f);
                x = r * std::cos(angle);
                y = r * std::sin(angle);
                break;
            }
            
            case Shape::Spiral:
            {
                // Archimedean spiral
                float r = phase; // Radius increases with phase
                x = r * std::cos(angle);
                y = r * std::sin(angle);
                break;
            }
            
            case Shape::Lemniscate:
            {
                // Lemniscate of Bernoulli (infinity symbol)
                float t = angle;
                float r = std::sqrt(2.0f * std::cos(2.0f * t));
                x = r * std::cos(t);
                y = r * std::sin(t);
                // Handle invalid values
                if (!std::isfinite(x)) x = 0.0f;
                if (!std::isfinite(y)) y = 0.0f;
                break;
            }
        }
    }
    
    float normalizeOutput(float value, size_t outputIndex)
    {
        if (outputRanges[outputIndex] == OutputRange::Unipolar)
        {
            return (value + 1.0f) * 0.5f; // Convert -1..+1 to 0..1
        }
        return value; // Already -1..+1
    }
    
    double sampleRate = 44100.0;
    double invSampleRate = 1.0 / 44100.0;
    
    Shape shape = Shape::Circle;
    float baseRate = 1.0f; // Hz
    bool clockSync = false;
    float beatsPerSecond = 2.0f; // 120 BPM default
    
    std::array<float, MAX_OUTPUTS> speedMultipliers;
    std::array<OutputRange, MAX_OUTPUTS> outputRanges;
    
    float masterPhase = 0.0f;
    std::array<float, MAX_OUTPUTS * 2> outputs { 0.0f };
    int numOutputs = 8;
};

} // namespace kndl
