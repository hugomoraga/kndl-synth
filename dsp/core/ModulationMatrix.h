#pragma once

#include <JuceHeader.h>
#include <array>
#include <functional>
#include <cmath>

namespace kndl {

/**
 * Fuentes de modulación disponibles.
 */
enum class ModSource
{
    None = 0,
    LFO1,
    LFO2,
    AmpEnv,
    FilterEnv,
    Velocity,
    ModWheel,
    Aftertouch,
    OrbitA,   // Orbit output 0 (1st output X)
    OrbitB,   // Orbit output 1 (1st output Y)
    OrbitC,   // Orbit output 2 (2nd output X)
    OrbitD,   // Orbit output 3 (2nd output Y)
    NumSources
};

/**
 * Destinos de modulación disponibles.
 */
enum class ModDestination
{
    None = 0,
    Osc1Pitch,
    Osc2Pitch,
    Osc1Level,
    Osc2Level,
    SubLevel,
    FilterCutoff,
    FilterResonance,
    AmpLevel,
    LFO1Rate,
    LFO2Rate,
    NumDestinations
};

/**
 * Curvas de modulación disponibles.
 */
enum class ModCurve
{
    Linear = 0,
    Exponential,
    Logarithmic,
    SCurve,
    Sine,
    NumCurves
};

/**
 * Una conexión de modulación.
 */
struct ModConnection
{
    ModSource source = ModSource::None;
    ModDestination destination = ModDestination::None;
    float amount = 0.0f;  // -1 a 1 (bipolar)
    ModCurve curve = ModCurve::Linear;
    float smoothingTime = 0.0f; // ms, 0 = no smoothing
    
    bool isActive() const { return source != ModSource::None && destination != ModDestination::None; }
};

/**
 * ModulationMatrix - Sistema de ruteo de modulación.
 * Permite conectar cualquier fuente a cualquier destino con cantidad variable.
 * 
 * Uso:
 *   1. Configurar conexiones con setConnection()
 *   2. Cada frame, actualizar valores de fuentes con setSourceValue()
 *   3. Obtener modulación total para cada destino con getModulatedValue()
 */
class ModulationMatrix
{
public:
    static constexpr int MaxConnections = 16;
    
    ModulationMatrix()
    {
        // Inicializar valores de fuentes
        sourceValues.fill(0.0f);
        destinationBaseValues.fill(0.0f);
        smoothedAmounts.fill(0.0f);
    }
    
    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        for (size_t i = 0; i < MaxConnections; ++i)
        {
            smoothers[i].reset(newSampleRate, 0.01); // Default 10ms
        }
    }
    
    /**
     * Configura una conexión de modulación.
     */
    void setConnection(int slot, ModSource source, ModDestination dest, float amount, 
                       ModCurve curve = ModCurve::Linear, float smoothingTime = 0.0f)
    {
        if (slot >= 0 && slot < MaxConnections)
        {
            size_t idx = static_cast<size_t>(slot);
            connections[idx] = { source, dest, amount, curve, smoothingTime };
            
            // Update smoother
            if (smoothingTime > 0.0f)
                smoothers[idx].reset(sampleRate, smoothingTime * 0.001);
            else
                smoothers[idx].setCurrentAndTargetValue(amount);
        }
    }
    
    /**
     * Limpia una conexión.
     */
    void clearConnection(int slot)
    {
        if (slot >= 0 && slot < MaxConnections)
        {
            connections[static_cast<size_t>(slot)] = {};
        }
    }
    
    /**
     * Actualiza el valor de una fuente de modulación.
     * Llamar cada sample o cada bloque según la fuente.
     */
    void setSourceValue(ModSource source, float value)
    {
        if (source != ModSource::None && source != ModSource::NumSources)
        {
            sourceValues[static_cast<size_t>(source)] = value;
        }
    }
    
    /**
     * Establece el valor base (sin modular) de un destino.
     */
    void setDestinationBaseValue(ModDestination dest, float value)
    {
        if (dest != ModDestination::None && dest != ModDestination::NumDestinations)
        {
            destinationBaseValues[static_cast<size_t>(dest)] = value;
        }
    }
    
