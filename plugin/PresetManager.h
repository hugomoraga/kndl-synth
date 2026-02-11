#pragma once

#include <JuceHeader.h>

namespace kndl {

/**
 * PresetManager - Gestiona guardar, cargar y organizar presets.
 */
class PresetManager
{
public:
    explicit PresetManager(juce::AudioProcessorValueTreeState& apvts)
        : parameters(apvts)
    {
        // Crear directorio de presets si no existe
        presetDirectory = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                            .getChildFile("KndlSynth")
                            .getChildFile("Presets");
        
        if (!presetDirectory.exists())
            presetDirectory.createDirectory();
        
        // Instalar presets de fábrica si no existen
        installFactoryPresets();
        
        // Cargar lista de presets
        refreshPresetList();
    }
    
    // Guardar preset actual
    bool savePreset(const juce::String& presetName)
    {
        if (presetName.isEmpty())
            return false;
        
        auto presetFile = presetDirectory.getChildFile(presetName + ".kndl");
        
        // Crear XML del estado actual
        auto state = parameters.copyState();
        auto xml = state.createXml();
        
        if (xml != nullptr)
        {
            // Agregar metadata
            xml->setAttribute("presetName", presetName);
            xml->setAttribute("version", "1.0");
            xml->setAttribute("createdAt", juce::Time::getCurrentTime().toISO8601(true));
            
            if (xml->writeTo(presetFile))
            {
                currentPresetName = presetName;
                refreshPresetList();
                return true;
            }
        }
        
        return false;
    }
    
    // Cargar preset
    bool loadPreset(const juce::String& presetName)
    {
        auto presetFile = presetDirectory.getChildFile(presetName + ".kndl");
        
        if (!presetFile.existsAsFile())
            return false;
        
        auto xml = juce::XmlDocument::parse(presetFile);
        
        if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
        {
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
            currentPresetName = presetName;
            return true;
        }
        
        return false;
    }
    
    // Cargar preset por índice
    bool loadPresetByIndex(int index)
    {
        if (index >= 0 && index < static_cast<int>(presetList.size()))
            return loadPreset(presetList[static_cast<size_t>(index)]);
        return false;
    }
    
    // Eliminar preset
    bool deletePreset(const juce::String& presetName)
    {
        auto presetFile = presetDirectory.getChildFile(presetName + ".kndl");
        
        if (presetFile.existsAsFile())
        {
            if (presetFile.deleteFile())
            {
                if (currentPresetName == presetName)
                    currentPresetName = "Init";
                refreshPresetList();
                return true;
            }
        }
        
        return false;
    }
    
    // Inicializar (reset a valores por defecto)
    void initPreset()
    {
        // Crear un nuevo estado con valores por defecto
        auto defaultState = parameters.copyState();
        
        // Iterar por todos los parámetros y resetear a su valor por defecto
        for (auto* param : parameters.processor.getParameters())
        {
            if (auto* rangedParam = dynamic_cast<juce::RangedAudioParameter*>(param))
            {
                rangedParam->setValueNotifyingHost(rangedParam->getDefaultValue());
            }
        }
        
        currentPresetName = "Init";
    }
    
    // Navegación
    void nextPreset()
    {
        if (presetList.empty()) return;
        
        int currentIndex = getCurrentPresetIndex();
        int nextIndex = (currentIndex + 1) % static_cast<int>(presetList.size());
        loadPresetByIndex(nextIndex);
    }
    
    void previousPreset()
    {
        if (presetList.empty()) return;
        
        int currentIndex = getCurrentPresetIndex();
        int prevIndex = currentIndex - 1;
        if (prevIndex < 0) prevIndex = static_cast<int>(presetList.size()) - 1;
        loadPresetByIndex(prevIndex);
    }
    
    // Getters
    const juce::String& getCurrentPresetName() const { return currentPresetName; }
    const std::vector<juce::String>& getPresetList() const { return presetList; }
    int getPresetCount() const { return static_cast<int>(presetList.size()); }
    
    int getCurrentPresetIndex() const
    {
        for (size_t i = 0; i < presetList.size(); ++i)
        {
            if (presetList[i] == currentPresetName)
                return static_cast<int>(i);
        }
        return -1;
    }
    
    const juce::File& getPresetDirectory() const { return presetDirectory; }
    
    // Refrescar lista de presets
    void refreshPresetList()
    {
        presetList.clear();
        
        auto files = presetDirectory.findChildFiles(juce::File::findFiles, false, "*.kndl");
        files.sort();
        
        for (const auto& file : files)
        {
            presetList.push_back(file.getFileNameWithoutExtension());
        }
    }
    
    // Exportar/Importar para compartir
    bool exportPreset(const juce::String& presetName, const juce::File& destination)
    {
        auto presetFile = presetDirectory.getChildFile(presetName + ".kndl");
        if (presetFile.existsAsFile())
            return presetFile.copyFileTo(destination);
        return false;
    }
    
