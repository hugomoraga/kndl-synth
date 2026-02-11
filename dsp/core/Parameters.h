#pragma once

#include <JuceHeader.h>

namespace kndl {

// Parameter IDs - centralizados para evitar typos
namespace ParamID {
    // Oscillator 1
    inline constexpr const char* OSC1_ENABLE = "osc1_enable";
    inline constexpr const char* OSC1_WAVEFORM = "osc1_waveform";
    inline constexpr const char* OSC1_LEVEL = "osc1_level";
    inline constexpr const char* OSC1_DETUNE = "osc1_detune";
    inline constexpr const char* OSC1_OCTAVE = "osc1_octave";
    
    // Oscillator 2
    inline constexpr const char* OSC2_ENABLE = "osc2_enable";
    inline constexpr const char* OSC2_WAVEFORM = "osc2_waveform";
    inline constexpr const char* OSC2_LEVEL = "osc2_level";
    inline constexpr const char* OSC2_DETUNE = "osc2_detune";
    inline constexpr const char* OSC2_OCTAVE = "osc2_octave";
    
    // Sub Oscillator
    inline constexpr const char* SUB_ENABLE = "sub_enable";
    inline constexpr const char* SUB_LEVEL = "sub_level";
    inline constexpr const char* SUB_OCTAVE = "sub_octave";
    
    // Filter
    inline constexpr const char* FILTER_CUTOFF = "filter_cutoff";
    inline constexpr const char* FILTER_RESONANCE = "filter_resonance";
    inline constexpr const char* FILTER_TYPE = "filter_type";
    inline constexpr const char* FILTER_DRIVE = "filter_drive";
    inline constexpr const char* FILTER_ENV_AMOUNT = "filter_env_amount";
    
    // Amp Envelope
    inline constexpr const char* AMP_ATTACK = "amp_attack";
    inline constexpr const char* AMP_DECAY = "amp_decay";
    inline constexpr const char* AMP_SUSTAIN = "amp_sustain";
    inline constexpr const char* AMP_RELEASE = "amp_release";
    
    // Filter Envelope
    inline constexpr const char* FILTER_ATTACK = "filter_attack";
    inline constexpr const char* FILTER_DECAY = "filter_decay";
    inline constexpr const char* FILTER_SUSTAIN = "filter_sustain";
    inline constexpr const char* FILTER_RELEASE = "filter_release";
    
    // LFO 1
    inline constexpr const char* LFO1_RATE = "lfo1_rate";
    inline constexpr const char* LFO1_WAVEFORM = "lfo1_waveform";
    inline constexpr const char* LFO1_SYNC = "lfo1_sync";
    
    // LFO 2
    inline constexpr const char* LFO2_RATE = "lfo2_rate";
    inline constexpr const char* LFO2_WAVEFORM = "lfo2_waveform";
    inline constexpr const char* LFO2_SYNC = "lfo2_sync";
    
    // Master
    inline constexpr const char* MASTER_GAIN = "master_gain";
    
    // Distortion
    inline constexpr const char* DIST_ENABLE = "dist_enable";
    inline constexpr const char* DIST_DRIVE = "dist_drive";
    inline constexpr const char* DIST_MIX = "dist_mix";
    
    // Chorus
    inline constexpr const char* CHORUS_ENABLE = "chorus_enable";
    inline constexpr const char* CHORUS_RATE = "chorus_rate";
    inline constexpr const char* CHORUS_DEPTH = "chorus_depth";
    inline constexpr const char* CHORUS_MIX = "chorus_mix";
    
    // Delay
    inline constexpr const char* DELAY_ENABLE = "delay_enable";
    inline constexpr const char* DELAY_TIME = "delay_time";
    inline constexpr const char* DELAY_FEEDBACK = "delay_feedback";
    inline constexpr const char* DELAY_MIX = "delay_mix";
    
    // Reverb
    inline constexpr const char* REVERB_ENABLE = "reverb_enable";
    inline constexpr const char* REVERB_SIZE = "reverb_size";
    inline constexpr const char* REVERB_DAMP = "reverb_damp";
    inline constexpr const char* REVERB_MIX = "reverb_mix";
    
