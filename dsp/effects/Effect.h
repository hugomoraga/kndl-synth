#pragma once

#include <JuceHeader.h>

namespace kndl {

/**
 * Interfaz base para todos los efectos.
 */
class Effect
{
public:
    virtual ~Effect() = default;
    
    virtual void prepare(double sampleRate, int samplesPerBlock) = 0;
    virtual float process(float input) = 0;
    virtual void reset() = 0;
    
    void setEnabled(bool shouldBeEnabled) { enabled = shouldBeEnabled; }
    bool isEnabled() const { return enabled; }
    
    void setMix(float wetDry) { mix = juce::jlimit(0.0f, 1.0f, wetDry); }
    float getMix() const { return mix; }
    
protected:
    double sampleRate = 44100.0;
    bool enabled = false;
    float mix = 0.5f;
};

} // namespace kndl
