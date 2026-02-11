#include "PluginEditor.h"
#include "../dsp/core/Parameters.h"

KndlSynthAudioProcessorEditor::KndlSynthAudioProcessorEditor (KndlSynthAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Set theme
    currentTheme = &arrakisTheme;
    
    // Initialize macro knobs
    for (size_t i = 0; i < macroKnobs.size(); ++i)
    {
        macroKnobs[i].setLabel("M" + juce::String(static_cast<int>(i) + 1));
        addAndMakeVisible(macroKnobs[i]);
    }
    
    // Add oscillator sections
    addAndMakeVisible(osc1Section);
    addAndMakeVisible(osc2Section);
    addAndMakeVisible(subSection);
    
    // Add other panels
    addAndMakeVisible(filterPanel);
    addAndMakeVisible(envPanel);
    addAndMakeVisible(filterEnvPanel);
    addAndMakeVisible(lfoPanel);
    addAndMakeVisible(monitorPanel);
    
    // Add filter knobs
    addAndMakeVisible(filterCutoffKnob);
    addAndMakeVisible(filterResoKnob);
    addAndMakeVisible(filterDriveKnob);
    addAndMakeVisible(filterEnvKnob);
    
    // Add amp envelope sliders
    addAndMakeVisible(ampAttackSlider);
    addAndMakeVisible(ampDecaySlider);
    addAndMakeVisible(ampSustainSlider);
    addAndMakeVisible(ampReleaseSlider);
    
    // Add filter envelope sliders
    addAndMakeVisible(filterAttackSlider);
    addAndMakeVisible(filterDecaySlider);
    addAndMakeVisible(filterSustainSlider);
    addAndMakeVisible(filterReleaseSlider);
    
    // Add LFO knobs
    addAndMakeVisible(lfo1RateKnob);
    addAndMakeVisible(lfo2RateKnob);
    
    // LFO waveform selectors
    addAndMakeVisible(lfo1WaveformSelector);
    lfo1WaveformSelector.addItem("SIN", 1);
    lfo1WaveformSelector.addItem("TRI", 2);
    lfo1WaveformSelector.addItem("SAW", 3);
    lfo1WaveformSelector.addItem("SQR", 4);
    lfo1WaveformSelector.setScrollWheelEnabled(false);
    
    addAndMakeVisible(lfo2WaveformSelector);
    lfo2WaveformSelector.addItem("SIN", 1);
    lfo2WaveformSelector.addItem("TRI", 2);
    lfo2WaveformSelector.addItem("SAW", 3);
    lfo2WaveformSelector.addItem("SQR", 4);
    lfo2WaveformSelector.setScrollWheelEnabled(false);
    
    // Add Orbit controls (inside modulators panel)
    addAndMakeVisible(orbitRateKnob);
    addAndMakeVisible(orbitShapeSelector);
    orbitShapeSelector.addItem("Circle", 1);
    orbitShapeSelector.addItem("Triangle", 2);
    orbitShapeSelector.addItem("Square", 3);
    orbitShapeSelector.addItem("Pentagon", 4);
    orbitShapeSelector.addItem("Star", 5);
    orbitShapeSelector.addItem("Spiral", 6);
    orbitShapeSelector.addItem("Lemniscate", 7);
    orbitShapeSelector.setScrollWheelEnabled(false);
    
    // Add filter mode selectors
    addAndMakeVisible(filterModeSelector);
    filterModeSelector.addItem("SVF", 1);
    filterModeSelector.addItem("Formant", 2);
    filterModeSelector.addItem("Comb", 3);
    filterModeSelector.addItem("Notch", 4);
    filterModeSelector.setScrollWheelEnabled(false);
    
    addAndMakeVisible(formantVowelSelector);
    formantVowelSelector.addItem("A", 1);
    formantVowelSelector.addItem("E", 2);
    formantVowelSelector.addItem("I", 3);
    formantVowelSelector.addItem("O", 4);
    formantVowelSelector.addItem("U", 5);
    formantVowelSelector.setScrollWheelEnabled(false);
    
    // Add master gain knob
    addAndMakeVisible(masterGainKnob);
    
    // Add scope, filter display and data display
    addAndMakeVisible(waveScope);
    addAndMakeVisible(filterDisplay);
    addAndMakeVisible(orbitScope);
    addAndMakeVisible(dataDisplay);
    
    // Add modulation matrix display
    modMatrixDisplay = std::make_unique<kndl::ui::KndlModMatrix>(audioProcessor.getModMatrix());
    addAndMakeVisible(*modMatrixDisplay);
    modMatrixDisplay->connectToAPVTS(audioProcessor.getAPVTS());
    
    // Add effect sections (all visible, tab system hides inactive)
    addAndMakeVisible(distortionSection);
    addAndMakeVisible(chorusSection);
    addAndMakeVisible(delaySection);
    addAndMakeVisible(reverbSection);
    addAndMakeVisible(ottSection);
    
    // FX tab buttons
    const juce::String fxTabNames[] = { "DIST", "CHORUS", "DELAY", "REVERB", "OTT" };
    for (int i = 0; i < 5; ++i)
    {
        fxTabButtons[static_cast<size_t>(i)].setButtonText(fxTabNames[i]);
        fxTabButtons[static_cast<size_t>(i)].setClickingTogglesState(false);
        fxTabButtons[static_cast<size_t>(i)].onClick = [this, i]() { selectFxTab(i); };
        addAndMakeVisible(fxTabButtons[static_cast<size_t>(i)]);
    }
    selectFxTab(0);
    
    // Add preset selector
    addAndMakeVisible(presetSelector);
    presetSelector.setPresetManager(&audioProcessor.getPresetManager());
    
    // Add sequencer controls
    addAndMakeVisible(seqButton);
    seqButton.setClickingTogglesState(true);
    seqButton.setToggleState(false, juce::dontSendNotification);
    seqButton.onClick = [this]() {
        bool enabled = seqButton.getToggleState();
        audioProcessor.getSequencer().setEnabled(enabled);
        seqButton.setButtonText(enabled ? "SEQ ON" : "SEQ");
        seqPatternSelector.setVisible(enabled);
        seqTempoSlider.setVisible(enabled);
        seqOctaveSlider.setVisible(enabled);
        seqTempoLabel.setVisible(enabled);
        resized(); // Re-layout to show/hide sequencer bar
    };
    
    addAndMakeVisible(seqPatternSelector);
    for (int i = 0; i < static_cast<int>(kndl::InternalSequencer::Pattern::NumPatterns); ++i)
    {
        auto pattern = static_cast<kndl::InternalSequencer::Pattern>(i);
        seqPatternSelector.addItem(kndl::InternalSequencer::getPatternName(pattern), i + 1);
    }
    seqPatternSelector.setSelectedId(static_cast<int>(kndl::InternalSequencer::Pattern::MinorArpeggio) + 1, juce::dontSendNotification);
    seqPatternSelector.onChange = [this]() {
        auto pattern = static_cast<kndl::InternalSequencer::Pattern>(seqPatternSelector.getSelectedId() - 1);
        audioProcessor.getSequencer().setPattern(pattern);
    };
    seqPatternSelector.setVisible(false);
    
    addAndMakeVisible(seqTempoSlider);
    seqTempoSlider.setRange(40.0, 300.0, 1.0);
    seqTempoSlider.setValue(120.0, juce::dontSendNotification);
    seqTempoSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    seqTempoSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    seqTempoSlider.setTextValueSuffix(" bpm");
    seqTempoSlider.onValueChange = [this]() {
        audioProcessor.getSequencer().setTempo(seqTempoSlider.getValue());
    };
    seqTempoSlider.setVisible(false);
    
    addAndMakeVisible(seqOctaveSlider);
    seqOctaveSlider.setRange(1, 7, 1);
    seqOctaveSlider.setValue(3, juce::dontSendNotification);
    seqOctaveSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    seqOctaveSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 25, 20);
    seqOctaveSlider.onValueChange = [this]() {
        audioProcessor.getSequencer().setBaseOctave(static_cast<int>(seqOctaveSlider.getValue()));
    };
    seqOctaveSlider.setVisible(false);
    
    addAndMakeVisible(seqTempoLabel);
    seqTempoLabel.setText("OCT", juce::dontSendNotification);
    seqTempoLabel.setJustificationType(juce::Justification::centredRight);
    seqTempoLabel.setVisible(false);
    
    // Add logging button
    addAndMakeVisible(logButton);
    logButton.setClickingTogglesState(true);
    logButton.setToggleState(false, juce::dontSendNotification);
    logButton.onClick = [this]() {
        bool enabled = logButton.getToggleState();
        kndl::Logger::getInstance().setEnabled(enabled);
        logButton.setButtonText(enabled ? "LOG ON" : "LOG");
        if (enabled)
        {
            KNDL_LOG_INFO("Logging enabled by user");
        }
    };
    
    // Apply theme to all components
    applyThemeToAllComponents();
    
    // Create parameter attachments
    auto& apvts = audioProcessor.getAPVTS();
    
    // OSC1 attachments
    osc1EnableAttachment = std::make_unique<ButtonAttachment>(apvts, kndl::ParamID::OSC1_ENABLE, osc1Section.getEnableButton());
    osc1LevelAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::OSC1_LEVEL, osc1Section.getLevelSlider());
    osc1DetuneAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::OSC1_DETUNE, osc1Section.getDetuneSlider());
    osc1OctaveAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::OSC1_OCTAVE, osc1Section.getOctaveSlider());
    osc1WaveformAttachment = std::make_unique<ComboBoxAttachment>(apvts, kndl::ParamID::OSC1_WAVEFORM, osc1Section.getWaveformSelector());
    
    // OSC2 attachments
    osc2EnableAttachment = std::make_unique<ButtonAttachment>(apvts, kndl::ParamID::OSC2_ENABLE, osc2Section.getEnableButton());
    osc2LevelAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::OSC2_LEVEL, osc2Section.getLevelSlider());
    osc2DetuneAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::OSC2_DETUNE, osc2Section.getDetuneSlider());
    osc2OctaveAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::OSC2_OCTAVE, osc2Section.getOctaveSlider());
    osc2WaveformAttachment = std::make_unique<ComboBoxAttachment>(apvts, kndl::ParamID::OSC2_WAVEFORM, osc2Section.getWaveformSelector());
    
    // Sub attachments
    subEnableAttachment = std::make_unique<ButtonAttachment>(apvts, kndl::ParamID::SUB_ENABLE, subSection.getEnableButton());
    subLevelAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::SUB_LEVEL, subSection.getLevelSlider());
    subOctaveAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::SUB_OCTAVE, subSection.getOctaveSlider());
    
    // Filter attachments
    filterCutoffAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::FILTER_CUTOFF, filterCutoffKnob);
    filterResoAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::FILTER_RESONANCE, filterResoKnob);
    filterDriveAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::FILTER_DRIVE, filterDriveKnob);
    filterEnvAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::FILTER_ENV_AMOUNT, filterEnvKnob);
    
    // Amp Env attachments
    ampAttackAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::AMP_ATTACK, ampAttackSlider);
    ampDecayAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::AMP_DECAY, ampDecaySlider);
    ampSustainAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::AMP_SUSTAIN, ampSustainSlider);
    ampReleaseAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::AMP_RELEASE, ampReleaseSlider);
    
    // Filter Env attachments
    filterAttackAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::FILTER_ATTACK, filterAttackSlider);
    filterDecayAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::FILTER_DECAY, filterDecaySlider);
    filterSustainAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::FILTER_SUSTAIN, filterSustainSlider);
    filterReleaseAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::FILTER_RELEASE, filterReleaseSlider);
    
    // LFO attachments
    lfo1RateAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::LFO1_RATE, lfo1RateKnob);
    lfo2RateAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::LFO2_RATE, lfo2RateKnob);
    lfo1WaveformAttachment = std::make_unique<ComboBoxAttachment>(apvts, kndl::ParamID::LFO1_WAVEFORM, lfo1WaveformSelector);
    lfo2WaveformAttachment = std::make_unique<ComboBoxAttachment>(apvts, kndl::ParamID::LFO2_WAVEFORM, lfo2WaveformSelector);
    
    // Orbit attachments
    orbitRateAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::ORBIT_RATE, orbitRateKnob);
    orbitShapeAttachment = std::make_unique<ComboBoxAttachment>(apvts, kndl::ParamID::ORBIT_SHAPE, orbitShapeSelector);
    
    // Filter mode attachments
    filterModeAttachment = std::make_unique<ComboBoxAttachment>(apvts, kndl::ParamID::FILTER_MODE, filterModeSelector);
    formantVowelAttachment = std::make_unique<ComboBoxAttachment>(apvts, kndl::ParamID::FORMANT_VOWEL, formantVowelSelector);
    
    // Master gain attachment
    masterGainAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::MASTER_GAIN, masterGainKnob);
    
    // Effect attachments
    distEnableAttachment = std::make_unique<ButtonAttachment>(apvts, kndl::ParamID::DIST_ENABLE, distortionSection.getEnableButton());
    distDriveAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::DIST_DRIVE, distortionSection.getKnob(0));
    distMixAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::DIST_MIX, distortionSection.getKnob(1));
    
    chorusEnableAttachment = std::make_unique<ButtonAttachment>(apvts, kndl::ParamID::CHORUS_ENABLE, chorusSection.getEnableButton());
    chorusRateAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::CHORUS_RATE, chorusSection.getKnob(0));
    chorusDepthAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::CHORUS_DEPTH, chorusSection.getKnob(1));
    chorusMixAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::CHORUS_MIX, chorusSection.getKnob(2));
    
    delayEnableAttachment = std::make_unique<ButtonAttachment>(apvts, kndl::ParamID::DELAY_ENABLE, delaySection.getEnableButton());
    delayTimeAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::DELAY_TIME, delaySection.getKnob(0));
    delayFeedbackAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::DELAY_FEEDBACK, delaySection.getKnob(1));
    delayMixAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::DELAY_MIX, delaySection.getKnob(2));
    
    reverbEnableAttachment = std::make_unique<ButtonAttachment>(apvts, kndl::ParamID::REVERB_ENABLE, reverbSection.getEnableButton());
    reverbSizeAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::REVERB_SIZE, reverbSection.getKnob(0));
    reverbDampAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::REVERB_DAMP, reverbSection.getKnob(1));
    reverbMixAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::REVERB_MIX, reverbSection.getKnob(2));
    
    ottEnableAttachment = std::make_unique<ButtonAttachment>(apvts, kndl::ParamID::OTT_ENABLE, ottSection.getEnableButton());
    ottDepthAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::OTT_DEPTH, ottSection.getKnob(0));
    ottTimeAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::OTT_TIME, ottSection.getKnob(1));
    ottMixAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::OTT_MIX, ottSection.getKnob(2));
    
    // Set size
    setSize (1000, 820);
    startTimerHz(30);
}

