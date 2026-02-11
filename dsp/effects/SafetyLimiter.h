#pragma once

#include <cmath>
#include <algorithm>

namespace kndl {

/**
 * SafetyLimiter - Protector de audio para evitar daño a parlantes.
 * 
 * Implementa un limiter de pico con:
 * 1. Envelope follower con attack rápido / release suave
 * 2. Gain reduction suave cuando la señal excede el threshold
 * 3. Brickwall hard clip como red de seguridad final
 * 
 * Siempre garantiza que la salida esté dentro de [-ceiling, +ceiling].
 */
class SafetyLimiter
{
public:
    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        updateCoefficients();
        reset();
    }
    
    /**
     * Threshold en dBFS. Señales por encima serán limitadas.
     * Default: -1.0 dBFS (headroom para evitar clips intersample)
     */
    void setThreshold(float thresholdDb)
    {
        threshold = std::pow(10.0f, thresholdDb / 20.0f);
    }
    
    /**
     * Ceiling absoluto en dBFS. Nada pasa de aquí. Jamás.
     * Default: -0.1 dBFS
     */
    void setCeiling(float ceilingDb)
    {
        ceiling = std::pow(10.0f, ceilingDb / 20.0f);
    }
    
    void setAttackMs(float ms)
    {
        attackMs = ms;
        updateCoefficients();
    }
    
    void setReleaseMs(float ms)
    {
        releaseMs = ms;
        updateCoefficients();
    }
    
    float process(float input)
    {
        float absInput = std::abs(input);
        
        // --- Envelope follower (peak detector) ---
        // Fast attack, slow release
        if (absInput > envelope)
            envelope = attackCoeff * envelope + (1.0f - attackCoeff) * absInput;
        else
            envelope = releaseCoeff * envelope + (1.0f - releaseCoeff) * absInput;
        
        // --- Gain computation ---
        float gainReduction = 1.0f;
        
        if (envelope > threshold)
        {
            // Compute how much we need to reduce
            // Soft knee: gentle compression before threshold, hard limiting above
            float overDb = 20.0f * std::log10(envelope / threshold);
            
            // Compression ratio increases with overshoot (adaptive)
            // Near threshold: ~4:1, far above: approaches infinity:1
            float targetDb = overDb / (1.0f + overDb * 0.5f); // Soft-knee limiter curve
            float targetLinear = threshold * std::pow(10.0f, targetDb / 20.0f);
            
            gainReduction = targetLinear / std::max(envelope, 0.0001f);
        }
        
        // Smooth the gain reduction to avoid zipper noise
        if (gainReduction < smoothedGain)
            smoothedGain = gainAttackCoeff * smoothedGain + (1.0f - gainAttackCoeff) * gainReduction;
        else
            smoothedGain = gainReleaseCoeff * smoothedGain + (1.0f - gainReleaseCoeff) * gainReduction;
        
        float output = input * smoothedGain;
        
        // --- Brickwall hard clip (absolute safety net) ---
        // This should rarely engage if the limiter is working properly
        if (output > ceiling)
            output = ceiling;
        else if (output < -ceiling)
            output = -ceiling;
        
        // Track gain reduction for metering
        currentGainReductionDb = (smoothedGain < 1.0f) 
            ? 20.0f * std::log10(std::max(smoothedGain, 0.0001f))
            : 0.0f;
        
        return output;
    }
    
    void reset()
    {
        envelope = 0.0f;
        smoothedGain = 1.0f;
        currentGainReductionDb = 0.0f;
    }
    
    /** Returns current gain reduction in dB (negative value when limiting) */
    float getGainReductionDb() const { return currentGainReductionDb; }
    
    /** Returns true if limiter is currently active */
    bool isLimiting() const { return smoothedGain < 0.99f; }
    
private:
    void updateCoefficients()
    {
        if (sampleRate <= 0.0) return;
        
        // Envelope follower coefficients
        // Attack: very fast to catch transients (0.1ms default)
        attackCoeff = std::exp(-1.0f / (static_cast<float>(sampleRate) * attackMs * 0.001f));
        // Release: smooth to avoid pumping (50ms default)
        releaseCoeff = std::exp(-1.0f / (static_cast<float>(sampleRate) * releaseMs * 0.001f));
        
        // Gain smoothing coefficients (slightly slower to sound natural)
        gainAttackCoeff = std::exp(-1.0f / (static_cast<float>(sampleRate) * 0.0002f)); // 0.2ms
        gainReleaseCoeff = std::exp(-1.0f / (static_cast<float>(sampleRate) * 0.080f)); // 80ms
    }
    
    double sampleRate = 44100.0;
    
    // Settings
    float threshold = 0.891f;   // -1.0 dBFS
    float ceiling = 0.989f;     // -0.1 dBFS (absolute max)
    float attackMs = 0.1f;      // Very fast attack
    float releaseMs = 50.0f;    // Smooth release
    
    // State
    float envelope = 0.0f;
    float smoothedGain = 1.0f;
    float currentGainReductionDb = 0.0f;
    
    // Coefficients
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float gainAttackCoeff = 0.0f;
    float gainReleaseCoeff = 0.0f;
};

} // namespace kndl