    /**
     * Actualiza los smoothed values. Llamar cada sample.
     */
    void updateSmoothing()
    {
        for (size_t i = 0; i < MaxConnections; ++i)
        {
            if (connections[i].smoothingTime > 0.0f)
            {
                smoothedAmounts[i] = smoothers[i].getNextValue();
            }
            else
            {
                smoothedAmounts[i] = connections[i].amount;
            }
        }
    }
    
    /**
     * Aplica una curva a un valor de modulación.
     */
    float applyCurve(float value, ModCurve curve) const
    {
        switch (curve)
        {
            case ModCurve::Linear:
                return value;
                
            case ModCurve::Exponential:
                return value >= 0.0f ? value * value : -(value * value);
                
            case ModCurve::Logarithmic:
                if (std::abs(value) < 0.001f) return 0.0f;
                return value >= 0.0f ? std::log(1.0f + value * 9.0f) / std::log(10.0f) 
                                     : -std::log(1.0f - value * 9.0f) / std::log(10.0f);
                
            case ModCurve::SCurve:
            {
                float x = juce::jlimit(-1.0f, 1.0f, value);
                return x * x * (3.0f - 2.0f * std::abs(x));
            }
            
            case ModCurve::Sine:
                return std::sin(value * juce::MathConstants<float>::halfPi);
                
            case ModCurve::NumCurves:
            default:
                return value;
        }
    }
    
    /**
     * Obtiene el valor modulado para un destino.
     * Suma todas las modulaciones activas al valor base.
     * IMPORTANTE: Llamar updateSmoothing() UNA VEZ por sample antes de usar esto.
     */
    float getModulatedValue(ModDestination dest) const
    {
        if (dest == ModDestination::None || dest == ModDestination::NumDestinations)
            return 0.0f;
        
        float baseValue = destinationBaseValues[static_cast<size_t>(dest)];
        float modulation = 0.0f;
        
        for (size_t i = 0; i < MaxConnections; ++i)
        {
            const auto& conn = connections[i];
            if (conn.isActive() && conn.destination == dest)
            {
                float sourceVal = sourceValues[static_cast<size_t>(conn.source)];
                float amount = smoothedAmounts[i];
                float curved = applyCurve(sourceVal, conn.curve);
                modulation += curved * amount;
            }
        }
        
        return baseValue + modulation;
    }
    
    /**
     * Obtiene solo la cantidad de modulación (sin valor base).
     * IMPORTANTE: Llamar updateSmoothing() UNA VEZ por sample antes de usar esto.
     */
    float getModulationAmount(ModDestination dest) const
    {
        float modulation = 0.0f;
        
        for (size_t i = 0; i < MaxConnections; ++i)
        {
            const auto& conn = connections[i];
            if (conn.isActive() && conn.destination == dest)
            {
                float sourceVal = sourceValues[static_cast<size_t>(conn.source)];
                float amount = smoothedAmounts[i];
                float curved = applyCurve(sourceVal, conn.curve);
                modulation += curved * amount;
            }
        }
        
        return modulation;
    }
    
    /**
     * Obtiene una conexión para edición/visualización.
     */
    const ModConnection& getConnection(int slot) const
    {
        static ModConnection empty;
        if (slot >= 0 && slot < MaxConnections)
            return connections[static_cast<size_t>(slot)];
        return empty;
    }
    
    /**
     * Resetea todas las conexiones.
     */
    void reset()
    {
        for (auto& conn : connections)
            conn = {};
        sourceValues.fill(0.0f);
    }
    
private:
    std::array<ModConnection, MaxConnections> connections;
    std::array<float, static_cast<size_t>(ModSource::NumSources)> sourceValues;
    std::array<float, static_cast<size_t>(ModDestination::NumDestinations)> destinationBaseValues;
    
    // Smoothing
    std::array<juce::SmoothedValue<float>, MaxConnections> smoothers;
    std::array<float, MaxConnections> smoothedAmounts;
    double sampleRate = 44100.0;
};

} // namespace kndl