KndlSynthAudioProcessorEditor::~KndlSynthAudioProcessorEditor()
{
    stopTimer();
}

void KndlSynthAudioProcessorEditor::applyThemeToAllComponents()
{
    // Oscillator sections
    osc1Section.setTheme(currentTheme);
    osc2Section.setTheme(currentTheme);
    subSection.setTheme(currentTheme);
    
    // Panels
    filterPanel.setTheme(currentTheme);
    envPanel.setTheme(currentTheme);
    filterEnvPanel.setTheme(currentTheme);
    lfoPanel.setTheme(currentTheme);
    monitorPanel.setTheme(currentTheme);
    
    // Macro knobs
    for (auto& knob : macroKnobs)
        knob.setTheme(currentTheme);
    
    // Filter knobs
    filterCutoffKnob.setTheme(currentTheme);
    filterResoKnob.setTheme(currentTheme);
    filterDriveKnob.setTheme(currentTheme);
    filterEnvKnob.setTheme(currentTheme);
    
    // Envelope sliders
    ampAttackSlider.setTheme(currentTheme);
    ampDecaySlider.setTheme(currentTheme);
    ampSustainSlider.setTheme(currentTheme);
    ampReleaseSlider.setTheme(currentTheme);
    filterAttackSlider.setTheme(currentTheme);
    filterDecaySlider.setTheme(currentTheme);
    filterSustainSlider.setTheme(currentTheme);
    filterReleaseSlider.setTheme(currentTheme);
    
    // LFO knobs
    lfo1RateKnob.setTheme(currentTheme);
    lfo2RateKnob.setTheme(currentTheme);
    
    // Orbit (inside modulators panel)
    orbitRateKnob.setTheme(currentTheme);
    if (currentTheme)
    {
        for (auto* combo : { &lfo1WaveformSelector, &lfo2WaveformSelector, &orbitShapeSelector })
        {
            combo->setColour(juce::ComboBox::backgroundColourId, currentTheme->getPanelBackground());
            combo->setColour(juce::ComboBox::textColourId, currentTheme->getTextPrimary());
            combo->setColour(juce::ComboBox::outlineColourId, currentTheme->getPanelBorder());
            combo->setColour(juce::ComboBox::arrowColourId, currentTheme->getAccentTertiary());
        }
        
        filterModeSelector.setColour(juce::ComboBox::backgroundColourId, currentTheme->getPanelBackground());
        filterModeSelector.setColour(juce::ComboBox::textColourId, currentTheme->getTextPrimary());
        filterModeSelector.setColour(juce::ComboBox::outlineColourId, currentTheme->getPanelBorder());
        filterModeSelector.setColour(juce::ComboBox::arrowColourId, currentTheme->getAccentPrimary());
        
        formantVowelSelector.setColour(juce::ComboBox::backgroundColourId, currentTheme->getPanelBackground());
        formantVowelSelector.setColour(juce::ComboBox::textColourId, currentTheme->getTextPrimary());
        formantVowelSelector.setColour(juce::ComboBox::outlineColourId, currentTheme->getPanelBorder());
        formantVowelSelector.setColour(juce::ComboBox::arrowColourId, currentTheme->getAccentPrimary());
    }
    
    // Master gain
    masterGainKnob.setTheme(currentTheme);
    
    // Scope and data display
    waveScope.setTheme(currentTheme);
    filterDisplay.setTheme(currentTheme);
    orbitScope.setTheme(currentTheme);
    dataDisplay.setTheme(currentTheme);
    
    if (modMatrixDisplay)
        modMatrixDisplay->setTheme(currentTheme);
    
    // Effect sections
    distortionSection.setTheme(currentTheme);
    chorusSection.setTheme(currentTheme);
    delaySection.setTheme(currentTheme);
    reverbSection.setTheme(currentTheme);
    ottSection.setTheme(currentTheme);
    
    // FX tab buttons styling
    selectFxTab(selectedFxTab);
    
    // Preset selector
    presetSelector.setTheme(currentTheme);
    
    // Sequencer controls
    if (currentTheme)
    {
        seqButton.setColour(juce::TextButton::buttonColourId, currentTheme->getPanelBackground());
        seqButton.setColour(juce::TextButton::buttonOnColourId, currentTheme->getAccentTertiary().withAlpha(0.3f));
        seqButton.setColour(juce::TextButton::textColourOffId, currentTheme->getTextMuted());
        seqButton.setColour(juce::TextButton::textColourOnId, currentTheme->getAccentTertiary());
        
        seqPatternSelector.setColour(juce::ComboBox::backgroundColourId, currentTheme->getPanelBackground());
        seqPatternSelector.setColour(juce::ComboBox::textColourId, currentTheme->getTextPrimary());
        seqPatternSelector.setColour(juce::ComboBox::outlineColourId, currentTheme->getPanelBorder());
        seqPatternSelector.setColour(juce::ComboBox::arrowColourId, currentTheme->getAccentTertiary());
        
        seqTempoSlider.setColour(juce::Slider::backgroundColourId, currentTheme->getSliderTrack());
        seqTempoSlider.setColour(juce::Slider::trackColourId, currentTheme->getAccentTertiary());
        seqTempoSlider.setColour(juce::Slider::thumbColourId, currentTheme->getSliderThumb());
        seqTempoSlider.setColour(juce::Slider::textBoxTextColourId, currentTheme->getTextSecondary());
        seqTempoSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        
        seqOctaveSlider.setColour(juce::Slider::backgroundColourId, currentTheme->getSliderTrack());
        seqOctaveSlider.setColour(juce::Slider::trackColourId, currentTheme->getAccentTertiary());
        seqOctaveSlider.setColour(juce::Slider::thumbColourId, currentTheme->getSliderThumb());
        seqOctaveSlider.setColour(juce::Slider::textBoxTextColourId, currentTheme->getTextSecondary());
        seqOctaveSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        
        seqTempoLabel.setColour(juce::Label::textColourId, currentTheme->getTextMuted());
        seqTempoLabel.setFont(currentTheme->getSmallFont());
    }
    
    // Log button
    if (currentTheme)
    {
        logButton.setColour(juce::TextButton::buttonColourId, currentTheme->getPanelBackground());
        logButton.setColour(juce::TextButton::buttonOnColourId, currentTheme->getWarning().withAlpha(0.3f));
        logButton.setColour(juce::TextButton::textColourOffId, currentTheme->getTextMuted());
        logButton.setColour(juce::TextButton::textColourOnId, currentTheme->getWarning());
    }
}

