#pragma once

#include <JuceHeader.h>
#include "../core/Parameters.h"

namespace kndl {

/**
 * Interface base para filtros.
 */
class Filter
{
public:
    virtual ~Filter() = default;
    
    virtual void prepare(double sampleRate, int samplesPerBlock) = 0;
    virtual void setCutoff(float frequency) = 0;
    virtual void setResonance(float resonance) = 0;
    virtual void setType(FilterType type) = 0;
    virtual float process(float input) = 0;
    virtual void reset() = 0;
    
protected:
    double sampleRate = 44100.0;
    float cutoff = 1000.0f;
    float resonance = 0.0f;
    FilterType filterType = FilterType::LowPass;
};

} // namespace kndl
