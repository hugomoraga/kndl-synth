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
    
    // Add master gain knob
    addAndMakeVisible(masterGainKnob);
    
    // Add scope and data display
    addAndMakeVisible(waveScope);
    addAndMakeVisible(dataDisplay);
    
    // Add effect sections
    addAndMakeVisible(distortionSection);
    addAndMakeVisible(chorusSection);
    addAndMakeVisible(delaySection);
    addAndMakeVisible(reverbSection);
    
    // Add preset selector
    addAndMakeVisible(presetSelector);
    presetSelector.setPresetManager(&audioProcessor.getPresetManager());
    
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
    osc1LevelAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::OSC1_LEVEL, osc1Section.getLevelSlider());
    osc1DetuneAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::OSC1_DETUNE, osc1Section.getDetuneSlider());
    osc1OctaveAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::OSC1_OCTAVE, osc1Section.getOctaveSlider());
    osc1WaveformAttachment = std::make_unique<ComboBoxAttachment>(apvts, kndl::ParamID::OSC1_WAVEFORM, osc1Section.getWaveformSelector());
    
    // OSC2 attachments
    osc2LevelAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::OSC2_LEVEL, osc2Section.getLevelSlider());
    osc2DetuneAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::OSC2_DETUNE, osc2Section.getDetuneSlider());
    osc2OctaveAttachment = std::make_unique<SliderAttachment>(apvts, kndl::ParamID::OSC2_OCTAVE, osc2Section.getOctaveSlider());
    osc2WaveformAttachment = std::make_unique<ComboBoxAttachment>(apvts, kndl::ParamID::OSC2_WAVEFORM, osc2Section.getWaveformSelector());
    
    // Sub attachments
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
    
    // Set size
    setSize (1000, 750);
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
    
    // Master gain
    masterGainKnob.setTheme(currentTheme);
    
    // Scope and data display
    waveScope.setTheme(currentTheme);
    dataDisplay.setTheme(currentTheme);
    
    // Effect sections
    distortionSection.setTheme(currentTheme);
    chorusSection.setTheme(currentTheme);
    delaySection.setTheme(currentTheme);
    reverbSection.setTheme(currentTheme);
    
    // Preset selector
    presetSelector.setTheme(currentTheme);
    
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
    dataDisplay.setOutputLevel(cachedDebugInfo.masterOutput);
    
    waveScope.pushSample(cachedDebugInfo.masterOutput);
    
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
    
    // Voice count
    int voiceCount = audioProcessor.getActiveVoiceCount();
    g.setColour(voiceCount > 0 ? currentTheme->getAccentPrimary() : currentTheme->getTextMuted());
    g.drawText(juce::String(voiceCount) + "v", bounds.getRight() - 45, bounds.getY(), 40, bounds.getHeight(), juce::Justification::centredRight);
}

void KndlSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    drawBackground(g);
    drawTopBar(g, layoutBounds.topBar);
}

void KndlSynthAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    const int margin = 10;
    const int gap = 6;
    
    auto workArea = bounds.reduced(margin);
    
    // === TOP BAR (50px) ===
    auto topBar = workArea.removeFromTop(50);
    workArea.removeFromTop(gap);
    layoutBounds.topBar = topBar;
    
    // Preset selector in top bar (after title area)
    auto presetArea = topBar;
    presetArea.removeFromLeft(80);  // Space for title
    presetArea = presetArea.removeFromLeft(220);  // Width for preset selector
    presetSelector.setBounds(presetArea.reduced(4, 10));
    
    // Master gain knob (right side of top bar, before log button)
    int masterKnobSize = 50;
    int masterKnobHeight = masterKnobSize + 14;
    masterGainKnob.setBounds(topBar.getRight() - 190, topBar.getY() + (topBar.getHeight() - masterKnobHeight) / 2, masterKnobSize, masterKnobHeight);
    
    // Log button (right side of top bar)
    logButton.setBounds(topBar.getRight() - 70, topBar.getY() + 12, 55, 26);
    
    // Macro knobs in top bar (after title/preset area)
    auto macroArea = topBar;
    macroArea.removeFromLeft(310);  // Title + preset selector
    macroArea.removeFromRight(180); // Space for status + log button
    
    int macroKnobSize = 55;
    int macroKnobHeight = macroKnobSize + 14;
    int macroSpacing = (macroArea.getWidth() - macroKnobSize * 6) / 7;
    int macroY = macroArea.getY() + (macroArea.getHeight() - macroKnobHeight) / 2;
    
    for (size_t i = 0; i < macroKnobs.size(); ++i)
    {
        int x = macroArea.getX() + macroSpacing + static_cast<int>(i) * (macroKnobSize + macroSpacing);
        macroKnobs[i].setBounds(x, macroY, macroKnobSize, macroKnobHeight);
    }
    
    // === BOTTOM (Monitor + Effects) ===
    int bottomHeight = workArea.getHeight() * 38 / 100;
    auto bottomArea = workArea.removeFromBottom(bottomHeight);
    workArea.removeFromBottom(gap);
    
    // Split bottom into effects (left) and monitor (right)
    int effectsWidth = bottomArea.getWidth() * 28 / 100;
    auto effectsArea = bottomArea.removeFromLeft(effectsWidth);
    bottomArea.removeFromLeft(gap);
    
    // Effects - 4 stacked sections
    int effectHeight = (effectsArea.getHeight() - gap * 3) / 4;
    
    distortionSection.setBounds(effectsArea.removeFromTop(effectHeight));
    effectsArea.removeFromTop(gap);
    chorusSection.setBounds(effectsArea.removeFromTop(effectHeight));
    effectsArea.removeFromTop(gap);
    delaySection.setBounds(effectsArea.removeFromTop(effectHeight));
    effectsArea.removeFromTop(gap);
    reverbSection.setBounds(effectsArea);
    
    // Monitor panel
    monitorPanel.setBounds(bottomArea);
    
    auto monitorContent = juce::Rectangle<int>(
        bottomArea.getX() + 10,
        bottomArea.getY() + 28,
        bottomArea.getWidth() - 20,
        bottomArea.getHeight() - 36
    );
    
    int scopeWidth = monitorContent.getWidth() * 40 / 100;
    waveScope.setBounds(monitorContent.removeFromLeft(scopeWidth).reduced(2));
    monitorContent.removeFromLeft(8);
    dataDisplay.setBounds(monitorContent.reduced(2));
    
    // === MIDDLE SECTION ===
    // Left column: Oscillators (3 stacked)
    // Center: Filter
    // Right: Envelopes + LFO
    
    int leftWidth = workArea.getWidth() * 22 / 100;
    int rightWidth = workArea.getWidth() * 30 / 100;
    
    auto leftArea = workArea.removeFromLeft(leftWidth);
    workArea.removeFromLeft(gap);
    auto rightArea = workArea.removeFromRight(rightWidth);
    workArea.removeFromRight(gap);
    auto centerArea = workArea;
    
    // === LEFT: Oscillators (stacked vertically) ===
    int oscHeight = (leftArea.getHeight() - gap * 2) / 3;
    
    osc1Section.setBounds(leftArea.removeFromTop(oscHeight));
    leftArea.removeFromTop(gap);
    osc2Section.setBounds(leftArea.removeFromTop(oscHeight));
    leftArea.removeFromTop(gap);
    subSection.setBounds(leftArea);
    
    // === CENTER: Filter ===
    filterPanel.setBounds(centerArea);
    
    int filterKnobSize = 60;
    int filterKnobHeight = filterKnobSize + 16;
    int filterGap = 14;
    
    int totalFilterWidth = filterKnobSize * 4 + filterGap * 3;
    int filterStartX = centerArea.getX() + (centerArea.getWidth() - totalFilterWidth) / 2;
    int filterY = centerArea.getY() + (centerArea.getHeight() - filterKnobHeight) / 2;
    
    filterCutoffKnob.setBounds(filterStartX, filterY, filterKnobSize, filterKnobHeight);
    filterResoKnob.setBounds(filterStartX + filterKnobSize + filterGap, filterY, filterKnobSize, filterKnobHeight);
    filterDriveKnob.setBounds(filterStartX + (filterKnobSize + filterGap) * 2, filterY, filterKnobSize, filterKnobHeight);
    filterEnvKnob.setBounds(filterStartX + (filterKnobSize + filterGap) * 3, filterY, filterKnobSize, filterKnobHeight);
    
    // === RIGHT: Envelopes + LFO ===
    int envHeight = (rightArea.getHeight() - gap * 2) * 40 / 100;
    
    auto ampEnvArea = rightArea.removeFromTop(envHeight);
    rightArea.removeFromTop(gap);
    auto filterEnvArea = rightArea.removeFromTop(envHeight);
    rightArea.removeFromTop(gap);
    auto lfoArea = rightArea;
    
    envPanel.setBounds(ampEnvArea);
    filterEnvPanel.setBounds(filterEnvArea);
    lfoPanel.setBounds(lfoArea);
    
    // Amp ADSR sliders
    int sliderWidth = 26;
    int sliderGap = 6;
    int sliderHeight = ampEnvArea.getHeight() - 45;
    
    int totalSliderWidth = sliderWidth * 4 + sliderGap * 3;
    int sliderStartX = ampEnvArea.getX() + (ampEnvArea.getWidth() - totalSliderWidth) / 2;
    int sliderY = ampEnvArea.getY() + 32;
    
    ampAttackSlider.setBounds(sliderStartX, sliderY, sliderWidth, sliderHeight);
    ampDecaySlider.setBounds(sliderStartX + sliderWidth + sliderGap, sliderY, sliderWidth, sliderHeight);
    ampSustainSlider.setBounds(sliderStartX + (sliderWidth + sliderGap) * 2, sliderY, sliderWidth, sliderHeight);
    ampReleaseSlider.setBounds(sliderStartX + (sliderWidth + sliderGap) * 3, sliderY, sliderWidth, sliderHeight);
    
    // Filter ADSR sliders
    sliderHeight = filterEnvArea.getHeight() - 45;
    sliderStartX = filterEnvArea.getX() + (filterEnvArea.getWidth() - totalSliderWidth) / 2;
    sliderY = filterEnvArea.getY() + 32;
    
    filterAttackSlider.setBounds(sliderStartX, sliderY, sliderWidth, sliderHeight);
    filterDecaySlider.setBounds(sliderStartX + sliderWidth + sliderGap, sliderY, sliderWidth, sliderHeight);
    filterSustainSlider.setBounds(sliderStartX + (sliderWidth + sliderGap) * 2, sliderY, sliderWidth, sliderHeight);
    filterReleaseSlider.setBounds(sliderStartX + (sliderWidth + sliderGap) * 3, sliderY, sliderWidth, sliderHeight);
    
    // LFO knobs
    int lfoKnobSize = 48;
    int lfoKnobHeight = lfoKnobSize + 16;
    int lfoKnobGap = 20;
    
    int totalLfoWidth = lfoKnobSize * 2 + lfoKnobGap;
    int lfoStartX = lfoArea.getX() + (lfoArea.getWidth() - totalLfoWidth) / 2;
    int lfoY = lfoArea.getY() + (lfoArea.getHeight() - lfoKnobHeight) / 2 + 8;
    
    lfo1RateKnob.setBounds(lfoStartX, lfoY, lfoKnobSize, lfoKnobHeight);
    lfo2RateKnob.setBounds(lfoStartX + lfoKnobSize + lfoKnobGap, lfoY, lfoKnobSize, lfoKnobHeight);
}