void KndlSynthAudioProcessorEditor::timerCallback()
{
    cachedDebugInfo = audioProcessor.getDebugInfo();
    
    if (audioProcessor.hasMidiActivity())
    {
        displayNote = audioProcessor.getLastMidiNote();
        midiIndicatorOn = true;
        midiIndicatorCountdown = 10;
        audioProcessor.clearMidiActivity();
    }
    
    if (midiIndicatorCountdown > 0)
    {
        midiIndicatorCountdown--;
        if (midiIndicatorCountdown == 0)
            midiIndicatorOn = false;
    }
    
    // Update data display
    dataDisplay.setNoteInfo(displayNote, cachedDebugInfo.ampEnvValue, audioProcessor.getActiveVoiceCount());
    dataDisplay.setOscValues(cachedDebugInfo.osc1Value, cachedDebugInfo.osc2Value, cachedDebugInfo.subValue);
    dataDisplay.setFilterValues(cachedDebugInfo.filterCutoff, cachedDebugInfo.filterOutput, cachedDebugInfo.filterEnvValue);
    dataDisplay.setEnvValues(cachedDebugInfo.ampEnvValue, cachedDebugInfo.filterEnvValue);
    dataDisplay.setLfoValues(cachedDebugInfo.lfo1Value, cachedDebugInfo.lfo2Value);
    dataDisplay.setOrbitValues(cachedDebugInfo.orbitA, cachedDebugInfo.orbitB,
                              cachedDebugInfo.orbitC, cachedDebugInfo.orbitD);
    
    // Feed Orbit scope (A=X, B=Y of first output pair)
    orbitScope.setCurrentXY(cachedDebugInfo.orbitA, cachedDebugInfo.orbitB);
    // Update shape name from the combo box
    orbitScope.currentShape = orbitShapeSelector.getSelectedItemIndex();
    orbitScope.setShapeName(orbitShapeSelector.getText());
    orbitScope.repaint();
    
    dataDisplay.setOutputLevel(cachedDebugInfo.masterOutput);
    
    waveScope.pushSample(cachedDebugInfo.masterOutput);
    
    // Update FX tab ON/OFF indicators
    if (currentTheme)
    {
        kndl::ui::KndlEffectSection* fxSections[] = {
            &distortionSection, &chorusSection, &delaySection, &reverbSection, &ottSection
        };
        for (int i = 0; i < 5; ++i)
        {
            if (i == selectedFxTab) continue;
            bool isOn = fxSections[i]->getEnableButton().getToggleState();
            fxTabButtons[static_cast<size_t>(i)].setColour(juce::TextButton::textColourOffId,
                isOn ? currentTheme->getAccentSecondary() : currentTheme->getTextMuted());
        }
    }
    
    // Update filter display with mode awareness
    auto& apvts = audioProcessor.getAPVTS();
    int filterType = static_cast<int>(*apvts.getRawParameterValue(kndl::ParamID::FILTER_TYPE));
    int filterMode = static_cast<int>(*apvts.getRawParameterValue(kndl::ParamID::FILTER_MODE));
    int formantVowel = static_cast<int>(*apvts.getRawParameterValue(kndl::ParamID::FORMANT_VOWEL));
    filterDisplay.updateFilterResponse(
        cachedDebugInfo.filterCutoff,
        cachedDebugInfo.filterResonance,
        filterType,
        filterMode,
        formantVowel
    );
    
    // Only show vowel selector when filter mode is Formant (mode 1)
    formantVowelSelector.setVisible(filterMode == 1);
    
    repaint();
}

