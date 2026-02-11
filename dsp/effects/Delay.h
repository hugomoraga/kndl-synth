#pragma once

#include "Effect.h"
#include <vector>
#include <cmath>

namespace kndl {

/**
 * Delay estéreo-compatible con:
 * - Interpolación cúbica (Hermite) para lectura suave
 * - Suavizado del tiempo de delay (evita clicks al cambiar)
 * - Filtro high-cut en el feedback (ecos progresivamente más oscuros)
 * - Saturación suave solo cuando el feedback se acerca a la inestabilidad
 */
class Delay : public Effect
{
public:
    void prepare(double newSampleRate, int /*samplesPerBlock*/) override
    {
        sampleRate = newSampleRate;
        
        // Buffer para hasta 2 segundos de delay
        bufferSize = static_cast<int>(sampleRate * 2.0);
        buffer.resize(static_cast<size_t>(bufferSize), 0.0f);
        writeIndex = 0;
        
        // Inicializar delay suavizado al valor actual
        updateDelayTime();
        smoothedDelaySamples = targetDelaySamples;
        
        // Coeficiente de suavizado: ~5ms de transición
        smoothCoeff = 1.0f - std::exp(-1.0f / (static_cast<float>(sampleRate) * 0.005f));
        
        // Filtro high-cut en feedback: inicializar estado
        feedbackFilterState = 0.0f;
        updateDampingCoeff();
    }
    
    void setDelayTime(float timeMs)
    {
        delayTimeMs = juce::jlimit(1.0f, 2000.0f, timeMs);
        updateDelayTime();
    }
    
    void setFeedback(float fb)
    {
        feedback = juce::jlimit(0.0f, 0.95f, fb);
    }
    
    void setDamping(float damp)
    {
        // 0.0 = bright (no filter), 1.0 = very dark
        dampAmount = juce::jlimit(0.0f, 1.0f, damp);
        updateDampingCoeff();
    }
    
    float process(float input) override
    {
        if (!enabled || buffer.empty())
            return input;
        
        // Suavizar el tiempo de delay para evitar clicks
        smoothedDelaySamples += smoothCoeff * (targetDelaySamples - smoothedDelaySamples);
        
        // Leer del buffer con interpolación cúbica (Hermite)
        float readPos = static_cast<float>(writeIndex) - smoothedDelaySamples;
        if (readPos < 0.0f)
            readPos += static_cast<float>(bufferSize);
        
        float delayed = hermiteInterpolate(readPos);
        
        // Filtro high-cut en el path de feedback (one-pole LP)
        // Cada repetición se vuelve más oscura, como un delay analógico
        feedbackFilterState += dampCoeff * (delayed - feedbackFilterState);
        float feedbackSignal = feedbackFilterState * feedback;
        
        // Saturación suave SOLO cuando hay riesgo de inestabilidad
        // Para señales normales (<1.0), tanh(x) ≈ x (transparente)
        // Para señales grandes, comprime suavemente
        if (std::abs(feedbackSignal) > 0.9f)
            feedbackSignal = std::tanh(feedbackSignal);
        
        // Escribir al buffer
        buffer[static_cast<size_t>(writeIndex)] = input + feedbackSignal;
        
        // Avanzar write index
        writeIndex = (writeIndex + 1) % bufferSize;
        
        // Mix dry/wet
        return input * (1.0f - mix) + delayed * mix;
    }
    
    void reset() override
    {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writeIndex = 0;
        feedbackFilterState = 0.0f;
        smoothedDelaySamples = targetDelaySamples;
    }
    
private:
    void updateDelayTime()
    {
        targetDelaySamples = (delayTimeMs / 1000.0f) * static_cast<float>(sampleRate);
        targetDelaySamples = juce::jlimit(1.0f, static_cast<float>(bufferSize - 1), targetDelaySamples);
    }
    
    void updateDampingCoeff()
    {
        // Frecuencia de corte del filtro de damping: de 18kHz (bright) a 800Hz (dark)
        float cutoffHz = 18000.0f * std::pow(800.0f / 18000.0f, dampAmount);
        float wc = 2.0f * juce::MathConstants<float>::pi * cutoffHz / static_cast<float>(sampleRate);
        dampCoeff = wc / (1.0f + wc);  // One-pole LP coefficient
    }
    
    // Interpolación cúbica Hermite - más suave que lineal, menos artefactos
    float hermiteInterpolate(float readPos) const
    {
        int idx0 = static_cast<int>(readPos);
        float frac = readPos - static_cast<float>(idx0);
        
        // 4 puntos para interpolación cúbica
        auto readSample = [this](int idx) -> float {
            idx = ((idx % bufferSize) + bufferSize) % bufferSize;
            return buffer[static_cast<size_t>(idx)];
        };
        
        float y0 = readSample(idx0 - 1);
        float y1 = readSample(idx0);
        float y2 = readSample(idx0 + 1);
        float y3 = readSample(idx0 + 2);
        
        // Hermite polynomial
        float c0 = y1;
        float c1 = 0.5f * (y2 - y0);
        float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        
        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }
    
    std::vector<float> buffer;
    int bufferSize = 0;
    int writeIndex = 0;
    
    float targetDelaySamples = 0.0f;
    float smoothedDelaySamples = 0.0f;
    float smoothCoeff = 0.01f;
    
    float delayTimeMs = 250.0f;
    float feedback = 0.3f;
    float dampAmount = 0.3f;
    float dampCoeff = 0.5f;         // One-pole LP coefficient for feedback filter
    float feedbackFilterState = 0.0f;
};

} // namespace kndl
