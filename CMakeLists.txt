cmake_minimum_required(VERSION 3.21)

project(kndl_synth VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use ccache if available for faster rebuilds
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
endif()

# JUCE (as submodule)
add_subdirectory(juce)

juce_add_plugin(KndlSynth
    COMPANY_NAME "kndl"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE

    FORMATS VST3 Standalone
    PRODUCT_NAME "KndlSynth"
)

juce_generate_juce_header(KndlSynth)

target_sources(KndlSynth PRIVATE
    plugin/PluginProcessor.cpp
    plugin/PluginProcessor.h
    plugin/PluginEditor.cpp
    plugin/PluginEditor.h
    plugin/PresetManager.h
    plugin/Logger.h
    
    # DSP Core (cpp files for incremental builds)
    dsp/KndlSynth.cpp
    dsp/KndlSynth.h
    dsp/core/Voice.cpp
    dsp/core/Voice.h
    dsp/core/Parameters.h
    dsp/core/VoiceManager.h
    dsp/core/ModulationMatrix.h
    
    # Oscillators (header-only, small files)
    dsp/oscillators/Oscillator.h
    dsp/oscillators/BasicOscillator.h
    dsp/oscillators/SubOscillator.h
    
    # Filters (header-only, small files)
    dsp/filters/Filter.h
    dsp/filters/SVFFilter.h
    
    # Modulators (header-only, small files)
    dsp/modulators/Envelope.h
    dsp/modulators/LFO.h
    
    # Effects
    dsp/effects/Effect.h
    dsp/effects/Delay.h
    dsp/effects/Chorus.h
    dsp/effects/Distortion.h
    dsp/effects/Reverb.h
    dsp/effects/DCBlocker.h
    
    # UI System
    ui/skins/Theme.h
    ui/skins/ArrakisTheme.h
    ui/components/KndlKnob.h
    ui/components/KndlSlider.h
    ui/components/KndlPanel.h
    ui/components/KndlScope.h
    ui/components/KndlOscSection.h
    ui/components/KndlEffectSection.h
    ui/components/KndlPresetSelector.h
    ui/layout/LayoutManager.h
)

target_include_directories(KndlSynth PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(KndlSynth PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(KndlSynth PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