void KndlSynthAudioProcessorEditor::drawBackground(juce::Graphics& g)
{
    // Main background gradient
    juce::ColourGradient bgGradient(
        currentTheme->getBackgroundPrimary(),
        0.0f, 0.0f,
        currentTheme->getBackgroundSecondary(),
        static_cast<float>(getWidth()), static_cast<float>(getHeight()),
        true
    );
    g.setGradientFill(bgGradient);
    g.fillAll();
    
    // Subtle texture overlay
    if (currentTheme->hasTexturedBackground())
    {
        juce::Random rng(123);
        g.setColour(currentTheme->getBackgroundTertiary().withAlpha(0.02f));
        
        for (int i = 0; i < 200; ++i)
        {
            float x = rng.nextFloat() * static_cast<float>(getWidth());
            float y = rng.nextFloat() * static_cast<float>(getHeight());
            float w = 2.0f + rng.nextFloat() * 8.0f;
            g.fillRect(x, y, w, 1.0f);
        }
    }
    
    // Vignette effect
    juce::ColourGradient vignette(
        juce::Colours::transparentBlack,
        getWidth() * 0.5f, getHeight() * 0.5f,
        juce::Colours::black.withAlpha(0.3f),
        0.0f, 0.0f,
        true
    );
    g.setGradientFill(vignette);
    g.fillAll();
}

