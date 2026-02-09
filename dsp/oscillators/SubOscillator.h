#pragma once

#include "BasicOscillator.h"

namespace kndl {

/**
 * Sub-oscilador dedicado.
 * Siempre genera sine o square una o dos octavas abajo.
 * Optimizado para bajos potentes.
 */
class SubOscillator : public Oscillator
{
public:
    void prepare(double newSampleRate) override
    {
        sampleRate = newSampleRate;
        phase = 0.0;
    }
    
    void setFrequency(float freq) override
    {
        // Aplicar octava (siempre hacia abajo)
        float subFreq = freq * std::pow(2.0f, static_cast<float>(octaveShift));
        frequency = subFreq;
        phaseIncrement = frequency / static_cast<float>(sampleRate);
    }
    
    void setOctave(int octave)
    {
        // Limitar a -1 o -2 octavas
        octaveShift = juce::jlimit(-2, -1, octave);
    }
    
    float process() override
    {
        // Sub siempre es sine puro para máximo punch en bajos
        float output = std::sin(static_cast<float>(phase * juce::MathConstants<double>::twoPi));
        
        phase += phaseIncrement;
        if (phase >= 1.0)
            phase -= 1.0;
        
        return output;
    }
    
    void reset() override
    {
        phase = 0.0;
    }
    
private:
    double phase = 0.0;
    double phaseIncrement = 0.0;
    int octaveShift = -1;  // -1 = una octava abajo
};

} // namespace kndl