    // OTT
    inline constexpr const char* OTT_ENABLE = "ott_enable";
    inline constexpr const char* OTT_DEPTH = "ott_depth";
    inline constexpr const char* OTT_TIME = "ott_time";
    inline constexpr const char* OTT_MIX = "ott_mix";
    
    // Advanced Filters
    inline constexpr const char* FILTER_MODE = "filter_mode"; // SVF, Formant, Comb, Notch
    inline constexpr const char* FORMANT_VOWEL = "formant_vowel";
    
    // Mod Matrix (8 slots)
    inline constexpr const char* MOD_1_SRC = "mod_1_src";
    inline constexpr const char* MOD_1_DST = "mod_1_dst";
    inline constexpr const char* MOD_1_AMT = "mod_1_amt";
    inline constexpr const char* MOD_2_SRC = "mod_2_src";
    inline constexpr const char* MOD_2_DST = "mod_2_dst";
    inline constexpr const char* MOD_2_AMT = "mod_2_amt";
    inline constexpr const char* MOD_3_SRC = "mod_3_src";
    inline constexpr const char* MOD_3_DST = "mod_3_dst";
    inline constexpr const char* MOD_3_AMT = "mod_3_amt";
    inline constexpr const char* MOD_4_SRC = "mod_4_src";
    inline constexpr const char* MOD_4_DST = "mod_4_dst";
    inline constexpr const char* MOD_4_AMT = "mod_4_amt";
    inline constexpr const char* MOD_5_SRC = "mod_5_src";
    inline constexpr const char* MOD_5_DST = "mod_5_dst";
    inline constexpr const char* MOD_5_AMT = "mod_5_amt";
    inline constexpr const char* MOD_6_SRC = "mod_6_src";
    inline constexpr const char* MOD_6_DST = "mod_6_dst";
    inline constexpr const char* MOD_6_AMT = "mod_6_amt";
    inline constexpr const char* MOD_7_SRC = "mod_7_src";
    inline constexpr const char* MOD_7_DST = "mod_7_dst";
    inline constexpr const char* MOD_7_AMT = "mod_7_amt";
    inline constexpr const char* MOD_8_SRC = "mod_8_src";
    inline constexpr const char* MOD_8_DST = "mod_8_dst";
    inline constexpr const char* MOD_8_AMT = "mod_8_amt";
    
    // Helper arrays for mod matrix
    inline const char* const MOD_SRC_IDS[] = { MOD_1_SRC, MOD_2_SRC, MOD_3_SRC, MOD_4_SRC, MOD_5_SRC, MOD_6_SRC, MOD_7_SRC, MOD_8_SRC };
    inline const char* const MOD_DST_IDS[] = { MOD_1_DST, MOD_2_DST, MOD_3_DST, MOD_4_DST, MOD_5_DST, MOD_6_DST, MOD_7_DST, MOD_8_DST };
    inline const char* const MOD_AMT_IDS[] = { MOD_1_AMT, MOD_2_AMT, MOD_3_AMT, MOD_4_AMT, MOD_5_AMT, MOD_6_AMT, MOD_7_AMT, MOD_8_AMT };
    inline constexpr int NUM_MOD_SLOTS = 8;
    
