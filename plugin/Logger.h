#pragma once

#include <JuceHeader.h>
#include <fstream>
#include <mutex>
#include <queue>
#include <atomic>

namespace kndl {

/**
 * Logger - Sistema de logging para debug del synth.
 * Escribe a un archivo en ~/Documents/KndlSynth/Logs/
 */
class Logger
{
public:
    enum class Level
    {
        Debug,
        Info,
        Warning,
        Error
    };
    
    static Logger& getInstance()
    {
        static Logger instance;
        return instance;
    }
    
    void setEnabled(bool enable)
    {
        enabled.store(enable);
        if (enable && !logFile.is_open())
            openLogFile();
    }
    
    bool isEnabled() const { return enabled.load(); }
    
    void setLevel(Level level) { minLevel = level; }
    
    // Logging methods
    void debug(const juce::String& message) { log(Level::Debug, message); }
    void info(const juce::String& message) { log(Level::Info, message); }
    void warning(const juce::String& message) { log(Level::Warning, message); }
    void error(const juce::String& message) { log(Level::Error, message); }
    
    // Structured logging for audio events
    void logMidiEvent(const juce::String& type, int note, float velocity)
    {
        if (!enabled.load()) return;
        log(Level::Info, "MIDI " + type + " | note=" + juce::String(note) + 
            " vel=" + juce::String(velocity, 2));
    }
    
    void logVoiceEvent(const juce::String& event, int voiceId, int note)
    {
        if (!enabled.load()) return;
        log(Level::Debug, "VOICE " + event + " | id=" + juce::String(voiceId) + 
            " note=" + juce::String(note));
    }
    
    void logParameterChange(const juce::String& param, float oldValue, float newValue)
    {
        if (!enabled.load()) return;
        log(Level::Debug, "PARAM " + param + " | " + juce::String(oldValue, 3) + 
            " -> " + juce::String(newValue, 3));
    }
    
    void logAudioStats(float peakLevel, float rmsLevel, int activeVoices, bool hasClipping)
    {
        if (!enabled.load()) return;
        
        // Solo loguear cada N samples para no saturar
        if (++audioStatsCounter < 1000) return;
        audioStatsCounter = 0;
        
        juce::String clipStr = hasClipping ? " [CLIP!]" : "";
        log(Level::Info, "AUDIO | peak=" + juce::String(peakLevel, 3) + 
            " rms=" + juce::String(rmsLevel, 3) + 
            " voices=" + juce::String(activeVoices) + clipStr);
    }
    
    void logAudioAnomaly(const juce::String& type, float value)
    {
        if (!enabled.load()) return;
        log(Level::Warning, "ANOMALY " + type + " | value=" + juce::String(value, 6));
    }
    
    void logEffectState(const juce::String& effect, bool isEnabled, float param1, float param2)
    {
        if (!enabled.load()) return;
        log(Level::Debug, "EFFECT " + effect + " | enabled=" + juce::String(isEnabled ? "ON" : "OFF") +
            " p1=" + juce::String(param1, 2) + " p2=" + juce::String(param2, 2));
    }
    
    void logEnvelopeState(const juce::String& env, const juce::String& state, float value)
    {
        if (!enabled.load()) return;
        log(Level::Debug, "ENV " + env + " | state=" + state + " value=" + juce::String(value, 4));
    }
    
    void logPresetChange(const juce::String& presetName)
    {
        if (!enabled.load()) return;
        log(Level::Info, "PRESET loaded: " + presetName);
    }
    
    void logDSPValues(float osc1, float osc2, float sub, float filterOut, float ampEnv)
    {
        if (!enabled.load()) return;
        
        // Solo loguear cada N samples
        if (++dspValuesCounter < 5000) return;
        dspValuesCounter = 0;
        
        log(Level::Debug, "DSP | osc1=" + juce::String(osc1, 3) + 
            " osc2=" + juce::String(osc2, 3) +
            " sub=" + juce::String(sub, 3) +
            " flt=" + juce::String(filterOut, 3) +
            " amp=" + juce::String(ampEnv, 3));
    }
    
    void flush()
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (logFile.is_open())
            logFile.flush();
    }
    
    ~Logger()
    {
        if (logFile.is_open())
        {
            log(Level::Info, "=== KndlSynth Session Ended ===");
            logFile.close();
        }
    }
    
private:
    Logger()
    {
        // No abrir archivo hasta que se habilite
    }
    
    void openLogFile()
    {
        auto logDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                        .getChildFile("KndlSynth")
                        .getChildFile("Logs");
        
        if (!logDir.exists())
            logDir.createDirectory();
        
        // Crear archivo con timestamp
        auto now = juce::Time::getCurrentTime();
        auto filename = "kndl_" + now.formatted("%Y%m%d_%H%M%S") + ".log";
        auto logPath = logDir.getChildFile(filename);
        
        logFile.open(logPath.getFullPathName().toStdString(), std::ios::out | std::ios::app);
        
        if (logFile.is_open())
        {
            log(Level::Info, "=== KndlSynth Session Started ===");
            log(Level::Info, "Log file: " + logPath.getFullPathName());
            log(Level::Info, "Sample rate will be logged when audio starts");
        }
    }
    
    void log(Level level, const juce::String& message)
    {
        if (!enabled.load() || level < minLevel)
            return;
        
        std::lock_guard<std::mutex> lock(mutex);
        
        if (!logFile.is_open())
            return;
        
        auto now = juce::Time::getCurrentTime();
        juce::String timestamp = now.formatted("%H:%M:%S.") + 
                                  juce::String(now.getMilliseconds()).paddedLeft('0', 3);
        
        juce::String levelStr;
        switch (level)
        {
            case Level::Debug:   levelStr = "[DBG]"; break;
            case Level::Info:    levelStr = "[INF]"; break;
            case Level::Warning: levelStr = "[WRN]"; break;
            case Level::Error:   levelStr = "[ERR]"; break;
        }
        
        logFile << timestamp.toStdString() << " " << levelStr.toStdString() 
                << " " << message.toStdString() << "\n";
        
        // Flush on warnings and errors
        if (level >= Level::Warning)
            logFile.flush();
    }
    
    std::ofstream logFile;
    std::mutex mutex;
    std::atomic<bool> enabled { false };
    Level minLevel = Level::Debug;
    int audioStatsCounter = 0;
    int dspValuesCounter = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Logger)
};

// Macros para facilitar el logging
#define KNDL_LOG_DEBUG(msg) kndl::Logger::getInstance().debug(msg)
#define KNDL_LOG_INFO(msg) kndl::Logger::getInstance().info(msg)
#define KNDL_LOG_WARNING(msg) kndl::Logger::getInstance().warning(msg)
#define KNDL_LOG_ERROR(msg) kndl::Logger::getInstance().error(msg)

} // namespace kndl
