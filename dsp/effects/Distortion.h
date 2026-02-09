#pragma once

#include "Effect.h"
#include <cmath>

namespace kndl {

/**
 * Distortion con varios modos.
 */
class Distortion : public Effect
{
public:
    enum class Mode
    {
        SoftClip = 0,
        HardClip,
        Foldback,
        Bitcrush
    };
    
    void prepare(double newSampleRate, int /*samplesPerBlock*/) override
    {
        sampleRate = newSampleRate;
    }
    
    void setDrive(float driveAmount)
    {
        drive = juce::jlimit(1.0f, 50.0f, driveAmount);
    }
    
    void setMode(Mode newMode)
    {
        mode = newMode;
    }
    
    float process(float input) override
    {
        if (!enabled)
            return input;
        
        float driven = input * drive;
        float distorted = 0.0f;
        
        switch (mode)
        {
            case Mode::SoftClip:
                distorted = std::tanh(driven);
                break;
                
            case Mode::HardClip:
                distorted = juce::jlimit(-1.0f, 1.0f, driven);
                break;
                
            case Mode::Foldback:
                // Foldback distortion
                while (driven > 1.0f || driven < -1.0f)
                {
                    if (driven > 1.0f)
                        driven = 2.0f - driven;
                    else if (driven < -1.0f)
                        driven = -2.0f - driven;
                }
                distorted = driven;
                break;
                
            case Mode::Bitcrush:
                {
                    // Reduce bit depth
                    float bits = 16.0f - (drive - 1.0f) * 0.3f;  // 16 bits down to ~1 bit
                    bits = juce::jmax(1.0f, bits);
                    float levels = std::pow(2.0f, bits);
                    distorted = std::round(input * levels) / levels;
                }
                break;
        }
        
        // Compensar ganancia
        float output = distorted / std::sqrt(drive);
        
        // Mix
        return input * (1.0f - mix) + output * mix;
    }
    
    void reset() override
    {
        // No state to reset
    }
    
private:
    float drive = 1.0f;
    Mode mode = Mode::SoftClip;
};

} // namespace kndl