    // Spellbook
    inline constexpr const char* SPELLBOOK_SHAPE = "spellbook_shape";
    inline constexpr const char* SPELLBOOK_RATE = "spellbook_rate";
    inline constexpr const char* SPELLBOOK_SYNC = "spellbook_sync";
    inline constexpr const char* SPELLBOOK_NUM_OUTPUTS = "spellbook_num_outputs";
}

// Waveform types
enum class Waveform {
    Sine = 0,
    Triangle,
    Saw,
    Square
};

// Filter types (SVF sub-types)
enum class FilterType {
    LowPass = 0,
    HighPass,
    BandPass
};

// Filter modes (engine selection)
enum class FilterMode {
    SVF = 0,
    Formant,
    Comb,
    Notch
};

// Crear el layout de parámetros para APVTS
inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    // === OSCILLATOR 1 ===
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ParamID::OSC1_ENABLE, 1},
        "Osc1 Enable",
        true  // ON by default
    ));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ParamID::OSC1_WAVEFORM, 1},
        "Osc1 Waveform",
        juce::StringArray{"Sine", "Triangle", "Saw", "Square"},
        2  // default: Saw
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::OSC1_LEVEL, 1},
        "Osc1 Level",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.8f
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::OSC1_DETUNE, 1},
        "Osc1 Detune",
        juce::NormalisableRange<float>(-100.0f, 100.0f),
        0.0f
    ));
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{ParamID::OSC1_OCTAVE, 1},
        "Osc1 Octave",
        -2, 2, 0
    ));
    
    // === OSCILLATOR 2 ===
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ParamID::OSC2_ENABLE, 1},
        "Osc2 Enable",
        false  // OFF by default (level starts at 0)
    ));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ParamID::OSC2_WAVEFORM, 1},
        "Osc2 Waveform",
        juce::StringArray{"Sine", "Triangle", "Saw", "Square"},
        2
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::OSC2_LEVEL, 1},
        "Osc2 Level",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.0f
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::OSC2_DETUNE, 1},
        "Osc2 Detune",
        juce::NormalisableRange<float>(-100.0f, 100.0f),
        0.0f
    ));
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{ParamID::OSC2_OCTAVE, 1},
        "Osc2 Octave",
        -2, 2, 0
    ));
    
    // === SUB OSCILLATOR ===
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ParamID::SUB_ENABLE, 1},
        "Sub Enable",
        false  // OFF by default
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::SUB_LEVEL, 1},
        "Sub Level",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.0f
    ));
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{ParamID::SUB_OCTAVE, 1},
        "Sub Octave",
        -2, 0, -1  // Ahora permite 0 (misma octava), -1 (una abajo), -2 (dos abajo)
    ));
    
    // === FILTER ===
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::FILTER_CUTOFF, 1},
        "Filter Cutoff",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.0f, 0.3f),  // skew for log
        8000.0f
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::FILTER_RESONANCE, 1},
        "Filter Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.0f
    ));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ParamID::FILTER_TYPE, 1},
        "Filter Type",
        juce::StringArray{"Low Pass", "High Pass", "Band Pass"},
        0
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::FILTER_DRIVE, 1},
        "Filter Drive",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.0f
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::FILTER_ENV_AMOUNT, 1},
        "Filter Env Amount",
        juce::NormalisableRange<float>(-1.0f, 1.0f),
        0.0f
    ));
    
    // === AMP ENVELOPE ===
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::AMP_ATTACK, 1},
        "Amp Attack",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.0f, 0.4f),
        0.01f
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::AMP_DECAY, 1},
        "Amp Decay",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.0f, 0.4f),
        0.1f
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::AMP_SUSTAIN, 1},
        "Amp Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.8f
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::AMP_RELEASE, 1},
        "Amp Release",
        juce::NormalisableRange<float>(0.001f, 10.0f, 0.0f, 0.4f),
        0.3f
    ));
    
    // === FILTER ENVELOPE ===
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::FILTER_ATTACK, 1},
        "Filter Attack",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.0f, 0.4f),
        0.01f
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::FILTER_DECAY, 1},
        "Filter Decay",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.0f, 0.4f),
        0.3f
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::FILTER_SUSTAIN, 1},
        "Filter Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::FILTER_RELEASE, 1},
        "Filter Release",
        juce::NormalisableRange<float>(0.001f, 10.0f, 0.0f, 0.4f),
        0.5f
    ));
    
    // === LFO 1 ===
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::LFO1_RATE, 1},
        "LFO1 Rate",
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.0f, 0.4f),
        1.0f
    ));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ParamID::LFO1_WAVEFORM, 1},
        "LFO1 Waveform",
        juce::StringArray{"Sine", "Triangle", "Saw", "Square"},
        0
    ));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ParamID::LFO1_SYNC, 1},
        "LFO1 Sync",
        false
    ));
    
    // === LFO 2 ===
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::LFO2_RATE, 1},
        "LFO2 Rate",
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.0f, 0.4f),
        1.0f
    ));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ParamID::LFO2_WAVEFORM, 1},
        "LFO2 Waveform",
        juce::StringArray{"Sine", "Triangle", "Saw", "Square"},
        0
    ));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ParamID::LFO2_SYNC, 1},
        "LFO2 Sync",
        false
    ));
    
    // === MASTER ===
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::MASTER_GAIN, 1},
        "Master Gain",
        juce::NormalisableRange<float>(-60.0f, 6.0f),
        -6.0f
    ));
    
    // === DISTORTION ===
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ParamID::DIST_ENABLE, 1},
        "Distortion Enable",
        false
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::DIST_DRIVE, 1},
        "Distortion Drive",
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.0f, 0.5f),
        1.0f
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::DIST_MIX, 1},
        "Distortion Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f
    ));
    
    // === CHORUS ===
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ParamID::CHORUS_ENABLE, 1},
        "Chorus Enable",
        false
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::CHORUS_RATE, 1},
        "Chorus Rate",
        juce::NormalisableRange<float>(0.1f, 5.0f),
        1.0f
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::CHORUS_DEPTH, 1},
        "Chorus Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::CHORUS_MIX, 1},
        "Chorus Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f
    ));
    
    // === DELAY ===
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ParamID::DELAY_ENABLE, 1},
        "Delay Enable",
        false
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::DELAY_TIME, 1},
        "Delay Time",
        juce::NormalisableRange<float>(10.0f, 1000.0f, 0.0f, 0.4f),
        250.0f
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::DELAY_FEEDBACK, 1},
        "Delay Feedback",
        juce::NormalisableRange<float>(0.0f, 0.95f),
        0.3f
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::DELAY_MIX, 1},
        "Delay Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.3f
    ));
    
    // === REVERB ===
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ParamID::REVERB_ENABLE, 1},
        "Reverb Enable",
        false
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::REVERB_SIZE, 1},
        "Reverb Size",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::REVERB_DAMP, 1},
        "Reverb Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::REVERB_MIX, 1},
        "Reverb Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.3f
    ));
    
    // === OTT ===
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ParamID::OTT_ENABLE, 1},
        "OTT Enable",
        false
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::OTT_DEPTH, 1},
        "OTT Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::OTT_TIME, 1},
        "OTT Time",
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.0f, 0.5f),
        10.0f
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::OTT_MIX, 1},
        "OTT Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f
    ));
    
    // === ADVANCED FILTERS ===
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ParamID::FILTER_MODE, 1},
        "Filter Mode",
        juce::StringArray{"SVF", "Formant", "Comb", "Notch"},
        0
    ));
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{ParamID::FORMANT_VOWEL, 1},
        "Formant Vowel",
        0, 4, 0  // A, E, I, O, U
    ));
    
    // === MODULATION MATRIX (8 slots) ===
    juce::StringArray modSourceNames {"None", "LFO1", "LFO2", "AmpEnv", "FilterEnv", "Velocity", "ModWheel", "Aftertouch", "SB.A", "SB.B", "SB.C", "SB.D"};
    juce::StringArray modDestNames {"None", "Osc1Pitch", "Osc2Pitch", "Osc1Level", "Osc2Level", "SubLevel", "FilterCutoff", "FilterReso", "AmpLevel", "LFO1Rate", "LFO2Rate"};
    
    for (int i = 0; i < ParamID::NUM_MOD_SLOTS; ++i)
    {
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{ParamID::MOD_SRC_IDS[i], 1},
            "Mod " + juce::String(i + 1) + " Source",
            modSourceNames, 0));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{ParamID::MOD_DST_IDS[i], 1},
            "Mod " + juce::String(i + 1) + " Destination",
            modDestNames, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ParamID::MOD_AMT_IDS[i], 1},
            "Mod " + juce::String(i + 1) + " Amount",
            juce::NormalisableRange<float>(-1.0f, 1.0f),
            0.0f));
    }
    
    // === SPELLBOOK ===
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ParamID::SPELLBOOK_SHAPE, 1},
        "Spellbook Shape",
        juce::StringArray{"Circle", "Triangle", "Square", "Pentagon", "Star", "Spiral", "Lemniscate"},
        0
    ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::SPELLBOOK_RATE, 1},
        "Spellbook Rate",
        juce::NormalisableRange<float>(0.01f, 20000.0f, 0.0f, 0.3f),
        1.0f
    ));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ParamID::SPELLBOOK_SYNC, 1},
        "Spellbook Sync",
        false
    ));
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{ParamID::SPELLBOOK_NUM_OUTPUTS, 1},
        "Spellbook Num Outputs",
        1, 16, 8
    ));
    
    return { params.begin(), params.end() };
}

} // namespace kndl