    bool importPreset(const juce::File& source)
    {
        if (!source.existsAsFile() || source.getFileExtension() != ".kndl")
            return false;
        
        auto destination = presetDirectory.getChildFile(source.getFileName());
        if (source.copyFileTo(destination))
        {
            refreshPresetList();
            return true;
        }
        return false;
    }
    
private:
    void installFactoryPresets()
    {
        // Lista de presets de fábrica embebidos
        struct FactoryPreset {
            const char* name;
            const char* xml;
        };
        
        static const FactoryPreset factoryPresets[] = {
            { "Init", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Init" version="1.0" category="Basic">
  <PARAM id="osc1_waveform" value="2"/><PARAM id="osc1_level" value="0.8"/><PARAM id="osc1_detune" value="0"/><PARAM id="osc1_octave" value="0"/>
  <PARAM id="osc2_waveform" value="2"/><PARAM id="osc2_level" value="0"/><PARAM id="osc2_detune" value="0"/><PARAM id="osc2_octave" value="0"/>
  <PARAM id="sub_level" value="0"/><PARAM id="sub_octave" value="-1"/>
  <PARAM id="filter_cutoff" value="8000"/><PARAM id="filter_resonance" value="0"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0"/><PARAM id="filter_env_amount" value="0"/>
  <PARAM id="amp_attack" value="0.01"/><PARAM id="amp_decay" value="0.1"/><PARAM id="amp_sustain" value="0.8"/><PARAM id="amp_release" value="0.3"/>
  <PARAM id="filter_attack" value="0.01"/><PARAM id="filter_decay" value="0.3"/><PARAM id="filter_sustain" value="0.5"/><PARAM id="filter_release" value="0.5"/>
  <PARAM id="lfo1_rate" value="1"/><PARAM id="lfo2_rate" value="1"/><PARAM id="master_gain" value="-6"/>
  <PARAM id="dist_enable" value="0"/><PARAM id="chorus_enable" value="0"/><PARAM id="delay_enable" value="0"/><PARAM id="reverb_enable" value="0"/>
</Parameters>)" },
            
            { "Deep Bass", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Deep Bass" version="1.0" category="Bass">
  <PARAM id="osc1_waveform" value="2"/><PARAM id="osc1_level" value="0.7"/><PARAM id="osc1_detune" value="0"/><PARAM id="osc1_octave" value="-1"/>
  <PARAM id="osc2_waveform" value="3"/><PARAM id="osc2_level" value="0.4"/><PARAM id="osc2_detune" value="5"/><PARAM id="osc2_octave" value="-1"/>
  <PARAM id="sub_level" value="0.6"/><PARAM id="sub_octave" value="-2"/>
  <PARAM id="filter_cutoff" value="800"/><PARAM id="filter_resonance" value="0.3"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.2"/><PARAM id="filter_env_amount" value="0.4"/>
  <PARAM id="amp_attack" value="0.005"/><PARAM id="amp_decay" value="0.2"/><PARAM id="amp_sustain" value="0.7"/><PARAM id="amp_release" value="0.15"/>
  <PARAM id="filter_attack" value="0.01"/><PARAM id="filter_decay" value="0.25"/><PARAM id="filter_sustain" value="0.3"/><PARAM id="filter_release" value="0.2"/>
  <PARAM id="lfo1_rate" value="0.5"/><PARAM id="lfo2_rate" value="1"/><PARAM id="master_gain" value="-3"/>
  <PARAM id="dist_enable" value="1"/><PARAM id="dist_drive" value="3"/><PARAM id="dist_mix" value="0.3"/>
  <PARAM id="chorus_enable" value="0"/><PARAM id="delay_enable" value="0"/><PARAM id="reverb_enable" value="0"/>
</Parameters>)" },
            
            { "Hypnotic Pad", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Hypnotic Pad" version="1.0" category="Pad">
  <PARAM id="osc1_waveform" value="2"/><PARAM id="osc1_level" value="0.5"/><PARAM id="osc1_detune" value="-7"/><PARAM id="osc1_octave" value="0"/>
  <PARAM id="osc2_waveform" value="2"/><PARAM id="osc2_level" value="0.5"/><PARAM id="osc2_detune" value="7"/><PARAM id="osc2_octave" value="0"/>
  <PARAM id="sub_level" value="0.3"/><PARAM id="sub_octave" value="-1"/>
  <PARAM id="filter_cutoff" value="3000"/><PARAM id="filter_resonance" value="0.2"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0"/><PARAM id="filter_env_amount" value="0.2"/>
  <PARAM id="amp_attack" value="0.8"/><PARAM id="amp_decay" value="0.5"/><PARAM id="amp_sustain" value="0.8"/><PARAM id="amp_release" value="1.5"/>
  <PARAM id="filter_attack" value="1.0"/><PARAM id="filter_decay" value="0.8"/><PARAM id="filter_sustain" value="0.6"/><PARAM id="filter_release" value="1.2"/>
  <PARAM id="lfo1_rate" value="0.3"/><PARAM id="lfo2_rate" value="0.15"/><PARAM id="master_gain" value="-6"/>
  <PARAM id="dist_enable" value="0"/><PARAM id="chorus_enable" value="1"/><PARAM id="chorus_rate" value="0.8"/><PARAM id="chorus_depth" value="0.6"/><PARAM id="chorus_mix" value="0.4"/>
  <PARAM id="delay_enable" value="1"/><PARAM id="delay_time" value="400"/><PARAM id="delay_feedback" value="0.4"/><PARAM id="delay_mix" value="0.25"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.8"/><PARAM id="reverb_damp" value="0.4"/><PARAM id="reverb_mix" value="0.4"/>
</Parameters>)" },
            
            { "Acid Lead", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Acid Lead" version="1.0" category="Lead">
  <PARAM id="osc1_waveform" value="2"/><PARAM id="osc1_level" value="0.9"/><PARAM id="osc1_detune" value="0"/><PARAM id="osc1_octave" value="0"/>
  <PARAM id="osc2_waveform" value="3"/><PARAM id="osc2_level" value="0.3"/><PARAM id="osc2_detune" value="12"/><PARAM id="osc2_octave" value="1"/>
  <PARAM id="sub_level" value="0"/><PARAM id="sub_octave" value="-1"/>
  <PARAM id="filter_cutoff" value="500"/><PARAM id="filter_resonance" value="0.7"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.4"/><PARAM id="filter_env_amount" value="0.8"/>
  <PARAM id="amp_attack" value="0.001"/><PARAM id="amp_decay" value="0.15"/><PARAM id="amp_sustain" value="0.6"/><PARAM id="amp_release" value="0.1"/>
  <PARAM id="filter_attack" value="0.001"/><PARAM id="filter_decay" value="0.2"/><PARAM id="filter_sustain" value="0.2"/><PARAM id="filter_release" value="0.15"/>
  <PARAM id="lfo1_rate" value="6"/><PARAM id="lfo2_rate" value="1"/><PARAM id="master_gain" value="-6"/>
  <PARAM id="dist_enable" value="1"/><PARAM id="dist_drive" value="5"/><PARAM id="dist_mix" value="0.5"/>
  <PARAM id="chorus_enable" value="0"/><PARAM id="delay_enable" value="1"/><PARAM id="delay_time" value="180"/><PARAM id="delay_feedback" value="0.35"/><PARAM id="delay_mix" value="0.2"/>
  <PARAM id="reverb_enable" value="0"/>
</Parameters>)" },
            
            { "Reese Bass", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Reese Bass" version="1.0" category="Bass">
  <PARAM id="osc1_waveform" value="2"/><PARAM id="osc1_level" value="0.7"/><PARAM id="osc1_detune" value="-15"/><PARAM id="osc1_octave" value="-1"/>
  <PARAM id="osc2_waveform" value="2"/><PARAM id="osc2_level" value="0.7"/><PARAM id="osc2_detune" value="15"/><PARAM id="osc2_octave" value="-1"/>
  <PARAM id="sub_level" value="0.5"/><PARAM id="sub_octave" value="-2"/>
  <PARAM id="filter_cutoff" value="1200"/><PARAM id="filter_resonance" value="0.25"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.35"/><PARAM id="filter_env_amount" value="0.3"/>
  <PARAM id="amp_attack" value="0.01"/><PARAM id="amp_decay" value="0.3"/><PARAM id="amp_sustain" value="0.8"/><PARAM id="amp_release" value="0.2"/>
  <PARAM id="filter_attack" value="0.01"/><PARAM id="filter_decay" value="0.4"/><PARAM id="filter_sustain" value="0.4"/><PARAM id="filter_release" value="0.3"/>
  <PARAM id="lfo1_rate" value="3"/><PARAM id="lfo2_rate" value="0.5"/><PARAM id="master_gain" value="-3"/>
  <PARAM id="dist_enable" value="1"/><PARAM id="dist_drive" value="6"/><PARAM id="dist_mix" value="0.35"/>
  <PARAM id="chorus_enable" value="0"/><PARAM id="delay_enable" value="0"/><PARAM id="reverb_enable" value="0"/>
</Parameters>)" },
            
            { "Techno Stab", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Techno Stab" version="1.0" category="Synth">
  <PARAM id="osc1_waveform" value="2"/><PARAM id="osc1_level" value="0.8"/><PARAM id="osc1_detune" value="0"/><PARAM id="osc1_octave" value="0"/>
  <PARAM id="osc2_waveform" value="2"/><PARAM id="osc2_level" value="0.6"/><PARAM id="osc2_detune" value="-10"/><PARAM id="osc2_octave" value="0"/>
  <PARAM id="sub_level" value="0.4"/><PARAM id="sub_octave" value="-1"/>
  <PARAM id="filter_cutoff" value="2000"/><PARAM id="filter_resonance" value="0.5"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.3"/><PARAM id="filter_env_amount" value="0.6"/>
  <PARAM id="amp_attack" value="0.001"/><PARAM id="amp_decay" value="0.08"/><PARAM id="amp_sustain" value="0"/><PARAM id="amp_release" value="0.05"/>
  <PARAM id="filter_attack" value="0.001"/><PARAM id="filter_decay" value="0.1"/><PARAM id="filter_sustain" value="0"/><PARAM id="filter_release" value="0.08"/>
  <PARAM id="lfo1_rate" value="4"/><PARAM id="lfo2_rate" value="1"/><PARAM id="master_gain" value="-3"/>
  <PARAM id="dist_enable" value="1"/><PARAM id="dist_drive" value="4"/><PARAM id="dist_mix" value="0.4"/>
  <PARAM id="chorus_enable" value="0"/><PARAM id="delay_enable" value="0"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.4"/><PARAM id="reverb_damp" value="0.6"/><PARAM id="reverb_mix" value="0.2"/>
</Parameters>)" },
            
            { "Dark Atmosphere", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Dark Atmosphere" version="1.0" category="Ambient">
  <PARAM id="osc1_waveform" value="1"/><PARAM id="osc1_level" value="0.4"/><PARAM id="osc1_detune" value="-5"/><PARAM id="osc1_octave" value="-1"/>
  <PARAM id="osc2_waveform" value="2"/><PARAM id="osc2_level" value="0.3"/><PARAM id="osc2_detune" value="5"/><PARAM id="osc2_octave" value="0"/>
  <PARAM id="sub_level" value="0.6"/><PARAM id="sub_octave" value="-2"/>
  <PARAM id="filter_cutoff" value="600"/><PARAM id="filter_resonance" value="0.3"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.15"/><PARAM id="filter_env_amount" value="0.3"/>
  <PARAM id="amp_attack" value="1.5"/><PARAM id="amp_decay" value="0.8"/><PARAM id="amp_sustain" value="0.6"/><PARAM id="amp_release" value="2.5"/>
  <PARAM id="filter_attack" value="2.0"/><PARAM id="filter_decay" value="1.5"/><PARAM id="filter_sustain" value="0.4"/><PARAM id="filter_release" value="3.0"/>
  <PARAM id="lfo1_rate" value="0.2"/><PARAM id="lfo2_rate" value="0.08"/><PARAM id="master_gain" value="-9"/>
  <PARAM id="dist_enable" value="0"/>
  <PARAM id="chorus_enable" value="1"/><PARAM id="chorus_rate" value="0.15"/><PARAM id="chorus_depth" value="0.7"/><PARAM id="chorus_mix" value="0.45"/>
  <PARAM id="delay_enable" value="1"/><PARAM id="delay_time" value="500"/><PARAM id="delay_feedback" value="0.5"/><PARAM id="delay_mix" value="0.3"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.9"/><PARAM id="reverb_damp" value="0.6"/><PARAM id="reverb_mix" value="0.55"/>
</Parameters>)" },
            
            { "Psychedelic Drone", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Psychedelic Drone" version="1.0" category="Ambient">
  <PARAM id="osc1_waveform" value="0"/><PARAM id="osc1_level" value="0.6"/><PARAM id="osc1_detune" value="-3"/><PARAM id="osc1_octave" value="-1"/>
  <PARAM id="osc2_waveform" value="0"/><PARAM id="osc2_level" value="0.6"/><PARAM id="osc2_detune" value="3"/><PARAM id="osc2_octave" value="-1"/>
  <PARAM id="sub_level" value="0.5"/><PARAM id="sub_octave" value="-2"/>
  <PARAM id="filter_cutoff" value="1500"/><PARAM id="filter_resonance" value="0.4"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.1"/><PARAM id="filter_env_amount" value="0.1"/>
  <PARAM id="amp_attack" value="2.0"/><PARAM id="amp_decay" value="1.0"/><PARAM id="amp_sustain" value="1.0"/><PARAM id="amp_release" value="3.0"/>
  <PARAM id="filter_attack" value="3.0"/><PARAM id="filter_decay" value="2.0"/><PARAM id="filter_sustain" value="0.7"/><PARAM id="filter_release" value="4.0"/>
  <PARAM id="lfo1_rate" value="0.1"/><PARAM id="lfo2_rate" value="0.07"/><PARAM id="master_gain" value="-9"/>
  <PARAM id="dist_enable" value="0"/>
  <PARAM id="chorus_enable" value="1"/><PARAM id="chorus_rate" value="0.2"/><PARAM id="chorus_depth" value="0.8"/><PARAM id="chorus_mix" value="0.5"/>
  <PARAM id="delay_enable" value="1"/><PARAM id="delay_time" value="700"/><PARAM id="delay_feedback" value="0.6"/><PARAM id="delay_mix" value="0.35"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.95"/><PARAM id="reverb_damp" value="0.3"/><PARAM id="reverb_mix" value="0.6"/>
</Parameters>)" },

            { "Formant Choir", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Formant Choir" version="1.0" category="Pad">
  <PARAM id="osc1_waveform" value="2"/><PARAM id="osc1_level" value="0.7"/><PARAM id="osc1_detune" value="-5"/><PARAM id="osc1_octave" value="0"/>
  <PARAM id="osc2_waveform" value="2"/><PARAM id="osc2_level" value="0.7"/><PARAM id="osc2_detune" value="5"/><PARAM id="osc2_octave" value="0"/>
  <PARAM id="sub_level" value="0.3"/><PARAM id="sub_octave" value="-1"/>
  <PARAM id="filter_cutoff" value="2200"/><PARAM id="filter_resonance" value="0.55"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.05"/><PARAM id="filter_env_amount" value="0.2"/>
  <PARAM id="filter_mode" value="1"/><PARAM id="formant_vowel" value="0"/>
  <PARAM id="amp_attack" value="0.8"/><PARAM id="amp_decay" value="0.5"/><PARAM id="amp_sustain" value="0.9"/><PARAM id="amp_release" value="1.5"/>
  <PARAM id="filter_attack" value="1.2"/><PARAM id="filter_decay" value="0.8"/><PARAM id="filter_sustain" value="0.6"/><PARAM id="filter_release" value="2.0"/>
  <PARAM id="lfo1_rate" value="0.15"/><PARAM id="lfo1_waveform" value="1"/><PARAM id="lfo2_rate" value="0.08"/><PARAM id="lfo2_waveform" value="0"/>
  <PARAM id="orbit_shape" value="0"/><PARAM id="orbit_rate" value="0.25"/>
  <PARAM id="mod_1_src" value="1"/><PARAM id="mod_1_dst" value="6"/><PARAM id="mod_1_amt" value="0.35"/>
  <PARAM id="mod_2_src" value="8"/><PARAM id="mod_2_dst" value="6"/><PARAM id="mod_2_amt" value="0.2"/>
  <PARAM id="master_gain" value="-4"/>
  <PARAM id="dist_enable" value="0"/><PARAM id="chorus_enable" value="1"/><PARAM id="chorus_rate" value="0.3"/><PARAM id="chorus_depth" value="0.6"/><PARAM id="chorus_mix" value="0.45"/>
  <PARAM id="delay_enable" value="0"/><PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.85"/><PARAM id="reverb_damp" value="0.4"/><PARAM id="reverb_mix" value="0.5"/>
</Parameters>)" },

            { "Comb Pluck", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Comb Pluck" version="1.0" category="Pluck">
  <PARAM id="osc1_waveform" value="3"/><PARAM id="osc1_level" value="0.9"/><PARAM id="osc1_detune" value="0"/><PARAM id="osc1_octave" value="0"/>
  <PARAM id="osc2_waveform" value="1"/><PARAM id="osc2_level" value="0.4"/><PARAM id="osc2_detune" value="7"/><PARAM id="osc2_octave" value="1"/>
  <PARAM id="sub_level" value="0"/><PARAM id="sub_octave" value="-1"/>
  <PARAM id="filter_cutoff" value="3500"/><PARAM id="filter_resonance" value="0.6"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.1"/><PARAM id="filter_env_amount" value="0.7"/>
  <PARAM id="filter_mode" value="2"/>
  <PARAM id="amp_attack" value="0.001"/><PARAM id="amp_decay" value="0.4"/><PARAM id="amp_sustain" value="0.0"/><PARAM id="amp_release" value="0.3"/>
  <PARAM id="filter_attack" value="0.001"/><PARAM id="filter_decay" value="0.35"/><PARAM id="filter_sustain" value="0.1"/><PARAM id="filter_release" value="0.2"/>
  <PARAM id="lfo1_rate" value="3.5"/><PARAM id="lfo2_rate" value="0.5"/><PARAM id="lfo2_waveform" value="2"/><PARAM id="master_gain" value="-3"/>
  <PARAM id="dist_enable" value="0"/><PARAM id="chorus_enable" value="0"/>
  <PARAM id="delay_enable" value="1"/><PARAM id="delay_time" value="330"/><PARAM id="delay_feedback" value="0.45"/><PARAM id="delay_mix" value="0.3"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.6"/><PARAM id="reverb_damp" value="0.5"/><PARAM id="reverb_mix" value="0.25"/>
</Parameters>)" },

            { "Orbit Pad", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Orbit Pad" version="1.0" category="Pad">
  <PARAM id="osc1_waveform" value="0"/><PARAM id="osc1_level" value="0.6"/><PARAM id="osc1_detune" value="-7"/><PARAM id="osc1_octave" value="0"/>
  <PARAM id="osc2_waveform" value="1"/><PARAM id="osc2_level" value="0.5"/><PARAM id="osc2_detune" value="7"/><PARAM id="osc2_octave" value="0"/>
  <PARAM id="sub_level" value="0.4"/><PARAM id="sub_octave" value="-1"/>
  <PARAM id="filter_cutoff" value="3000"/><PARAM id="filter_resonance" value="0.3"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0"/><PARAM id="filter_env_amount" value="0.15"/>
  <PARAM id="amp_attack" value="1.5"/><PARAM id="amp_decay" value="1.0"/><PARAM id="amp_sustain" value="0.85"/><PARAM id="amp_release" value="3.0"/>
  <PARAM id="filter_attack" value="2.0"/><PARAM id="filter_decay" value="1.5"/><PARAM id="filter_sustain" value="0.5"/><PARAM id="filter_release" value="3.5"/>
  <PARAM id="lfo1_rate" value="0.2"/><PARAM id="lfo2_rate" value="0.35"/><PARAM id="lfo2_waveform" value="1"/>
  <PARAM id="orbit_shape" value="4"/><PARAM id="orbit_rate" value="0.12"/>
  <PARAM id="mod_1_src" value="8"/><PARAM id="mod_1_dst" value="6"/><PARAM id="mod_1_amt" value="0.4"/>
  <PARAM id="mod_2_src" value="9"/><PARAM id="mod_2_dst" value="1"/><PARAM id="mod_2_amt" value="0.15"/>
  <PARAM id="mod_3_src" value="10"/><PARAM id="mod_3_dst" value="4"/><PARAM id="mod_3_amt" value="0.25"/>
  <PARAM id="mod_4_src" value="11"/><PARAM id="mod_4_dst" value="5"/><PARAM id="mod_4_amt" value="0.2"/>
  <PARAM id="master_gain" value="-5"/>
  <PARAM id="dist_enable" value="0"/><PARAM id="chorus_enable" value="1"/><PARAM id="chorus_rate" value="0.15"/><PARAM id="chorus_depth" value="0.7"/><PARAM id="chorus_mix" value="0.5"/>
  <PARAM id="delay_enable" value="1"/><PARAM id="delay_time" value="500"/><PARAM id="delay_feedback" value="0.5"/><PARAM id="delay_mix" value="0.3"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.9"/><PARAM id="reverb_damp" value="0.35"/><PARAM id="reverb_mix" value="0.55"/>
</Parameters>)" },

            { "Notch Sweep", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Notch Sweep" version="1.0" category="Lead">
  <PARAM id="osc1_waveform" value="2"/><PARAM id="osc1_level" value="0.85"/><PARAM id="osc1_detune" value="0"/><PARAM id="osc1_octave" value="0"/>
  <PARAM id="osc2_waveform" value="2"/><PARAM id="osc2_level" value="0.5"/><PARAM id="osc2_detune" value="15"/><PARAM id="osc2_octave" value="0"/>
  <PARAM id="sub_level" value="0.2"/><PARAM id="sub_octave" value="-1"/>
  <PARAM id="filter_cutoff" value="1800"/><PARAM id="filter_resonance" value="0.7"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.15"/><PARAM id="filter_env_amount" value="0.5"/>
  <PARAM id="filter_mode" value="3"/>
  <PARAM id="amp_attack" value="0.01"/><PARAM id="amp_decay" value="0.3"/><PARAM id="amp_sustain" value="0.7"/><PARAM id="amp_release" value="0.4"/>
  <PARAM id="filter_attack" value="0.01"/><PARAM id="filter_decay" value="0.6"/><PARAM id="filter_sustain" value="0.3"/><PARAM id="filter_release" value="0.5"/>
  <PARAM id="lfo1_rate" value="0.8"/><PARAM id="lfo1_waveform" value="1"/><PARAM id="lfo2_rate" value="4.0"/><PARAM id="master_gain" value="-3"/>
  <PARAM id="mod_1_src" value="1"/><PARAM id="mod_1_dst" value="6"/><PARAM id="mod_1_amt" value="0.5"/>
  <PARAM id="mod_2_src" value="2"/><PARAM id="mod_2_dst" value="7"/><PARAM id="mod_2_amt" value="0.3"/>
  <PARAM id="dist_enable" value="1"/><PARAM id="dist_drive" value="2.5"/><PARAM id="dist_mix" value="0.35"/>
  <PARAM id="chorus_enable" value="0"/>
  <PARAM id="delay_enable" value="1"/><PARAM id="delay_time" value="375"/><PARAM id="delay_feedback" value="0.4"/><PARAM id="delay_mix" value="0.25"/>
  <PARAM id="reverb_enable" value="0"/>
</Parameters>)" },

            { "Vowel Bass", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Vowel Bass" version="1.0" category="Bass">
  <PARAM id="osc1_waveform" value="2"/><PARAM id="osc1_level" value="0.9"/><PARAM id="osc1_detune" value="0"/><PARAM id="osc1_octave" value="-1"/>
  <PARAM id="osc2_waveform" value="3"/><PARAM id="osc2_level" value="0.6"/><PARAM id="osc2_detune" value="5"/><PARAM id="osc2_octave" value="-1"/>
  <PARAM id="sub_level" value="0.7"/><PARAM id="sub_octave" value="-2"/>
  <PARAM id="filter_cutoff" value="1600"/><PARAM id="filter_resonance" value="0.65"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.3"/><PARAM id="filter_env_amount" value="0.6"/>
  <PARAM id="filter_mode" value="1"/><PARAM id="formant_vowel" value="3"/>
  <PARAM id="amp_attack" value="0.005"/><PARAM id="amp_decay" value="0.2"/><PARAM id="amp_sustain" value="0.75"/><PARAM id="amp_release" value="0.15"/>
  <PARAM id="filter_attack" value="0.005"/><PARAM id="filter_decay" value="0.25"/><PARAM id="filter_sustain" value="0.35"/><PARAM id="filter_release" value="0.2"/>
  <PARAM id="lfo1_rate" value="2.0"/><PARAM id="lfo1_waveform" value="3"/><PARAM id="lfo2_rate" value="0.5"/><PARAM id="master_gain" value="-2"/>
  <PARAM id="mod_1_src" value="4"/><PARAM id="mod_1_dst" value="6"/><PARAM id="mod_1_amt" value="0.3"/>
  <PARAM id="dist_enable" value="1"/><PARAM id="dist_drive" value="3.0"/><PARAM id="dist_mix" value="0.3"/>
  <PARAM id="chorus_enable" value="0"/><PARAM id="delay_enable" value="0"/><PARAM id="reverb_enable" value="0"/>
</Parameters>)" },

            { "Spiral Texture", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Spiral Texture" version="1.0" category="Ambient">
  <PARAM id="osc1_waveform" value="0"/><PARAM id="osc1_level" value="0.5"/><PARAM id="osc1_detune" value="-12"/><PARAM id="osc1_octave" value="0"/>
  <PARAM id="osc2_waveform" value="0"/><PARAM id="osc2_level" value="0.5"/><PARAM id="osc2_detune" value="12"/><PARAM id="osc2_octave" value="1"/>
  <PARAM id="sub_level" value="0.3"/><PARAM id="sub_octave" value="-2"/>
  <PARAM id="filter_cutoff" value="4000"/><PARAM id="filter_resonance" value="0.2"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0"/><PARAM id="filter_env_amount" value="0.05"/>
  <PARAM id="amp_attack" value="3.0"/><PARAM id="amp_decay" value="2.0"/><PARAM id="amp_sustain" value="0.9"/><PARAM id="amp_release" value="5.0"/>
  <PARAM id="filter_attack" value="4.0"/><PARAM id="filter_decay" value="3.0"/><PARAM id="filter_sustain" value="0.6"/><PARAM id="filter_release" value="5.0"/>
  <PARAM id="lfo1_rate" value="0.05"/><PARAM id="lfo2_rate" value="0.12"/><PARAM id="lfo2_waveform" value="1"/>
  <PARAM id="orbit_shape" value="5"/><PARAM id="orbit_rate" value="0.07"/>
  <PARAM id="mod_1_src" value="8"/><PARAM id="mod_1_dst" value="1"/><PARAM id="mod_1_amt" value="0.2"/>
  <PARAM id="mod_2_src" value="9"/><PARAM id="mod_2_dst" value="2"/><PARAM id="mod_2_amt" value="0.2"/>
  <PARAM id="mod_3_src" value="10"/><PARAM id="mod_3_dst" value="6"/><PARAM id="mod_3_amt" value="0.3"/>
  <PARAM id="mod_4_src" value="11"/><PARAM id="mod_4_dst" value="8"/><PARAM id="mod_4_amt" value="0.15"/>
  <PARAM id="mod_5_src" value="1"/><PARAM id="mod_5_dst" value="3"/><PARAM id="mod_5_amt" value="0.2"/>
  <PARAM id="mod_6_src" value="2"/><PARAM id="mod_6_dst" value="4"/><PARAM id="mod_6_amt" value="-0.2"/>
  <PARAM id="master_gain" value="-6"/>
  <PARAM id="dist_enable" value="0"/><PARAM id="chorus_enable" value="1"/><PARAM id="chorus_rate" value="0.1"/><PARAM id="chorus_depth" value="0.9"/><PARAM id="chorus_mix" value="0.55"/>
  <PARAM id="delay_enable" value="1"/><PARAM id="delay_time" value="800"/><PARAM id="delay_feedback" value="0.65"/><PARAM id="delay_mix" value="0.4"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.98"/><PARAM id="reverb_damp" value="0.2"/><PARAM id="reverb_mix" value="0.65"/>
</Parameters>)" },

            { "OTT Supersaw", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="OTT Supersaw" version="1.0" category="Lead">
  <PARAM id="osc1_waveform" value="2"/><PARAM id="osc1_level" value="0.8"/><PARAM id="osc1_detune" value="-20"/><PARAM id="osc1_octave" value="0"/>
  <PARAM id="osc2_waveform" value="2"/><PARAM id="osc2_level" value="0.8"/><PARAM id="osc2_detune" value="20"/><PARAM id="osc2_octave" value="0"/>
  <PARAM id="sub_level" value="0.5"/><PARAM id="sub_octave" value="-1"/>
  <PARAM id="filter_cutoff" value="6000"/><PARAM id="filter_resonance" value="0.15"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.1"/><PARAM id="filter_env_amount" value="0.3"/>
  <PARAM id="amp_attack" value="0.01"/><PARAM id="amp_decay" value="0.15"/><PARAM id="amp_sustain" value="0.85"/><PARAM id="amp_release" value="0.25"/>
  <PARAM id="filter_attack" value="0.01"/><PARAM id="filter_decay" value="0.2"/><PARAM id="filter_sustain" value="0.6"/><PARAM id="filter_release" value="0.3"/>
  <PARAM id="lfo1_rate" value="5.0"/><PARAM id="lfo2_rate" value="0.3"/><PARAM id="lfo2_waveform" value="2"/><PARAM id="master_gain" value="-3"/>
  <PARAM id="mod_1_src" value="2"/><PARAM id="mod_1_dst" value="6"/><PARAM id="mod_1_amt" value="0.15"/>
  <PARAM id="dist_enable" value="0"/><PARAM id="chorus_enable" value="1"/><PARAM id="chorus_rate" value="0.8"/><PARAM id="chorus_depth" value="0.5"/><PARAM id="chorus_mix" value="0.4"/>
  <PARAM id="delay_enable" value="0"/><PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.6"/><PARAM id="reverb_damp" value="0.5"/><PARAM id="reverb_mix" value="0.3"/>
  <PARAM id="ott_enable" value="1"/><PARAM id="ott_depth" value="0.7"/><PARAM id="ott_time" value="0.3"/><PARAM id="ott_mix" value="0.6"/>
</Parameters>)" },

            { "Square Wobble", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Square Wobble" version="1.0" category="Bass">
  <PARAM id="osc1_waveform" value="3"/><PARAM id="osc1_level" value="0.85"/><PARAM id="osc1_detune" value="0"/><PARAM id="osc1_octave" value="-1"/>
  <PARAM id="osc2_waveform" value="2"/><PARAM id="osc2_level" value="0.5"/><PARAM id="osc2_detune" value="10"/><PARAM id="osc2_octave" value="0"/>
  <PARAM id="sub_level" value="0.6"/><PARAM id="sub_octave" value="-2"/>
  <PARAM id="filter_cutoff" value="800"/><PARAM id="filter_resonance" value="0.5"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.4"/><PARAM id="filter_env_amount" value="0.7"/>
  <PARAM id="amp_attack" value="0.005"/><PARAM id="amp_decay" value="0.2"/><PARAM id="amp_sustain" value="0.8"/><PARAM id="amp_release" value="0.15"/>
  <PARAM id="filter_attack" value="0.005"/><PARAM id="filter_decay" value="0.3"/><PARAM id="filter_sustain" value="0.2"/><PARAM id="filter_release" value="0.2"/>
  <PARAM id="lfo1_rate" value="4.0"/><PARAM id="lfo1_waveform" value="3"/><PARAM id="lfo2_rate" value="0.25"/><PARAM id="lfo2_waveform" value="2"/>
  <PARAM id="orbit_shape" value="2"/><PARAM id="orbit_rate" value="2.0"/>
  <PARAM id="mod_1_src" value="1"/><PARAM id="mod_1_dst" value="6"/><PARAM id="mod_1_amt" value="0.6"/>
  <PARAM id="mod_2_src" value="2"/><PARAM id="mod_2_dst" value="9"/><PARAM id="mod_2_amt" value="0.4"/>
  <PARAM id="mod_3_src" value="8"/><PARAM id="mod_3_dst" value="7"/><PARAM id="mod_3_amt" value="0.25"/>
  <PARAM id="master_gain" value="-2"/>
  <PARAM id="dist_enable" value="1"/><PARAM id="dist_drive" value="4.0"/><PARAM id="dist_mix" value="0.4"/>
  <PARAM id="chorus_enable" value="0"/><PARAM id="delay_enable" value="0"/><PARAM id="reverb_enable" value="0"/>
  <PARAM id="ott_enable" value="1"/><PARAM id="ott_depth" value="0.5"/><PARAM id="ott_time" value="0.4"/><PARAM id="ott_mix" value="0.45"/>
</Parameters>)" },

            { "Lemniscate Keys", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Lemniscate Keys" version="1.0" category="Keys">
  <PARAM id="osc1_waveform" value="1"/><PARAM id="osc1_level" value="0.7"/><PARAM id="osc1_detune" value="0"/><PARAM id="osc1_octave" value="0"/>
  <PARAM id="osc2_waveform" value="0"/><PARAM id="osc2_level" value="0.45"/><PARAM id="osc2_detune" value="3"/><PARAM id="osc2_octave" value="1"/>
  <PARAM id="sub_level" value="0.15"/><PARAM id="sub_octave" value="-1"/>
  <PARAM id="filter_cutoff" value="4500"/><PARAM id="filter_resonance" value="0.2"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0"/><PARAM id="filter_env_amount" value="0.4"/>
  <PARAM id="amp_attack" value="0.01"/><PARAM id="amp_decay" value="0.8"/><PARAM id="amp_sustain" value="0.5"/><PARAM id="amp_release" value="0.6"/>
  <PARAM id="filter_attack" value="0.01"/><PARAM id="filter_decay" value="0.5"/><PARAM id="filter_sustain" value="0.3"/><PARAM id="filter_release" value="0.5"/>
  <PARAM id="lfo1_rate" value="3.0"/><PARAM id="lfo2_rate" value="0.6"/><PARAM id="lfo2_waveform" value="1"/>
  <PARAM id="orbit_shape" value="6"/><PARAM id="orbit_rate" value="0.3"/>
  <PARAM id="mod_1_src" value="1"/><PARAM id="mod_1_dst" value="1"/><PARAM id="mod_1_amt" value="0.08"/>
  <PARAM id="mod_2_src" value="8"/><PARAM id="mod_2_dst" value="6"/><PARAM id="mod_2_amt" value="0.2"/>
  <PARAM id="mod_3_src" value="9"/><PARAM id="mod_3_dst" value="3"/><PARAM id="mod_3_amt" value="0.15"/>
  <PARAM id="master_gain" value="-4"/>
  <PARAM id="dist_enable" value="0"/><PARAM id="chorus_enable" value="1"/><PARAM id="chorus_rate" value="0.5"/><PARAM id="chorus_depth" value="0.4"/><PARAM id="chorus_mix" value="0.35"/>
  <PARAM id="delay_enable" value="1"/><PARAM id="delay_time" value="440"/><PARAM id="delay_feedback" value="0.35"/><PARAM id="delay_mix" value="0.2"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.7"/><PARAM id="reverb_damp" value="0.45"/><PARAM id="reverb_mix" value="0.35"/>
</Parameters>)" },

            { "Metallic Ring", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Metallic Ring" version="1.0" category="FX">
  <PARAM id="osc1_waveform" value="3"/><PARAM id="osc1_level" value="0.7"/><PARAM id="osc1_detune" value="0"/><PARAM id="osc1_octave" value="1"/>
  <PARAM id="osc2_waveform" value="1"/><PARAM id="osc2_level" value="0.6"/><PARAM id="osc2_detune" value="50"/><PARAM id="osc2_octave" value="1"/>
  <PARAM id="sub_level" value="0"/><PARAM id="sub_octave" value="-1"/>
  <PARAM id="filter_cutoff" value="5000"/><PARAM id="filter_resonance" value="0.85"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.2"/><PARAM id="filter_env_amount" value="0.6"/>
  <PARAM id="filter_mode" value="2"/>
  <PARAM id="amp_attack" value="0.001"/><PARAM id="amp_decay" value="1.5"/><PARAM id="amp_sustain" value="0.0"/><PARAM id="amp_release" value="1.0"/>
  <PARAM id="filter_attack" value="0.001"/><PARAM id="filter_decay" value="1.0"/><PARAM id="filter_sustain" value="0.1"/><PARAM id="filter_release" value="0.8"/>
  <PARAM id="lfo1_rate" value="8.0"/><PARAM id="lfo2_rate" value="0.15"/><PARAM id="lfo2_waveform" value="1"/>
  <PARAM id="orbit_shape" value="3"/><PARAM id="orbit_rate" value="1.5"/>
  <PARAM id="mod_1_src" value="1"/><PARAM id="mod_1_dst" value="2"/><PARAM id="mod_1_amt" value="0.12"/>
  <PARAM id="mod_2_src" value="8"/><PARAM id="mod_2_dst" value="6"/><PARAM id="mod_2_amt" value="0.35"/>
  <PARAM id="master_gain" value="-5"/>
  <PARAM id="dist_enable" value="0"/><PARAM id="chorus_enable" value="0"/>
  <PARAM id="delay_enable" value="1"/><PARAM id="delay_time" value="280"/><PARAM id="delay_feedback" value="0.55"/><PARAM id="delay_mix" value="0.35"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.8"/><PARAM id="reverb_damp" value="0.3"/><PARAM id="reverb_mix" value="0.45"/>
</Parameters>)" },

            // =========================================================
            // CINEMATIC / EXPERIMENTAL PRESETS
            // =========================================================

            { "Sardaukar Chant", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Sardaukar Chant" version="1.0" category="Cinematic">
  <PARAM id="osc1_waveform" value="2"/><PARAM id="osc1_level" value="0.75"/><PARAM id="osc1_detune" value="-3"/><PARAM id="osc1_octave" value="-1"/>
  <PARAM id="osc2_waveform" value="2"/><PARAM id="osc2_level" value="0.7"/><PARAM id="osc2_detune" value="3"/><PARAM id="osc2_octave" value="-1"/>
  <PARAM id="sub_level" value="0.6"/><PARAM id="sub_octave" value="-2"/>
  <PARAM id="filter_cutoff" value="1400"/><PARAM id="filter_resonance" value="0.45"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.15"/><PARAM id="filter_env_amount" value="0.25"/>
  <PARAM id="filter_mode" value="1"/><PARAM id="formant_vowel" value="3"/>
  <PARAM id="amp_attack" value="1.8"/><PARAM id="amp_decay" value="1.0"/><PARAM id="amp_sustain" value="0.85"/><PARAM id="amp_release" value="4.0"/>
  <PARAM id="filter_attack" value="3.0"/><PARAM id="filter_decay" value="2.0"/><PARAM id="filter_sustain" value="0.4"/><PARAM id="filter_release" value="5.0"/>
  <PARAM id="lfo1_rate" value="0.08"/><PARAM id="lfo1_waveform" value="1"/><PARAM id="lfo2_rate" value="0.12"/><PARAM id="lfo2_waveform" value="0"/>
  <PARAM id="orbit_shape" value="3"/><PARAM id="orbit_rate" value="0.06"/>
  <PARAM id="mod_1_src" value="8"/><PARAM id="mod_1_dst" value="6"/><PARAM id="mod_1_amt" value="0.55"/>
  <PARAM id="mod_2_src" value="9"/><PARAM id="mod_2_dst" value="7"/><PARAM id="mod_2_amt" value="0.3"/>
  <PARAM id="mod_3_src" value="1"/><PARAM id="mod_3_dst" value="3"/><PARAM id="mod_3_amt" value="0.2"/>
  <PARAM id="mod_4_src" value="2"/><PARAM id="mod_4_dst" value="4"/><PARAM id="mod_4_amt" value="-0.2"/>
  <PARAM id="mod_5_src" value="10"/><PARAM id="mod_5_dst" value="5"/><PARAM id="mod_5_amt" value="0.15"/>
  <PARAM id="mod_6_src" value="4"/><PARAM id="mod_6_dst" value="8"/><PARAM id="mod_6_amt" value="0.3"/>
  <PARAM id="master_gain" value="-6"/>
  <PARAM id="dist_enable" value="1"/><PARAM id="dist_drive" value="1.5"/><PARAM id="dist_mix" value="0.15"/>
  <PARAM id="chorus_enable" value="1"/><PARAM id="chorus_rate" value="0.12"/><PARAM id="chorus_depth" value="0.8"/><PARAM id="chorus_mix" value="0.35"/>
  <PARAM id="delay_enable" value="1"/><PARAM id="delay_time" value="800"/><PARAM id="delay_feedback" value="0.55"/><PARAM id="delay_mix" value="0.2"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.95"/><PARAM id="reverb_damp" value="0.45"/><PARAM id="reverb_mix" value="0.6"/>
</Parameters>)" },

            { "Sandworm Rising", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Sandworm Rising" version="1.0" category="Cinematic">
  <PARAM id="osc1_waveform" value="2"/><PARAM id="osc1_level" value="0.9"/><PARAM id="osc1_detune" value="0"/><PARAM id="osc1_octave" value="-2"/>
  <PARAM id="osc2_waveform" value="3"/><PARAM id="osc2_level" value="0.5"/><PARAM id="osc2_detune" value="-15"/><PARAM id="osc2_octave" value="-1"/>
  <PARAM id="sub_level" value="0.8"/><PARAM id="sub_octave" value="-2"/>
  <PARAM id="filter_cutoff" value="600"/><PARAM id="filter_resonance" value="0.7"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.4"/><PARAM id="filter_env_amount" value="0.7"/>
  <PARAM id="filter_mode" value="2"/>
  <PARAM id="amp_attack" value="3.0"/><PARAM id="amp_decay" value="0.5"/><PARAM id="amp_sustain" value="0.9"/><PARAM id="amp_release" value="5.0"/>
  <PARAM id="filter_attack" value="4.0"/><PARAM id="filter_decay" value="2.0"/><PARAM id="filter_sustain" value="0.6"/><PARAM id="filter_release" value="6.0"/>
  <PARAM id="lfo1_rate" value="0.05"/><PARAM id="lfo1_waveform" value="2"/><PARAM id="lfo2_rate" value="0.3"/><PARAM id="lfo2_waveform" value="0"/>
  <PARAM id="orbit_shape" value="5"/><PARAM id="orbit_rate" value="0.04"/>
  <PARAM id="mod_1_src" value="8"/><PARAM id="mod_1_dst" value="6"/><PARAM id="mod_1_amt" value="0.6"/>
  <PARAM id="mod_2_src" value="9"/><PARAM id="mod_2_dst" value="1"/><PARAM id="mod_2_amt" value="0.08"/>
  <PARAM id="mod_3_src" value="1"/><PARAM id="mod_3_dst" value="7"/><PARAM id="mod_3_amt" value="0.4"/>
  <PARAM id="mod_4_src" value="4"/><PARAM id="mod_4_dst" value="6"/><PARAM id="mod_4_amt" value="0.5"/>
  <PARAM id="mod_5_src" value="10"/><PARAM id="mod_5_dst" value="8"/><PARAM id="mod_5_amt" value="0.15"/>
  <PARAM id="mod_6_src" value="2"/><PARAM id="mod_6_dst" value="2"/><PARAM id="mod_6_amt" value="0.05"/>
  <PARAM id="master_gain" value="-8"/>
  <PARAM id="dist_enable" value="1"/><PARAM id="dist_drive" value="3.0"/><PARAM id="dist_mix" value="0.25"/>
  <PARAM id="chorus_enable" value="0"/>
  <PARAM id="delay_enable" value="1"/><PARAM id="delay_time" value="1200"/><PARAM id="delay_feedback" value="0.65"/><PARAM id="delay_mix" value="0.15"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.98"/><PARAM id="reverb_damp" value="0.55"/><PARAM id="reverb_mix" value="0.55"/>
</Parameters>)" },

            { "Spice Vision", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Spice Vision" version="1.0" category="Cinematic">
  <PARAM id="osc1_waveform" value="0"/><PARAM id="osc1_level" value="0.6"/><PARAM id="osc1_detune" value="-7"/><PARAM id="osc1_octave" value="0"/>
  <PARAM id="osc2_waveform" value="0"/><PARAM id="osc2_level" value="0.6"/><PARAM id="osc2_detune" value="7"/><PARAM id="osc2_octave" value="1"/>
  <PARAM id="sub_level" value="0.3"/><PARAM id="sub_octave" value="-1"/>
  <PARAM id="filter_cutoff" value="4500"/><PARAM id="filter_resonance" value="0.25"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.0"/><PARAM id="filter_env_amount" value="0.1"/>
  <PARAM id="filter_mode" value="0"/>
  <PARAM id="amp_attack" value="2.5"/><PARAM id="amp_decay" value="1.5"/><PARAM id="amp_sustain" value="0.75"/><PARAM id="amp_release" value="5.0"/>
  <PARAM id="filter_attack" value="3.0"/><PARAM id="filter_decay" value="2.0"/><PARAM id="filter_sustain" value="0.5"/><PARAM id="filter_release" value="4.0"/>
  <PARAM id="lfo1_rate" value="0.15"/><PARAM id="lfo1_waveform" value="0"/><PARAM id="lfo2_rate" value="0.07"/><PARAM id="lfo2_waveform" value="1"/>
  <PARAM id="orbit_shape" value="6"/><PARAM id="orbit_rate" value="55.0"/>
  <PARAM id="mod_1_src" value="8"/><PARAM id="mod_1_dst" value="1"/><PARAM id="mod_1_amt" value="0.12"/>
  <PARAM id="mod_2_src" value="9"/><PARAM id="mod_2_dst" value="2"/><PARAM id="mod_2_amt" value="-0.12"/>
  <PARAM id="mod_3_src" value="10"/><PARAM id="mod_3_dst" value="6"/><PARAM id="mod_3_amt" value="0.2"/>
  <PARAM id="mod_4_src" value="1"/><PARAM id="mod_4_dst" value="8"/><PARAM id="mod_4_amt" value="0.15"/>
  <PARAM id="mod_5_src" value="2"/><PARAM id="mod_5_dst" value="9"/><PARAM id="mod_5_amt" value="0.3"/>
  <PARAM id="mod_6_src" value="11"/><PARAM id="mod_6_dst" value="7"/><PARAM id="mod_6_amt" value="0.2"/>
  <PARAM id="master_gain" value="-7"/>
  <PARAM id="dist_enable" value="0"/>
  <PARAM id="chorus_enable" value="1"/><PARAM id="chorus_rate" value="0.08"/><PARAM id="chorus_depth" value="0.9"/><PARAM id="chorus_mix" value="0.45"/>
  <PARAM id="delay_enable" value="1"/><PARAM id="delay_time" value="600"/><PARAM id="delay_feedback" value="0.5"/><PARAM id="delay_mix" value="0.3"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.92"/><PARAM id="reverb_damp" value="0.25"/><PARAM id="reverb_mix" value="0.65"/>
</Parameters>)" },

            { "Fremen Whisper", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Fremen Whisper" version="1.0" category="Cinematic">
  <PARAM id="osc1_waveform" value="2"/><PARAM id="osc1_level" value="0.4"/><PARAM id="osc1_detune" value="25"/><PARAM id="osc1_octave" value="1"/>
  <PARAM id="osc2_waveform" value="2"/><PARAM id="osc2_level" value="0.4"/><PARAM id="osc2_detune" value="-25"/><PARAM id="osc2_octave" value="1"/>
  <PARAM id="sub_level" value="0.15"/><PARAM id="sub_octave" value="0"/>
  <PARAM id="filter_cutoff" value="2800"/><PARAM id="filter_resonance" value="0.65"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.05"/><PARAM id="filter_env_amount" value="-0.3"/>
  <PARAM id="filter_mode" value="3"/>
  <PARAM id="amp_attack" value="0.8"/><PARAM id="amp_decay" value="2.0"/><PARAM id="amp_sustain" value="0.3"/><PARAM id="amp_release" value="6.0"/>
  <PARAM id="filter_attack" value="0.5"/><PARAM id="filter_decay" value="3.0"/><PARAM id="filter_sustain" value="0.2"/><PARAM id="filter_release" value="4.0"/>
  <PARAM id="lfo1_rate" value="0.25"/><PARAM id="lfo1_waveform" value="0"/><PARAM id="lfo2_rate" value="3.5"/><PARAM id="lfo2_waveform" value="2"/>
  <PARAM id="orbit_shape" value="1"/><PARAM id="orbit_rate" value="0.18"/>
  <PARAM id="mod_1_src" value="1"/><PARAM id="mod_1_dst" value="6"/><PARAM id="mod_1_amt" value="0.35"/>
  <PARAM id="mod_2_src" value="2"/><PARAM id="mod_2_dst" value="3"/><PARAM id="mod_2_amt" value="0.5"/>
  <PARAM id="mod_3_src" value="2"/><PARAM id="mod_3_dst" value="4"/><PARAM id="mod_3_amt" value="-0.5"/>
  <PARAM id="mod_4_src" value="8"/><PARAM id="mod_4_dst" value="7"/><PARAM id="mod_4_amt" value="0.4"/>
  <PARAM id="mod_5_src" value="9"/><PARAM id="mod_5_dst" value="10"/><PARAM id="mod_5_amt" value="0.3"/>
  <PARAM id="mod_6_src" value="3"/><PARAM id="mod_6_dst" value="8"/><PARAM id="mod_6_amt" value="0.4"/>
  <PARAM id="master_gain" value="-5"/>
  <PARAM id="dist_enable" value="0"/>
  <PARAM id="chorus_enable" value="1"/><PARAM id="chorus_rate" value="0.3"/><PARAM id="chorus_depth" value="0.5"/><PARAM id="chorus_mix" value="0.3"/>
  <PARAM id="delay_enable" value="1"/><PARAM id="delay_time" value="450"/><PARAM id="delay_feedback" value="0.6"/><PARAM id="delay_mix" value="0.35"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.88"/><PARAM id="reverb_damp" value="0.5"/><PARAM id="reverb_mix" value="0.5"/>
</Parameters>)" },

            { "Arrakis Wind", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Arrakis Wind" version="1.0" category="Cinematic">
  <PARAM id="osc1_waveform" value="2"/><PARAM id="osc1_level" value="0.8"/><PARAM id="osc1_detune" value="40"/><PARAM id="osc1_octave" value="0"/>
  <PARAM id="osc2_waveform" value="3"/><PARAM id="osc2_level" value="0.6"/><PARAM id="osc2_detune" value="-35"/><PARAM id="osc2_octave" value="0"/>
  <PARAM id="sub_level" value="0.0"/><PARAM id="sub_octave" value="-1"/>
  <PARAM id="filter_cutoff" value="1200"/><PARAM id="filter_resonance" value="0.8"/><PARAM id="filter_type" value="2"/><PARAM id="filter_drive" value="0.3"/><PARAM id="filter_env_amount" value="0.4"/>
  <PARAM id="filter_mode" value="2"/>
  <PARAM id="amp_attack" value="2.0"/><PARAM id="amp_decay" value="1.0"/><PARAM id="amp_sustain" value="0.7"/><PARAM id="amp_release" value="3.0"/>
  <PARAM id="filter_attack" value="1.5"/><PARAM id="filter_decay" value="3.0"/><PARAM id="filter_sustain" value="0.3"/><PARAM id="filter_release" value="4.0"/>
  <PARAM id="lfo1_rate" value="0.4"/><PARAM id="lfo1_waveform" value="2"/><PARAM id="lfo2_rate" value="6.0"/><PARAM id="lfo2_waveform" value="0"/>
  <PARAM id="orbit_shape" value="4"/><PARAM id="orbit_rate" value="0.35"/>
  <PARAM id="mod_1_src" value="8"/><PARAM id="mod_1_dst" value="6"/><PARAM id="mod_1_amt" value="0.7"/>
  <PARAM id="mod_2_src" value="9"/><PARAM id="mod_2_dst" value="7"/><PARAM id="mod_2_amt" value="0.5"/>
  <PARAM id="mod_3_src" value="1"/><PARAM id="mod_3_dst" value="1"/><PARAM id="mod_3_amt" value="0.1"/>
  <PARAM id="mod_4_src" value="2"/><PARAM id="mod_4_dst" value="2"/><PARAM id="mod_4_amt" value="-0.08"/>
  <PARAM id="mod_5_src" value="10"/><PARAM id="mod_5_dst" value="9"/><PARAM id="mod_5_amt" value="0.4"/>
  <PARAM id="mod_6_src" value="11"/><PARAM id="mod_6_dst" value="10"/><PARAM id="mod_6_amt" value="-0.35"/>
  <PARAM id="master_gain" value="-7"/>
  <PARAM id="dist_enable" value="1"/><PARAM id="dist_drive" value="2.0"/><PARAM id="dist_mix" value="0.2"/>
  <PARAM id="chorus_enable" value="0"/>
  <PARAM id="delay_enable" value="1"/><PARAM id="delay_time" value="900"/><PARAM id="delay_feedback" value="0.7"/><PARAM id="delay_mix" value="0.25"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.93"/><PARAM id="reverb_damp" value="0.6"/><PARAM id="reverb_mix" value="0.5"/>
</Parameters>)" },

            { "Voice of the Worm", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Voice of the Worm" version="1.0" category="Cinematic">
  <PARAM id="osc1_waveform" value="3"/><PARAM id="osc1_level" value="0.85"/><PARAM id="osc1_detune" value="0"/><PARAM id="osc1_octave" value="-2"/>
  <PARAM id="osc2_waveform" value="2"/><PARAM id="osc2_level" value="0.65"/><PARAM id="osc2_detune" value="8"/><PARAM id="osc2_octave" value="-2"/>
  <PARAM id="sub_level" value="0.9"/><PARAM id="sub_octave" value="-2"/>
  <PARAM id="filter_cutoff" value="800"/><PARAM id="filter_resonance" value="0.55"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.5"/><PARAM id="filter_env_amount" value="0.35"/>
  <PARAM id="filter_mode" value="1"/><PARAM id="formant_vowel" value="4"/>
  <PARAM id="amp_attack" value="2.0"/><PARAM id="amp_decay" value="0.8"/><PARAM id="amp_sustain" value="0.9"/><PARAM id="amp_release" value="4.0"/>
  <PARAM id="filter_attack" value="2.5"/><PARAM id="filter_decay" value="1.5"/><PARAM id="filter_sustain" value="0.5"/><PARAM id="filter_release" value="3.0"/>
  <PARAM id="lfo1_rate" value="0.1"/><PARAM id="lfo1_waveform" value="0"/><PARAM id="lfo2_rate" value="0.04"/><PARAM id="lfo2_waveform" value="1"/>
  <PARAM id="orbit_shape" value="0"/><PARAM id="orbit_rate" value="0.08"/>
  <PARAM id="mod_1_src" value="8"/><PARAM id="mod_1_dst" value="6"/><PARAM id="mod_1_amt" value="0.45"/>
  <PARAM id="mod_2_src" value="9"/><PARAM id="mod_2_dst" value="7"/><PARAM id="mod_2_amt" value="0.35"/>
  <PARAM id="mod_3_src" value="1"/><PARAM id="mod_3_dst" value="5"/><PARAM id="mod_3_amt" value="0.3"/>
  <PARAM id="mod_4_src" value="2"/><PARAM id="mod_4_dst" value="6"/><PARAM id="mod_4_amt" value="0.2"/>
  <PARAM id="mod_5_src" value="10"/><PARAM id="mod_5_dst" value="1"/><PARAM id="mod_5_amt" value="0.04"/>
  <PARAM id="mod_6_src" value="11"/><PARAM id="mod_6_dst" value="2"/><PARAM id="mod_6_amt" value="-0.04"/>
  <PARAM id="master_gain" value="-9"/>
  <PARAM id="dist_enable" value="1"/><PARAM id="dist_drive" value="4.0"/><PARAM id="dist_mix" value="0.3"/>
  <PARAM id="chorus_enable" value="0"/>
  <PARAM id="delay_enable" value="0"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.97"/><PARAM id="reverb_damp" value="0.65"/><PARAM id="reverb_mix" value="0.45"/>
  <PARAM id="ott_enable" value="1"/><PARAM id="ott_depth" value="0.4"/><PARAM id="ott_time" value="0.6"/><PARAM id="ott_mix" value="0.25"/>
</Parameters>)" },

            { "Prescient Dream", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Prescient Dream" version="1.0" category="Cinematic">
  <PARAM id="osc1_waveform" value="1"/><PARAM id="osc1_level" value="0.5"/><PARAM id="osc1_detune" value="-12"/><PARAM id="osc1_octave" value="0"/>
  <PARAM id="osc2_waveform" value="0"/><PARAM id="osc2_level" value="0.5"/><PARAM id="osc2_detune" value="12"/><PARAM id="osc2_octave" value="1"/>
  <PARAM id="sub_level" value="0.35"/><PARAM id="sub_octave" value="-1"/>
  <PARAM id="filter_cutoff" value="5000"/><PARAM id="filter_resonance" value="0.15"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.0"/><PARAM id="filter_env_amount" value="0.05"/>
  <PARAM id="filter_mode" value="0"/>
  <PARAM id="amp_attack" value="3.5"/><PARAM id="amp_decay" value="2.0"/><PARAM id="amp_sustain" value="0.7"/><PARAM id="amp_release" value="6.0"/>
  <PARAM id="filter_attack" value="4.0"/><PARAM id="filter_decay" value="3.0"/><PARAM id="filter_sustain" value="0.4"/><PARAM id="filter_release" value="5.0"/>
  <PARAM id="lfo1_rate" value="0.06"/><PARAM id="lfo1_waveform" value="0"/><PARAM id="lfo2_rate" value="0.1"/><PARAM id="lfo2_waveform" value="1"/>
  <PARAM id="orbit_shape" value="6"/><PARAM id="orbit_rate" value="0.03"/>
  <PARAM id="mod_1_src" value="8"/><PARAM id="mod_1_dst" value="1"/><PARAM id="mod_1_amt" value="0.06"/>
  <PARAM id="mod_2_src" value="9"/><PARAM id="mod_2_dst" value="2"/><PARAM id="mod_2_amt" value="-0.06"/>
  <PARAM id="mod_3_src" value="10"/><PARAM id="mod_3_dst" value="3"/><PARAM id="mod_3_amt" value="0.25"/>
  <PARAM id="mod_4_src" value="11"/><PARAM id="mod_4_dst" value="4"/><PARAM id="mod_4_amt" value="-0.25"/>
  <PARAM id="mod_5_src" value="1"/><PARAM id="mod_5_dst" value="6"/><PARAM id="mod_5_amt" value="0.15"/>
  <PARAM id="mod_6_src" value="2"/><PARAM id="mod_6_dst" value="8"/><PARAM id="mod_6_amt" value="0.1"/>
  <PARAM id="mod_7_src" value="4"/><PARAM id="mod_7_dst" value="9"/><PARAM id="mod_7_amt" value="0.2"/>
  <PARAM id="master_gain" value="-6"/>
  <PARAM id="dist_enable" value="0"/>
  <PARAM id="chorus_enable" value="1"/><PARAM id="chorus_rate" value="0.05"/><PARAM id="chorus_depth" value="0.95"/><PARAM id="chorus_mix" value="0.5"/>
  <PARAM id="delay_enable" value="1"/><PARAM id="delay_time" value="750"/><PARAM id="delay_feedback" value="0.6"/><PARAM id="delay_mix" value="0.3"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.96"/><PARAM id="reverb_damp" value="0.2"/><PARAM id="reverb_mix" value="0.7"/>
</Parameters>)" },

            { "Bene Gesserit", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Bene Gesserit" version="1.0" category="Cinematic">
  <PARAM id="osc1_waveform" value="2"/><PARAM id="osc1_level" value="0.65"/><PARAM id="osc1_detune" value="2"/><PARAM id="osc1_octave" value="0"/>
  <PARAM id="osc2_waveform" value="1"/><PARAM id="osc2_level" value="0.55"/><PARAM id="osc2_detune" value="-2"/><PARAM id="osc2_octave" value="0"/>
  <PARAM id="sub_level" value="0.25"/><PARAM id="sub_octave" value="-1"/>
  <PARAM id="filter_cutoff" value="1800"/><PARAM id="filter_resonance" value="0.5"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.1"/><PARAM id="filter_env_amount" value="0.15"/>
  <PARAM id="filter_mode" value="1"/><PARAM id="formant_vowel" value="1"/>
  <PARAM id="amp_attack" value="1.2"/><PARAM id="amp_decay" value="0.8"/><PARAM id="amp_sustain" value="0.8"/><PARAM id="amp_release" value="3.5"/>
  <PARAM id="filter_attack" value="1.5"/><PARAM id="filter_decay" value="1.0"/><PARAM id="filter_sustain" value="0.6"/><PARAM id="filter_release" value="3.0"/>
  <PARAM id="lfo1_rate" value="0.2"/><PARAM id="lfo1_waveform" value="1"/><PARAM id="lfo2_rate" value="0.08"/><PARAM id="lfo2_waveform" value="0"/>
  <PARAM id="orbit_shape" value="3"/><PARAM id="orbit_rate" value="0.15"/>
  <PARAM id="mod_1_src" value="8"/><PARAM id="mod_1_dst" value="6"/><PARAM id="mod_1_amt" value="0.5"/>
  <PARAM id="mod_2_src" value="9"/><PARAM id="mod_2_dst" value="3"/><PARAM id="mod_2_amt" value="0.3"/>
  <PARAM id="mod_3_src" value="10"/><PARAM id="mod_3_dst" value="4"/><PARAM id="mod_3_amt" value="-0.3"/>
  <PARAM id="mod_4_src" value="1"/><PARAM id="mod_4_dst" value="7"/><PARAM id="mod_4_amt" value="0.25"/>
  <PARAM id="mod_5_src" value="2"/><PARAM id="mod_5_dst" value="5"/><PARAM id="mod_5_amt" value="0.2"/>
  <PARAM id="mod_6_src" value="4"/><PARAM id="mod_6_dst" value="8"/><PARAM id="mod_6_amt" value="0.35"/>
  <PARAM id="mod_7_src" value="11"/><PARAM id="mod_7_dst" value="10"/><PARAM id="mod_7_amt" value="0.25"/>
  <PARAM id="master_gain" value="-5"/>
  <PARAM id="dist_enable" value="0"/>
  <PARAM id="chorus_enable" value="1"/><PARAM id="chorus_rate" value="0.18"/><PARAM id="chorus_depth" value="0.65"/><PARAM id="chorus_mix" value="0.4"/>
  <PARAM id="delay_enable" value="1"/><PARAM id="delay_time" value="550"/><PARAM id="delay_feedback" value="0.45"/><PARAM id="delay_mix" value="0.2"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.9"/><PARAM id="reverb_damp" value="0.35"/><PARAM id="reverb_mix" value="0.55"/>
</Parameters>)" },

            { "Holtzman Shield", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Holtzman Shield" version="1.0" category="Cinematic">
  <PARAM id="osc1_waveform" value="3"/><PARAM id="osc1_level" value="0.9"/><PARAM id="osc1_detune" value="0"/><PARAM id="osc1_octave" value="0"/>
  <PARAM id="osc2_waveform" value="3"/><PARAM id="osc2_level" value="0.7"/><PARAM id="osc2_detune" value="50"/><PARAM id="osc2_octave" value="1"/>
  <PARAM id="sub_level" value="0.0"/><PARAM id="sub_octave" value="-1"/>
  <PARAM id="filter_cutoff" value="3500"/><PARAM id="filter_resonance" value="0.85"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.35"/><PARAM id="filter_env_amount" value="0.6"/>
  <PARAM id="filter_mode" value="2"/>
  <PARAM id="amp_attack" value="0.001"/><PARAM id="amp_decay" value="0.15"/><PARAM id="amp_sustain" value="0.0"/><PARAM id="amp_release" value="2.0"/>
  <PARAM id="filter_attack" value="0.001"/><PARAM id="filter_decay" value="0.3"/><PARAM id="filter_sustain" value="0.1"/><PARAM id="filter_release" value="1.5"/>
  <PARAM id="lfo1_rate" value="12.0"/><PARAM id="lfo1_waveform" value="3"/><PARAM id="lfo2_rate" value="0.5"/><PARAM id="lfo2_waveform" value="2"/>
  <PARAM id="orbit_shape" value="2"/><PARAM id="orbit_rate" value="3.5"/>
  <PARAM id="mod_1_src" value="8"/><PARAM id="mod_1_dst" value="6"/><PARAM id="mod_1_amt" value="0.6"/>
  <PARAM id="mod_2_src" value="9"/><PARAM id="mod_2_dst" value="7"/><PARAM id="mod_2_amt" value="0.5"/>
  <PARAM id="mod_3_src" value="1"/><PARAM id="mod_3_dst" value="1"/><PARAM id="mod_3_amt" value="0.2"/>
  <PARAM id="mod_4_src" value="10"/><PARAM id="mod_4_dst" value="3"/><PARAM id="mod_4_amt" value="0.4"/>
  <PARAM id="mod_5_src" value="11"/><PARAM id="mod_5_dst" value="4"/><PARAM id="mod_5_amt" value="-0.4"/>
  <PARAM id="mod_6_src" value="5"/><PARAM id="mod_6_dst" value="8"/><PARAM id="mod_6_amt" value="0.5"/>
  <PARAM id="master_gain" value="-7"/>
  <PARAM id="dist_enable" value="1"/><PARAM id="dist_drive" value="2.5"/><PARAM id="dist_mix" value="0.3"/>
  <PARAM id="chorus_enable" value="0"/>
  <PARAM id="delay_enable" value="1"/><PARAM id="delay_time" value="180"/><PARAM id="delay_feedback" value="0.75"/><PARAM id="delay_mix" value="0.35"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.75"/><PARAM id="reverb_damp" value="0.3"/><PARAM id="reverb_mix" value="0.4"/>
  <PARAM id="ott_enable" value="1"/><PARAM id="ott_depth" value="0.6"/><PARAM id="ott_time" value="0.3"/><PARAM id="ott_mix" value="0.35"/>
</Parameters>)" },

            { "Thumper Signal", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Thumper Signal" version="1.0" category="Cinematic">
  <PARAM id="osc1_waveform" value="0"/><PARAM id="osc1_level" value="0.9"/><PARAM id="osc1_detune" value="0"/><PARAM id="osc1_octave" value="-2"/>
  <PARAM id="osc2_waveform" value="3"/><PARAM id="osc2_level" value="0.4"/><PARAM id="osc2_detune" value="0"/><PARAM id="osc2_octave" value="-1"/>
  <PARAM id="sub_level" value="0.95"/><PARAM id="sub_octave" value="-2"/>
  <PARAM id="filter_cutoff" value="400"/><PARAM id="filter_resonance" value="0.6"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.6"/><PARAM id="filter_env_amount" value="0.8"/>
  <PARAM id="filter_mode" value="0"/>
  <PARAM id="amp_attack" value="0.001"/><PARAM id="amp_decay" value="0.25"/><PARAM id="amp_sustain" value="0.0"/><PARAM id="amp_release" value="0.8"/>
  <PARAM id="filter_attack" value="0.001"/><PARAM id="filter_decay" value="0.2"/><PARAM id="filter_sustain" value="0.05"/><PARAM id="filter_release" value="0.5"/>
  <PARAM id="lfo1_rate" value="4.0"/><PARAM id="lfo1_waveform" value="3"/><PARAM id="lfo2_rate" value="0.15"/><PARAM id="lfo2_waveform" value="2"/>
  <PARAM id="orbit_shape" value="2"/><PARAM id="orbit_rate" value="0.8"/>
  <PARAM id="mod_1_src" value="1"/><PARAM id="mod_1_dst" value="8"/><PARAM id="mod_1_amt" value="0.7"/>
  <PARAM id="mod_2_src" value="8"/><PARAM id="mod_2_dst" value="6"/><PARAM id="mod_2_amt" value="0.3"/>
  <PARAM id="mod_3_src" value="2"/><PARAM id="mod_3_dst" value="1"/><PARAM id="mod_3_amt" value="0.05"/>
  <PARAM id="mod_4_src" value="5"/><PARAM id="mod_4_dst" value="6"/><PARAM id="mod_4_amt" value="0.4"/>
  <PARAM id="mod_5_src" value="9"/><PARAM id="mod_5_dst" value="7"/><PARAM id="mod_5_amt" value="0.25"/>
  <PARAM id="master_gain" value="-8"/>
  <PARAM id="dist_enable" value="1"/><PARAM id="dist_drive" value="5.0"/><PARAM id="dist_mix" value="0.4"/>
  <PARAM id="chorus_enable" value="0"/>
  <PARAM id="delay_enable" value="1"/><PARAM id="delay_time" value="250"/><PARAM id="delay_feedback" value="0.5"/><PARAM id="delay_mix" value="0.2"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.85"/><PARAM id="reverb_damp" value="0.7"/><PARAM id="reverb_mix" value="0.35"/>
</Parameters>)" },

            // =========================================================
            // CLASSIC SYNTH EMULATION PRESETS
            // =========================================================

            { "303 Acid Line", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="303 Acid Line" version="1.0" category="Classic">
  <PARAM id="osc1_waveform" value="2"/><PARAM id="osc1_level" value="0.95"/><PARAM id="osc1_detune" value="0"/><PARAM id="osc1_octave" value="-1"/>
  <PARAM id="osc2_enable" value="0"/><PARAM id="osc2_level" value="0.0"/>
  <PARAM id="sub_level" value="0.0"/>
  <PARAM id="filter_cutoff" value="500"/><PARAM id="filter_resonance" value="0.82"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.4"/><PARAM id="filter_env_amount" value="0.85"/>
  <PARAM id="filter_mode" value="0"/>
  <PARAM id="amp_attack" value="0.001"/><PARAM id="amp_decay" value="0.2"/><PARAM id="amp_sustain" value="0.0"/><PARAM id="amp_release" value="0.15"/>
  <PARAM id="filter_attack" value="0.001"/><PARAM id="filter_decay" value="0.25"/><PARAM id="filter_sustain" value="0.0"/><PARAM id="filter_release" value="0.1"/>
  <PARAM id="lfo1_rate" value="5.0"/><PARAM id="lfo1_waveform" value="3"/>
  <PARAM id="master_gain" value="-4"/>
  <PARAM id="dist_enable" value="1"/><PARAM id="dist_drive" value="3.5"/><PARAM id="dist_mix" value="0.45"/>
  <PARAM id="chorus_enable" value="0"/>
  <PARAM id="delay_enable" value="1"/><PARAM id="delay_time" value="187"/><PARAM id="delay_feedback" value="0.35"/><PARAM id="delay_mix" value="0.15"/>
  <PARAM id="reverb_enable" value="0"/>
</Parameters>)" },

            { "303 Square Acid", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="303 Square Acid" version="1.0" category="Classic">
  <PARAM id="osc1_waveform" value="3"/><PARAM id="osc1_level" value="0.9"/><PARAM id="osc1_detune" value="0"/><PARAM id="osc1_octave" value="-1"/>
  <PARAM id="osc2_enable" value="0"/><PARAM id="osc2_level" value="0.0"/>
  <PARAM id="sub_level" value="0.0"/>
  <PARAM id="filter_cutoff" value="400"/><PARAM id="filter_resonance" value="0.88"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.5"/><PARAM id="filter_env_amount" value="0.9"/>
  <PARAM id="filter_mode" value="0"/>
  <PARAM id="amp_attack" value="0.001"/><PARAM id="amp_decay" value="0.18"/><PARAM id="amp_sustain" value="0.0"/><PARAM id="amp_release" value="0.12"/>
  <PARAM id="filter_attack" value="0.001"/><PARAM id="filter_decay" value="0.2"/><PARAM id="filter_sustain" value="0.0"/><PARAM id="filter_release" value="0.08"/>
  <PARAM id="master_gain" value="-4"/>
  <PARAM id="dist_enable" value="1"/><PARAM id="dist_drive" value="4.5"/><PARAM id="dist_mix" value="0.5"/>
  <PARAM id="chorus_enable" value="0"/><PARAM id="delay_enable" value="0"/><PARAM id="reverb_enable" value="0"/>
</Parameters>)" },

            { "Juno Pad", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Juno Pad" version="1.0" category="Classic">
  <PARAM id="osc1_waveform" value="2"/><PARAM id="osc1_level" value="0.65"/><PARAM id="osc1_detune" value="-8"/><PARAM id="osc1_octave" value="0"/>
  <PARAM id="osc2_waveform" value="3"/><PARAM id="osc2_level" value="0.55"/><PARAM id="osc2_detune" value="8"/><PARAM id="osc2_octave" value="0"/>
  <PARAM id="sub_level" value="0.35"/><PARAM id="sub_octave" value="-1"/>
  <PARAM id="filter_cutoff" value="2500"/><PARAM id="filter_resonance" value="0.2"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.0"/><PARAM id="filter_env_amount" value="0.1"/>
  <PARAM id="filter_mode" value="0"/>
  <PARAM id="amp_attack" value="0.8"/><PARAM id="amp_decay" value="0.5"/><PARAM id="amp_sustain" value="0.85"/><PARAM id="amp_release" value="2.5"/>
  <PARAM id="filter_attack" value="1.0"/><PARAM id="filter_decay" value="0.8"/><PARAM id="filter_sustain" value="0.7"/><PARAM id="filter_release" value="2.0"/>
  <PARAM id="lfo1_rate" value="0.3"/><PARAM id="lfo1_waveform" value="1"/>
  <PARAM id="mod_1_src" value="1"/><PARAM id="mod_1_dst" value="6"/><PARAM id="mod_1_amt" value="0.08"/>
  <PARAM id="master_gain" value="-4"/>
  <PARAM id="dist_enable" value="0"/>
  <PARAM id="chorus_enable" value="1"/><PARAM id="chorus_rate" value="0.5"/><PARAM id="chorus_depth" value="0.7"/><PARAM id="chorus_mix" value="0.55"/>
  <PARAM id="delay_enable" value="0"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.7"/><PARAM id="reverb_damp" value="0.4"/><PARAM id="reverb_mix" value="0.25"/>
</Parameters>)" },

            { "Juno Bass", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Juno Bass" version="1.0" category="Classic">
  <PARAM id="osc1_waveform" value="2"/><PARAM id="osc1_level" value="0.8"/><PARAM id="osc1_detune" value="-5"/><PARAM id="osc1_octave" value="-1"/>
  <PARAM id="osc2_waveform" value="3"/><PARAM id="osc2_level" value="0.6"/><PARAM id="osc2_detune" value="5"/><PARAM id="osc2_octave" value="-1"/>
  <PARAM id="sub_level" value="0.5"/><PARAM id="sub_octave" value="-2"/>
  <PARAM id="filter_cutoff" value="1200"/><PARAM id="filter_resonance" value="0.3"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.05"/><PARAM id="filter_env_amount" value="0.35"/>
  <PARAM id="filter_mode" value="0"/>
  <PARAM id="amp_attack" value="0.005"/><PARAM id="amp_decay" value="0.3"/><PARAM id="amp_sustain" value="0.7"/><PARAM id="amp_release" value="0.2"/>
  <PARAM id="filter_attack" value="0.005"/><PARAM id="filter_decay" value="0.4"/><PARAM id="filter_sustain" value="0.4"/><PARAM id="filter_release" value="0.2"/>
  <PARAM id="master_gain" value="-3"/>
  <PARAM id="dist_enable" value="0"/>
  <PARAM id="chorus_enable" value="1"/><PARAM id="chorus_rate" value="0.6"/><PARAM id="chorus_depth" value="0.5"/><PARAM id="chorus_mix" value="0.4"/>
  <PARAM id="delay_enable" value="0"/><PARAM id="reverb_enable" value="0"/>
</Parameters>)" },

            { "Minimoog Lead", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Minimoog Lead" version="1.0" category="Classic">
  <PARAM id="osc1_waveform" value="2"/><PARAM id="osc1_level" value="0.8"/><PARAM id="osc1_detune" value="-6"/><PARAM id="osc1_octave" value="0"/>
  <PARAM id="osc2_waveform" value="2"/><PARAM id="osc2_level" value="0.75"/><PARAM id="osc2_detune" value="6"/><PARAM id="osc2_octave" value="0"/>
  <PARAM id="sub_level" value="0.6"/><PARAM id="sub_octave" value="-1"/>
  <PARAM id="filter_cutoff" value="1800"/><PARAM id="filter_resonance" value="0.35"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.2"/><PARAM id="filter_env_amount" value="0.4"/>
  <PARAM id="filter_mode" value="0"/>
  <PARAM id="amp_attack" value="0.01"/><PARAM id="amp_decay" value="0.2"/><PARAM id="amp_sustain" value="0.8"/><PARAM id="amp_release" value="0.15"/>
  <PARAM id="filter_attack" value="0.01"/><PARAM id="filter_decay" value="0.3"/><PARAM id="filter_sustain" value="0.5"/><PARAM id="filter_release" value="0.2"/>
  <PARAM id="lfo1_rate" value="5.5"/><PARAM id="lfo1_waveform" value="0"/>
  <PARAM id="mod_1_src" value="5"/><PARAM id="mod_1_dst" value="6"/><PARAM id="mod_1_amt" value="0.2"/>
  <PARAM id="master_gain" value="-3"/>
  <PARAM id="dist_enable" value="1"/><PARAM id="dist_drive" value="1.2"/><PARAM id="dist_mix" value="0.2"/>
  <PARAM id="chorus_enable" value="0"/><PARAM id="delay_enable" value="0"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.3"/><PARAM id="reverb_damp" value="0.5"/><PARAM id="reverb_mix" value="0.1"/>
</Parameters>)" },

            { "Minimoog Bass", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Minimoog Bass" version="1.0" category="Classic">
  <PARAM id="osc1_waveform" value="2"/><PARAM id="osc1_level" value="0.85"/><PARAM id="osc1_detune" value="-4"/><PARAM id="osc1_octave" value="-1"/>
  <PARAM id="osc2_waveform" value="3"/><PARAM id="osc2_level" value="0.7"/><PARAM id="osc2_detune" value="4"/><PARAM id="osc2_octave" value="-2"/>
  <PARAM id="sub_level" value="0.7"/><PARAM id="sub_octave" value="-2"/>
  <PARAM id="filter_cutoff" value="800"/><PARAM id="filter_resonance" value="0.3"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.3"/><PARAM id="filter_env_amount" value="0.5"/>
  <PARAM id="filter_mode" value="0"/>
  <PARAM id="amp_attack" value="0.005"/><PARAM id="amp_decay" value="0.15"/><PARAM id="amp_sustain" value="0.75"/><PARAM id="amp_release" value="0.1"/>
  <PARAM id="filter_attack" value="0.005"/><PARAM id="filter_decay" value="0.2"/><PARAM id="filter_sustain" value="0.3"/><PARAM id="filter_release" value="0.1"/>
  <PARAM id="master_gain" value="-3"/>
  <PARAM id="dist_enable" value="1"/><PARAM id="dist_drive" value="1.5"/><PARAM id="dist_mix" value="0.15"/>
  <PARAM id="chorus_enable" value="0"/><PARAM id="delay_enable" value="0"/><PARAM id="reverb_enable" value="0"/>
</Parameters>)" },

            { "Prophet Strings", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Prophet Strings" version="1.0" category="Classic">
  <PARAM id="osc1_waveform" value="2"/><PARAM id="osc1_level" value="0.6"/><PARAM id="osc1_detune" value="-10"/><PARAM id="osc1_octave" value="0"/>
  <PARAM id="osc2_waveform" value="2"/><PARAM id="osc2_level" value="0.6"/><PARAM id="osc2_detune" value="10"/><PARAM id="osc2_octave" value="0"/>
  <PARAM id="sub_level" value="0.2"/><PARAM id="sub_octave" value="-1"/>
  <PARAM id="filter_cutoff" value="3500"/><PARAM id="filter_resonance" value="0.1"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.0"/><PARAM id="filter_env_amount" value="0.05"/>
  <PARAM id="filter_mode" value="0"/>
  <PARAM id="amp_attack" value="1.2"/><PARAM id="amp_decay" value="0.5"/><PARAM id="amp_sustain" value="0.9"/><PARAM id="amp_release" value="1.5"/>
  <PARAM id="filter_attack" value="1.5"/><PARAM id="filter_decay" value="1.0"/><PARAM id="filter_sustain" value="0.8"/><PARAM id="filter_release" value="1.5"/>
  <PARAM id="lfo1_rate" value="0.2"/><PARAM id="lfo1_waveform" value="1"/>
  <PARAM id="mod_1_src" value="1"/><PARAM id="mod_1_dst" value="6"/><PARAM id="mod_1_amt" value="0.06"/>
  <PARAM id="master_gain" value="-4"/>
  <PARAM id="dist_enable" value="0"/>
  <PARAM id="chorus_enable" value="1"/><PARAM id="chorus_rate" value="0.35"/><PARAM id="chorus_depth" value="0.75"/><PARAM id="chorus_mix" value="0.5"/>
  <PARAM id="delay_enable" value="0"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.65"/><PARAM id="reverb_damp" value="0.3"/><PARAM id="reverb_mix" value="0.3"/>
</Parameters>)" },

            { "OBX Brass", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="OBX Brass" version="1.0" category="Classic">
  <PARAM id="osc1_waveform" value="2"/><PARAM id="osc1_level" value="0.75"/><PARAM id="osc1_detune" value="-6"/><PARAM id="osc1_octave" value="0"/>
  <PARAM id="osc2_waveform" value="2"/><PARAM id="osc2_level" value="0.75"/><PARAM id="osc2_detune" value="6"/><PARAM id="osc2_octave" value="0"/>
  <PARAM id="sub_level" value="0.3"/><PARAM id="sub_octave" value="-1"/>
  <PARAM id="filter_cutoff" value="1000"/><PARAM id="filter_resonance" value="0.15"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.1"/><PARAM id="filter_env_amount" value="0.6"/>
  <PARAM id="filter_mode" value="0"/>
  <PARAM id="amp_attack" value="0.02"/><PARAM id="amp_decay" value="0.15"/><PARAM id="amp_sustain" value="0.85"/><PARAM id="amp_release" value="0.2"/>
  <PARAM id="filter_attack" value="0.02"/><PARAM id="filter_decay" value="0.2"/><PARAM id="filter_sustain" value="0.6"/><PARAM id="filter_release" value="0.25"/>
  <PARAM id="lfo1_rate" value="6.0"/><PARAM id="lfo1_waveform" value="0"/>
  <PARAM id="mod_1_src" value="5"/><PARAM id="mod_1_dst" value="6"/><PARAM id="mod_1_amt" value="0.15"/>
  <PARAM id="master_gain" value="-3"/>
  <PARAM id="dist_enable" value="0"/>
  <PARAM id="chorus_enable" value="1"/><PARAM id="chorus_rate" value="0.4"/><PARAM id="chorus_depth" value="0.4"/><PARAM id="chorus_mix" value="0.3"/>
  <PARAM id="delay_enable" value="0"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.4"/><PARAM id="reverb_damp" value="0.4"/><PARAM id="reverb_mix" value="0.15"/>
</Parameters>)" },

            { "SH-101 Seq Lead", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="SH-101 Seq Lead" version="1.0" category="Classic">
  <PARAM id="osc1_waveform" value="3"/><PARAM id="osc1_level" value="0.9"/><PARAM id="osc1_detune" value="0"/><PARAM id="osc1_octave" value="0"/>
  <PARAM id="osc2_enable" value="0"/><PARAM id="osc2_level" value="0.0"/>
  <PARAM id="sub_level" value="0.4"/><PARAM id="sub_octave" value="-1"/>
  <PARAM id="filter_cutoff" value="2000"/><PARAM id="filter_resonance" value="0.5"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.1"/><PARAM id="filter_env_amount" value="0.5"/>
  <PARAM id="filter_mode" value="0"/>
  <PARAM id="amp_attack" value="0.005"/><PARAM id="amp_decay" value="0.15"/><PARAM id="amp_sustain" value="0.7"/><PARAM id="amp_release" value="0.1"/>
  <PARAM id="filter_attack" value="0.005"/><PARAM id="filter_decay" value="0.25"/><PARAM id="filter_sustain" value="0.3"/><PARAM id="filter_release" value="0.15"/>
  <PARAM id="lfo1_rate" value="4.5"/><PARAM id="lfo1_waveform" value="3"/>
  <PARAM id="mod_1_src" value="1"/><PARAM id="mod_1_dst" value="1"/><PARAM id="mod_1_amt" value="0.08"/>
  <PARAM id="master_gain" value="-3"/>
  <PARAM id="dist_enable" value="0"/>
  <PARAM id="chorus_enable" value="0"/>
  <PARAM id="delay_enable" value="1"/><PARAM id="delay_time" value="375"/><PARAM id="delay_feedback" value="0.4"/><PARAM id="delay_mix" value="0.2"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.35"/><PARAM id="reverb_damp" value="0.5"/><PARAM id="reverb_mix" value="0.15"/>
</Parameters>)" },

            { "MS-20 Growl", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="MS-20 Growl" version="1.0" category="Classic">
  <PARAM id="osc1_waveform" value="2"/><PARAM id="osc1_level" value="0.85"/><PARAM id="osc1_detune" value="0"/><PARAM id="osc1_octave" value="-1"/>
  <PARAM id="osc2_waveform" value="3"/><PARAM id="osc2_level" value="0.7"/><PARAM id="osc2_detune" value="12"/><PARAM id="osc2_octave" value="-1"/>
  <PARAM id="sub_level" value="0.0"/>
  <PARAM id="filter_cutoff" value="900"/><PARAM id="filter_resonance" value="0.85"/><PARAM id="filter_type" value="1"/><PARAM id="filter_drive" value="0.6"/><PARAM id="filter_env_amount" value="0.65"/>
  <PARAM id="filter_mode" value="0"/>
  <PARAM id="amp_attack" value="0.01"/><PARAM id="amp_decay" value="0.2"/><PARAM id="amp_sustain" value="0.75"/><PARAM id="amp_release" value="0.15"/>
  <PARAM id="filter_attack" value="0.01"/><PARAM id="filter_decay" value="0.3"/><PARAM id="filter_sustain" value="0.4"/><PARAM id="filter_release" value="0.2"/>
  <PARAM id="lfo1_rate" value="7.0"/><PARAM id="lfo1_waveform" value="2"/><PARAM id="lfo2_rate" value="0.5"/><PARAM id="lfo2_waveform" value="0"/>
  <PARAM id="mod_1_src" value="1"/><PARAM id="mod_1_dst" value="6"/><PARAM id="mod_1_amt" value="0.15"/>
  <PARAM id="mod_2_src" value="2"/><PARAM id="mod_2_dst" value="1"/><PARAM id="mod_2_amt" value="0.05"/>
  <PARAM id="master_gain" value="-5"/>
  <PARAM id="dist_enable" value="1"/><PARAM id="dist_drive" value="3.0"/><PARAM id="dist_mix" value="0.35"/>
  <PARAM id="chorus_enable" value="0"/><PARAM id="delay_enable" value="0"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.25"/><PARAM id="reverb_damp" value="0.6"/><PARAM id="reverb_mix" value="0.1"/>
</Parameters>)" },

            { "DX7 Bell", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="DX7 Bell" version="1.0" category="Classic">
  <PARAM id="osc1_waveform" value="0"/><PARAM id="osc1_level" value="0.7"/><PARAM id="osc1_detune" value="0"/><PARAM id="osc1_octave" value="1"/>
  <PARAM id="osc2_waveform" value="0"/><PARAM id="osc2_level" value="0.5"/><PARAM id="osc2_detune" value="0"/><PARAM id="osc2_octave" value="2"/>
  <PARAM id="sub_level" value="0.2"/><PARAM id="sub_octave" value="0"/>
  <PARAM id="filter_cutoff" value="6000"/><PARAM id="filter_resonance" value="0.1"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.0"/><PARAM id="filter_env_amount" value="0.15"/>
  <PARAM id="filter_mode" value="0"/>
  <PARAM id="amp_attack" value="0.001"/><PARAM id="amp_decay" value="2.0"/><PARAM id="amp_sustain" value="0.0"/><PARAM id="amp_release" value="3.0"/>
  <PARAM id="filter_attack" value="0.001"/><PARAM id="filter_decay" value="1.5"/><PARAM id="filter_sustain" value="0.2"/><PARAM id="filter_release" value="2.5"/>
  <PARAM id="orbit_shape" value="0"/><PARAM id="orbit_rate" value="80.0"/>
  <PARAM id="mod_1_src" value="8"/><PARAM id="mod_1_dst" value="1"/><PARAM id="mod_1_amt" value="0.15"/>
  <PARAM id="mod_2_src" value="9"/><PARAM id="mod_2_dst" value="2"/><PARAM id="mod_2_amt" value="0.1"/>
  <PARAM id="mod_3_src" value="3"/><PARAM id="mod_3_dst" value="8"/><PARAM id="mod_3_amt" value="0.5"/>
  <PARAM id="master_gain" value="-5"/>
  <PARAM id="dist_enable" value="0"/><PARAM id="chorus_enable" value="0"/>
  <PARAM id="delay_enable" value="1"/><PARAM id="delay_time" value="400"/><PARAM id="delay_feedback" value="0.35"/><PARAM id="delay_mix" value="0.25"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.75"/><PARAM id="reverb_damp" value="0.2"/><PARAM id="reverb_mix" value="0.4"/>
</Parameters>)" },

            { "Polysix Sweep", R"(<?xml version="1.0" encoding="UTF-8"?>
<Parameters presetName="Polysix Sweep" version="1.0" category="Classic">
  <PARAM id="osc1_waveform" value="2"/><PARAM id="osc1_level" value="0.7"/><PARAM id="osc1_detune" value="-5"/><PARAM id="osc1_octave" value="0"/>
  <PARAM id="osc2_waveform" value="1"/><PARAM id="osc2_level" value="0.5"/><PARAM id="osc2_detune" value="5"/><PARAM id="osc2_octave" value="0"/>
  <PARAM id="sub_level" value="0.3"/><PARAM id="sub_octave" value="-1"/>
  <PARAM id="filter_cutoff" value="1500"/><PARAM id="filter_resonance" value="0.4"/><PARAM id="filter_type" value="0"/><PARAM id="filter_drive" value="0.0"/><PARAM id="filter_env_amount" value="0.3"/>
  <PARAM id="filter_mode" value="0"/>
  <PARAM id="amp_attack" value="0.5"/><PARAM id="amp_decay" value="0.8"/><PARAM id="amp_sustain" value="0.8"/><PARAM id="amp_release" value="2.0"/>
  <PARAM id="filter_attack" value="0.8"/><PARAM id="filter_decay" value="1.5"/><PARAM id="filter_sustain" value="0.5"/><PARAM id="filter_release" value="2.0"/>
  <PARAM id="lfo1_rate" value="0.15"/><PARAM id="lfo1_waveform" value="1"/>
  <PARAM id="mod_1_src" value="1"/><PARAM id="mod_1_dst" value="6"/><PARAM id="mod_1_amt" value="0.2"/>
  <PARAM id="master_gain" value="-4"/>
  <PARAM id="dist_enable" value="0"/>
  <PARAM id="chorus_enable" value="1"/><PARAM id="chorus_rate" value="0.45"/><PARAM id="chorus_depth" value="0.6"/><PARAM id="chorus_mix" value="0.45"/>
  <PARAM id="delay_enable" value="0"/>
  <PARAM id="reverb_enable" value="1"/><PARAM id="reverb_size" value="0.55"/><PARAM id="reverb_damp" value="0.35"/><PARAM id="reverb_mix" value="0.2"/>
</Parameters>)" }
        };
        
        // Copiar presets de fábrica si no existen
        for (const auto& preset : factoryPresets)
        {
            auto presetFile = presetDirectory.getChildFile(juce::String(preset.name) + ".kndl");
            if (!presetFile.existsAsFile())
            {
                presetFile.replaceWithText(preset.xml);
            }
        }
    }
    
    juce::AudioProcessorValueTreeState& parameters;
    juce::File presetDirectory;
    juce::String currentPresetName = "Init";
    std::vector<juce::String> presetList;
};

} // namespace kndl