void KndlSynthAudioProcessorEditor::drawTopBar(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // Title
    g.setColour(currentTheme->getTextPrimary());
    g.setFont(currentTheme->getTitleFont());
    g.drawText("KNDL", bounds.getX() + 15, bounds.getY(), 70, bounds.getHeight(), juce::Justification::centredLeft);
    
    // Preset selector is now a component, no need to draw here
    
    // Status indicators on right
    int statusX = bounds.getRight() - 120;
    
    // MIDI indicator
    juce::Rectangle<float> midiLight(static_cast<float>(statusX), static_cast<float>(bounds.getCentreY() - 6), 12.0f, 12.0f);
    
    if (midiIndicatorOn)
    {
        g.setColour(currentTheme->getPositive());
        g.fillEllipse(midiLight);
        g.setColour(currentTheme->getPositive().withAlpha(0.3f));
        g.fillEllipse(midiLight.expanded(4.0f));
    }
    else
    {
        g.setColour(currentTheme->getTextMuted());
        g.fillEllipse(midiLight);
    }
    
    g.setColour(currentTheme->getTextSecondary());
    g.setFont(currentTheme->getSmallFont());
    g.drawText("MIDI", statusX + 18, bounds.getY(), 40, bounds.getHeight(), juce::Justification::centredLeft);
    
    // Limiter indicator
    bool isLimiting = cachedDebugInfo.isLimiting;
    float grDb = cachedDebugInfo.gainReductionDb;
    int limiterX = statusX + 56;
    
    juce::Rectangle<float> limiterLight(static_cast<float>(limiterX), static_cast<float>(bounds.getCentreY() - 6), 12.0f, 12.0f);
    
    if (isLimiting)
    {
        // Red glow when limiting, brighter with more GR
        float intensity = juce::jlimit(0.3f, 1.0f, std::abs(grDb) / 6.0f);
        auto limColor = currentTheme->getNegative().withAlpha(intensity);
        g.setColour(limColor);
        g.fillEllipse(limiterLight);
        g.setColour(limColor.withAlpha(0.3f));
        g.fillEllipse(limiterLight.expanded(3.0f));
    }
    else
    {
        g.setColour(currentTheme->getTextMuted().withAlpha(0.4f));
        g.fillEllipse(limiterLight);
    }
    
    g.setColour(isLimiting ? currentTheme->getNegative() : currentTheme->getTextMuted().withAlpha(0.5f));
    g.setFont(currentTheme->getSmallFont());
    g.drawText("LIM", limiterX + 16, bounds.getY(), 30, bounds.getHeight(), juce::Justification::centredLeft);
    
    // Voice count
    int voiceCount = audioProcessor.getActiveVoiceCount();
    g.setColour(voiceCount > 0 ? currentTheme->getAccentPrimary() : currentTheme->getTextMuted());
    g.drawText(juce::String(voiceCount) + "v", bounds.getRight() - 45, bounds.getY(), 40, bounds.getHeight(), juce::Justification::centredRight);
}

void KndlSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    drawBackground(g);
    drawTopBar(g, layoutBounds.topBar);
    
    // Draw sequencer bar background when active
    if (audioProcessor.getSequencer().isEnabled())
    {
        auto seqBarArea = juce::Rectangle<int>(
            layoutBounds.topBar.getX(),
            layoutBounds.topBar.getBottom() + 6,
            layoutBounds.topBar.getWidth(),
            28
        );
        
        g.setColour(currentTheme->getPanelBackground().withAlpha(0.6f));
        g.fillRoundedRectangle(seqBarArea.toFloat(), 4.0f);
        g.setColour(currentTheme->getAccentTertiary().withAlpha(0.3f));
        g.drawRoundedRectangle(seqBarArea.toFloat(), 4.0f, 1.0f);
    }
}

void KndlSynthAudioProcessorEditor::resized()
{
    using namespace kndl::ui;
    auto g = KndlGrid(getLocalBounds().reduced(10), 6);
    
    // === MAIN VERTICAL STRUCTURE ===
    auto main = audioProcessor.getSequencer().isEnabled()
        ? g.rows({ px(64), px(28), fr(65), fr(35) })
        : g.rows({ px(64), fr(65), fr(35) });
    
    int row = 0;
    layoutBounds.topBar = main[row];
    layoutTopBar(main[row++]);
    
    if (audioProcessor.getSequencer().isEnabled())
        layoutSeqBar(main[row++]);
    
    auto middle = main[row++];
    auto bottom = main[row++];
    
    // === MIDDLE: 3 columns (osc | filter+modulators | right) ===
    auto mid = KndlGrid(middle, 6).cols({ 3, 4, 5 });
    
    // Oscillators: 3 equal rows
    auto oscs = mid.sub(0).equalRows(3);
    osc1Section.setBounds(oscs[0]);
    osc2Section.setBounds(oscs[1]);
    subSection.setBounds(oscs[2]);
    
    // Center column: Filter on top, Modulators below (1/3 height = same as sub osc row)
    auto center = mid.sub(1).rows({ fr(2), fr(1) });
    layoutFilter(center[0]);
    layoutModulators(center[1]);
    
    // Right column: envelopes side by side + mod matrix below
    auto right = mid.sub(2).rows({ fr(40), fr(60) });
    auto envs = KndlGrid(right[0], 6).equalCols(2);
    layoutAmpEnv(envs[0]);
    layoutFilterEnv(envs[1]);
    if (modMatrixDisplay)
        modMatrixDisplay->setBounds(right[1].reduced(4));
    
    // === BOTTOM: effects + monitor ===
    auto bot = KndlGrid(bottom, 6).cols({ 3, 9 });
    layoutEffects(bot[0]);
    layoutMonitor(bot[1]);
}

