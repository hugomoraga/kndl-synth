#pragma once

#include <cstdint>
#include <cmath>

namespace kndl {

enum class NoiseType {
    White = 0,
    Pink,
    Crackle
};

/**
 * Generador de ruido de alta calidad.
 * - White: ruido blanco uniforme (energía igual en todas las frecuencias)
 * - Pink: ruido rosa (-3dB/octava, suena más "natural")
 * - Crackle: impulsos sparse, como vinilo o interferencia eléctrica
 * 
 * Usa xorshift32 para generación rápida y thread-safe.
 */
class NoiseGenerator
{
public:
    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        b0 = b1 = b2 = b3 = b4 = b5 = b6 = 0.0f;
        lastCrackle = 0.0f;
    }
    
    void setType(NoiseType type) { noiseType = type; }
    
    float process()
    {
        switch (noiseType)
        {
            case NoiseType::White:   return processWhite();
            case NoiseType::Pink:    return processPink();
            case NoiseType::Crackle: return processCrackle();
            default:                 return processWhite();
        }
    }
    
    void reset()
    {
        b0 = b1 = b2 = b3 = b4 = b5 = b6 = 0.0f;
        lastCrackle = 0.0f;
    }
    
private:
    // Fast xorshift32 PRNG (thread-safe, no global state)
    float randomFloat()
    {
        rngState ^= rngState << 13;
        rngState ^= rngState >> 17;
        rngState ^= rngState << 5;
        return static_cast<float>(rngState) / static_cast<float>(UINT32_MAX) * 2.0f - 1.0f;
    }
    
    float processWhite()
    {
        return randomFloat();
    }
    
    float processPink()
    {
        // Paul Kellet's refined pink noise algorithm
        // Spectral density: -3dB/octave
        float white = randomFloat();
        b0 = 0.99886f * b0 + white * 0.0555179f;
        b1 = 0.99332f * b1 + white * 0.0750759f;
        b2 = 0.96900f * b2 + white * 0.1538520f;
        b3 = 0.86650f * b3 + white * 0.3104856f;
        b4 = 0.55000f * b4 + white * 0.5329522f;
        b5 = -0.7616f * b5 - white * 0.0168980f;
        float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
        b6 = white * 0.115926f;
        return pink * 0.11f; // Normalize to ~[-1, 1]
    }
    
    float processCrackle()
    {
        // Sparse impulse noise: vinyl crackle / electrical interference
        float r = (randomFloat() + 1.0f) * 0.5f; // 0 to 1
        if (r > 0.997f) // ~0.3% chance per sample = sparse crackles
            lastCrackle = (randomFloat() > 0.0f) ? 1.0f : -1.0f;
        else
            lastCrackle *= 0.92f; // Fast exponential decay
        return lastCrackle;
    }
    
    double sampleRate = 44100.0;
    NoiseType noiseType = NoiseType::White;
    uint32_t rngState = 0x12345678; // PRNG seed
    
    // Pink noise filter state (Paul Kellet coefficients)
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, b3 = 0.0f;
    float b4 = 0.0f, b5 = 0.0f, b6 = 0.0f;
    
    // Crackle state
    float lastCrackle = 0.0f;
};

} // namespace kndl
