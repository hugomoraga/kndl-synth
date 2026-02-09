#pragma once

#include <JuceHeader.h>
#include "../core/Parameters.h"

namespace kndl {

/**
 * Interface base para todos los osciladores.
 * Cada oscilador debe poder:
 * - Prepararse con sample rate
 * - Setear frecuencia
 * - Generar siguiente sample
 * - Resetearse
 */
class Oscillator
{
public:
    virtual ~Oscillator() = default;
    
    virtual void prepare(double sampleRate) = 0;
    virtual void setFrequency(float frequency) = 0;
    virtual float process() = 0;
    virtual void reset() = 0;
    
    virtual void setWaveform(Waveform waveform) { currentWaveform = waveform; }
    Waveform getWaveform() const { return currentWaveform; }
    
protected:
    double sampleRate = 44100.0;
    float frequency = 440.0f;
    Waveform currentWaveform = Waveform::Saw;
};

} // namespace kndl