// =============================================================================
// Layout Helpers
// =============================================================================

void KndlSynthAudioProcessorEditor::layoutTopBar(juce::Rectangle<int> area)
{
    // Preset selector (after title)
    auto presetArea = area;
    presetArea.removeFromLeft(80);
    presetArea = presetArea.removeFromLeft(220);
    presetSelector.setBounds(presetArea.reduced(4, 10));
    
    // Master gain knob
    int knobSize = 50;
    int knobH = knobSize + 14;
    masterGainKnob.setBounds(area.getRight() - 250,
        area.getY() + (area.getHeight() - knobH) / 2, knobSize, knobH);
    
    // SEQ + LOG buttons
    seqButton.setBounds(area.getRight() - 130, area.getY() + 12, 50, 26);
    logButton.setBounds(area.getRight() - 70, area.getY() + 12, 55, 26);
    
    // Macro knobs (fill space between preset and master)
    auto macroArea = area;
    macroArea.removeFromLeft(310);
    macroArea.removeFromRight(250);
    
    int macroSize = 55;
    int macroH = macroSize + 14;
    int spacing = (macroArea.getWidth() - macroSize * 6) / 7;
    int macroY = macroArea.getY() + (macroArea.getHeight() - macroH) / 2;
    
    for (size_t i = 0; i < macroKnobs.size(); ++i)
    {
        int x = macroArea.getX() + spacing + static_cast<int>(i) * (macroSize + spacing);
        macroKnobs[i].setBounds(x, macroY, macroSize, macroH);
    }
}

void KndlSynthAudioProcessorEditor::layoutSeqBar(juce::Rectangle<int> area)
{
    int x = area.getX() + 10;
    int y = area.getY() + 2;
    int h = 24;
    
    seqPatternSelector.setBounds(x, y, 110, h);
    x += 118;
    seqTempoSlider.setBounds(x, y, 200, h);
    x += 208;
    seqTempoLabel.setBounds(x, y, 28, h);
    x += 30;
    seqOctaveSlider.setBounds(x, y, 80, h);
}

void KndlSynthAudioProcessorEditor::layoutFilter(juce::Rectangle<int> area)
{
    filterPanel.setBounds(area);
    
    // Mode selectors at top
    int selY = area.getY() + 28;
    int modeW = 90, vowelW = 60, selH = 24, gap = 6;
    int totalW = modeW + gap + vowelW;
    int cx = area.getX() + (area.getWidth() - totalW) / 2;
    filterModeSelector.setBounds(cx, selY, modeW, selH);
    formantVowelSelector.setBounds(cx + modeW + gap, selY, vowelW, selH);
    
    // Knobs centered below selectors
    int knobSize = 56, knobH = knobSize + 16, knobGap = 12;
    int totalKnobW = knobSize * 4 + knobGap * 3;
    int startX = area.getX() + (area.getWidth() - totalKnobW) / 2;
    int knobY = area.getY() + selH + 36
        + (area.getHeight() - selH - 36 - knobH) / 2;
    
    filterCutoffKnob.setBounds(startX, knobY, knobSize, knobH);
    filterResoKnob.setBounds(startX + (knobSize + knobGap), knobY, knobSize, knobH);
    filterDriveKnob.setBounds(startX + (knobSize + knobGap) * 2, knobY, knobSize, knobH);
    filterEnvKnob.setBounds(startX + (knobSize + knobGap) * 3, knobY, knobSize, knobH);
}

void KndlSynthAudioProcessorEditor::layoutAmpEnv(juce::Rectangle<int> area)
{
    envPanel.setBounds(area);
    
    int sw = 26, sg = 6, sh = area.getHeight() - 45;
    int totalW = sw * 4 + sg * 3;
    int sx = area.getX() + (area.getWidth() - totalW) / 2;
    int sy = area.getY() + 32;
    
    ampAttackSlider.setBounds(sx, sy, sw, sh);
    ampDecaySlider.setBounds(sx + (sw + sg), sy, sw, sh);
    ampSustainSlider.setBounds(sx + (sw + sg) * 2, sy, sw, sh);
    ampReleaseSlider.setBounds(sx + (sw + sg) * 3, sy, sw, sh);
}

void KndlSynthAudioProcessorEditor::layoutFilterEnv(juce::Rectangle<int> area)
{
    filterEnvPanel.setBounds(area);
    
    int sw = 26, sg = 6, sh = area.getHeight() - 45;
    int totalW = sw * 4 + sg * 3;
    int sx = area.getX() + (area.getWidth() - totalW) / 2;
    int sy = area.getY() + 32;
    
    filterAttackSlider.setBounds(sx, sy, sw, sh);
    filterDecaySlider.setBounds(sx + (sw + sg), sy, sw, sh);
    filterSustainSlider.setBounds(sx + (sw + sg) * 2, sy, sw, sh);
    filterReleaseSlider.setBounds(sx + (sw + sg) * 3, sy, sw, sh);
}

