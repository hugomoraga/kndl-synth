#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "../ui/skins/Theme.h"
#include "../ui/skins/ArrakisTheme.h"
#include "../ui/components/KndlKnob.h"
#include "../ui/components/KndlSlider.h"
#include "../ui/components/KndlPanel.h"
#include "../ui/components/KndlScope.h"
#include "../ui/components/KndlOscSection.h"
#include "../ui/components/KndlEffectSection.h"
#include "../ui/components/KndlPresetSelector.h"
#include "../ui/layout/LayoutManager.h"
#include "Logger.h"

class KndlSynthAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    explicit KndlSynthAudioProcessorEditor (KndlSynthAudioProcessor&);
    ~KndlSynthAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void applyThemeToAllComponents();
    void drawBackground(juce::Graphics& g);
    void drawTopBar(juce::Graphics& g, juce::Rectangle<int> bounds);
    
    KndlSynthAudioProcessor& audioProcessor;
    
    // Theme system
    kndl::ui::ArrakisTheme arrakisTheme;
    const kndl::ui::Theme* currentTheme = nullptr;
    
    // Layout
    kndl::ui::LayoutManager layoutManager;
    kndl::ui::LayoutManager::Bounds layoutBounds;
    
    // Oscillator sections (individual panels)
    kndl::ui::KndlOscSection osc1Section { "OSC 1" };
    kndl::ui::KndlOscSection osc2Section { "OSC 2" };
    kndl::ui::KndlSubSection subSection;
    
    // Other panels
    kndl::ui::KndlPanel filterPanel { "FILTER" };
    kndl::ui::KndlPanel envPanel { "AMP" };
    kndl::ui::KndlPanel filterEnvPanel { "FLT ENV" };
    kndl::ui::KndlPanel lfoPanel { "LFO" };
    kndl::ui::KndlPanel monitorPanel { "MONITOR" };
    
    // Scope and data display
    kndl::ui::KndlScope waveScope;
    kndl::ui::KndlDataDisplay dataDisplay;
    
    // Preset selector
    kndl::ui::KndlPresetSelector presetSelector;
    
    // Filter controls
    kndl::ui::KndlKnob filterCutoffKnob { "CUTOFF" };
    kndl::ui::KndlKnob filterResoKnob { "RESO" };
    kndl::ui::KndlKnob filterDriveKnob { "DRIVE" };
    kndl::ui::KndlKnob filterEnvKnob { "ENV" };
    
    // Amp Envelope controls (ADSR sliders)
    kndl::ui::KndlSlider ampAttackSlider { "A" };
    kndl::ui::KndlSlider ampDecaySlider { "D" };
    kndl::ui::KndlSlider ampSustainSlider { "S" };
    kndl::ui::KndlSlider ampReleaseSlider { "R" };
    
    // Filter Envelope controls
    kndl::ui::KndlSlider filterAttackSlider { "A" };
    kndl::ui::KndlSlider filterDecaySlider { "D" };
    kndl::ui::KndlSlider filterSustainSlider { "S" };
    kndl::ui::KndlSlider filterReleaseSlider { "R" };
    
    // LFO controls
    kndl::ui::KndlKnob lfo1RateKnob { "LFO1" };
    kndl::ui::KndlKnob lfo2RateKnob { "LFO2" };
    
    // Master output
    kndl::ui::KndlKnob masterGainKnob { "MASTER" };
    
    // Macro knobs (top bar)
    std::array<kndl::ui::KndlKnob, 6> macroKnobs;
    
    // Parameter attachments
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    
    // OSC1 attachments
    std::unique_ptr<SliderAttachment> osc1LevelAttachment;
    std::unique_ptr<SliderAttachment> osc1DetuneAttachment;
    std::unique_ptr<SliderAttachment> osc1OctaveAttachment;
    std::unique_ptr<ComboBoxAttachment> osc1WaveformAttachment;
    
    // OSC2 attachments
    std::unique_ptr<SliderAttachment> osc2LevelAttachment;
    std::unique_ptr<SliderAttachment> osc2DetuneAttachment;
    std::unique_ptr<SliderAttachment> osc2OctaveAttachment;
    std::unique_ptr<ComboBoxAttachment> osc2WaveformAttachment;
    
    // Sub attachments
    std::unique_ptr<SliderAttachment> subLevelAttachment;
    std::unique_ptr<SliderAttachment> subOctaveAttachment;
    
    // Filter attachments
    std::unique_ptr<SliderAttachment> filterCutoffAttachment;
    std::unique_ptr<SliderAttachment> filterResoAttachment;
    std::unique_ptr<SliderAttachment> filterDriveAttachment;
    std::unique_ptr<SliderAttachment> filterEnvAttachment;
    
    // Amp Env attachments
    std::unique_ptr<SliderAttachment> ampAttackAttachment;
    std::unique_ptr<SliderAttachment> ampDecayAttachment;
    std::unique_ptr<SliderAttachment> ampSustainAttachment;
    std::unique_ptr<SliderAttachment> ampReleaseAttachment;
    
    // Filter Env attachments
    std::unique_ptr<SliderAttachment> filterAttackAttachment;
    std::unique_ptr<SliderAttachment> filterDecayAttachment;
    std::unique_ptr<SliderAttachment> filterSustainAttachment;
    std::unique_ptr<SliderAttachment> filterReleaseAttachment;
    
    // LFO attachments
    std::unique_ptr<SliderAttachment> lfo1RateAttachment;
    std::unique_ptr<SliderAttachment> lfo2RateAttachment;
    
    // Master gain attachment
    std::unique_ptr<SliderAttachment> masterGainAttachment;
    
    // Effects sections
    kndl::ui::KndlEffectSection distortionSection { "DIST", {"DRV", "MIX"} };
    kndl::ui::KndlEffectSection chorusSection { "CHORUS", {"RATE", "DPT", "MIX"} };
    kndl::ui::KndlEffectSection delaySection { "DELAY", {"TIME", "FB", "MIX"} };
    kndl::ui::KndlEffectSection reverbSection { "REVERB", {"SIZE", "DMP", "MIX"} };
    
    // Effect attachments
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    
    std::unique_ptr<ButtonAttachment> distEnableAttachment;
    std::unique_ptr<SliderAttachment> distDriveAttachment;
    std::unique_ptr<SliderAttachment> distMixAttachment;
    
    std::unique_ptr<ButtonAttachment> chorusEnableAttachment;
    std::unique_ptr<SliderAttachment> chorusRateAttachment;
    std::unique_ptr<SliderAttachment> chorusDepthAttachment;
    std::unique_ptr<SliderAttachment> chorusMixAttachment;
    
    std::unique_ptr<ButtonAttachment> delayEnableAttachment;
    std::unique_ptr<SliderAttachment> delayTimeAttachment;
    std::unique_ptr<SliderAttachment> delayFeedbackAttachment;
    std::unique_ptr<SliderAttachment> delayMixAttachment;
    
    std::unique_ptr<ButtonAttachment> reverbEnableAttachment;
    std::unique_ptr<SliderAttachment> reverbSizeAttachment;
    std::unique_ptr<SliderAttachment> reverbDampAttachment;
    std::unique_ptr<SliderAttachment> reverbMixAttachment;
    
    // State
    kndl::DebugInfo cachedDebugInfo;
    int displayNote = -1;
    bool midiIndicatorOn = false;
    int midiIndicatorCountdown = 0;
    
    // Logging button
    juce::TextButton logButton { "LOG" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KndlSynthAudioProcessorEditor)
};
