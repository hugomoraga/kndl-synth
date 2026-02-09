#pragma once

#include <JuceHeader.h>
#include <array>
#include <functional>

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
 * Una conexión de modulación.
 */
struct ModConnection
{
    ModSource source = ModSource::None;
    ModDestination destination = ModDestination::None;
    float amount = 0.0f;  // -1 a 1
    
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
    }
    
    /**
     * Configura una conexión de modulación.
     */
    void setConnection(int slot, ModSource source, ModDestination dest, float amount)
    {
        if (slot >= 0 && slot < MaxConnections)
        {
            connections[static_cast<size_t>(slot)] = { source, dest, amount };
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
     * Obtiene el valor modulado para un destino.
     * Suma todas las modulaciones activas al valor base.
     */
    float getModulatedValue(ModDestination dest) const
    {
        if (dest == ModDestination::None || dest == ModDestination::NumDestinations)
            return 0.0f;
        
        float baseValue = destinationBaseValues[static_cast<size_t>(dest)];
        float modulation = 0.0f;
        
        for (const auto& conn : connections)
        {
            if (conn.isActive() && conn.destination == dest)
            {
                float sourceVal = sourceValues[static_cast<size_t>(conn.source)];
                modulation += sourceVal * conn.amount;
            }
        }
        
        return baseValue + modulation;
    }
    
    /**
     * Obtiene solo la cantidad de modulación (sin valor base).
     */
    float getModulationAmount(ModDestination dest) const
    {
        float modulation = 0.0f;
        
        for (const auto& conn : connections)
        {
            if (conn.isActive() && conn.destination == dest)
            {
                float sourceVal = sourceValues[static_cast<size_t>(conn.source)];
                modulation += sourceVal * conn.amount;
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
};

} // namespace kndl