void KndlSynthAudioProcessorEditor::layoutModulators(juce::Rectangle<int> area)
{
    using namespace kndl::ui;
    lfoPanel.setBounds(area);
    
    // Content area after panel header
    auto content = juce::Rectangle<int>(
        area.getX() + 6, area.getY() + 22,
        area.getWidth() - 12, area.getHeight() - 26);
    
    // 4 columns: LFO1 | LFO2 | SB Shape | SB Rate
    auto cols = KndlGrid(content, 4).equalCols(4);
    
    // LFO1: knob on top, waveform selector below
    {
        auto c = cols[0];
        int knobSize = juce::jmin(38, c.getWidth() - 4);
        int knobH = knobSize + 12;
        int selH = 20;
        int totalH = knobH + 4 + selH;
        int startY = c.getY() + (c.getHeight() - totalH) / 2;
        int cx = c.getX() + (c.getWidth() - knobSize) / 2;
        lfo1RateKnob.setBounds(cx, startY, knobSize, knobH);
        int selW = c.getWidth() - 4;
        lfo1WaveformSelector.setBounds(c.getX() + 2, startY + knobH + 4, selW, selH);
    }
    
    // LFO2: same layout
    {
        auto c = cols[1];
        int knobSize = juce::jmin(38, c.getWidth() - 4);
        int knobH = knobSize + 12;
        int selH = 20;
        int totalH = knobH + 4 + selH;
        int startY = c.getY() + (c.getHeight() - totalH) / 2;
        int cx = c.getX() + (c.getWidth() - knobSize) / 2;
        lfo2RateKnob.setBounds(cx, startY, knobSize, knobH);
        int selW = c.getWidth() - 4;
        lfo2WaveformSelector.setBounds(c.getX() + 2, startY + knobH + 4, selW, selH);
    }
    
    // Orbit shape selector (centered vertically)
    {
        auto c = cols[2];
        int selH = 20;
        int selW = c.getWidth() - 4;
        orbitShapeSelector.setBounds(c.getX() + 2, c.getY() + (c.getHeight() - selH) / 2, selW, selH);
    }
    
    // Orbit rate knob
    {
        auto c = cols[3];
        int knobSize = juce::jmin(38, c.getWidth() - 4);
        int knobH = knobSize + 12;
        int cx = c.getX() + (c.getWidth() - knobSize) / 2;
        int cy = c.getY() + (c.getHeight() - knobH) / 2;
        orbitRateKnob.setBounds(cx, cy, knobSize, knobH);
    }
}

void KndlSynthAudioProcessorEditor::selectFxTab(int index)
{
    selectedFxTab = juce::jlimit(0, 4, index);
    
    kndl::ui::KndlEffectSection* sections[] = {
        &distortionSection, &chorusSection, &delaySection, &reverbSection, &ottSection
    };
    
    for (int i = 0; i < 5; ++i)
    {
        sections[i]->setVisible(i == selectedFxTab);
        
        // Update tab button appearance
        if (currentTheme)
        {
            bool isSelected = (i == selectedFxTab);
            bool isOn = sections[i]->getEnableButton().getToggleState();
            
            auto& btn = fxTabButtons[static_cast<size_t>(i)];
            
            if (isSelected)
            {
                btn.setColour(juce::TextButton::buttonColourId, currentTheme->getAccentPrimary().withAlpha(0.25f));
                btn.setColour(juce::TextButton::textColourOffId, currentTheme->getTextPrimary());
            }
            else
            {
                btn.setColour(juce::TextButton::buttonColourId, currentTheme->getPanelBackground().withAlpha(0.5f));
                btn.setColour(juce::TextButton::textColourOffId,
                    isOn ? currentTheme->getAccentSecondary() : currentTheme->getTextMuted());
            }
        }
    }
    
    resized();
    repaint();
}

void KndlSynthAudioProcessorEditor::layoutEffects(juce::Rectangle<int> area)
{
    using namespace kndl::ui;
    
    // Tab bar at top
    int tabH = 22;
    auto tabBar = area.removeFromTop(tabH);
    int tabW = tabBar.getWidth() / 5;
    
    for (int i = 0; i < 5; ++i)
    {
        fxTabButtons[static_cast<size_t>(i)].setBounds(
            tabBar.getX() + i * tabW, tabBar.getY(), tabW, tabH);
    }
    
    // Content: only the selected section fills the remaining space
    auto content = area.reduced(0, 2);
    
    kndl::ui::KndlEffectSection* sections[] = {
        &distortionSection, &chorusSection, &delaySection, &reverbSection, &ottSection
    };
    
    sections[selectedFxTab]->setBounds(content);
}

void KndlSynthAudioProcessorEditor::layoutMonitor(juce::Rectangle<int> area)
{
    using namespace kndl::ui;
    monitorPanel.setBounds(area);
    
    // Content inside the panel (after header, with padding)
    auto content = juce::Rectangle<int>(
        area.getX() + 10, area.getY() + 28,
        area.getWidth() - 20, area.getHeight() - 36);
    
    auto parts = KndlGrid(content, 6).cols({ 2, 2, 2, 5 });
    waveScope.setBounds(parts[0].reduced(2));
    filterDisplay.setBounds(parts[1].reduced(2));
    orbitScope.setBounds(parts[2].reduced(2));
    dataDisplay.setBounds(parts[3].reduced(2));
}
