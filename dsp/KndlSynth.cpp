#include "KndlSynth.h"

namespace kndl {

KndlSynth::KndlSynth(juce::AudioProcessorValueTreeState& apvts)
    : parameters(apvts)
{
    cacheParameterPointers();
}

void KndlSynth::prepare(double newSampleRate, int newSamplesPerBlock)
{
    sampleRate = newSampleRate;
    samplesPerBlock = newSamplesPerBlock;
    
    KNDL_LOG_INFO("Audio prepared: sampleRate=" + juce::String(newSampleRate) + 
                  " blockSize=" + juce::String(newSamplesPerBlock));
    
    voiceManager.prepare(newSampleRate, newSamplesPerBlock);
    lfo1.prepare(newSampleRate);
    lfo2.prepare(newSampleRate);
    spellbook.prepare(newSampleRate);
    
    // Prepare effects
    distortion.prepare(newSampleRate, newSamplesPerBlock);
    chorus.prepare(newSampleRate, newSamplesPerBlock);
    delay.prepare(newSampleRate, newSamplesPerBlock);
    reverb.prepare(newSampleRate, newSamplesPerBlock);
    ott.prepare(newSampleRate, newSamplesPerBlock);
    
    // Prepare modulation matrix
    modMatrix.prepare(newSampleRate);
    
    // Prepare DC blockers
    dcBlockerL.prepare(newSampleRate);
    dcBlockerR.prepare(newSampleRate);
    
    masterGain.reset(newSampleRate, 0.02);
}

void KndlSynth::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    
    if (numSamples == 0 || numChannels == 0)
        return;
    
    updateParametersFromAPVTS();
    
    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;
    
    int sampleIndex = 0;
    
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();
        const int messagePosition = metadata.samplePosition;
        
        while (sampleIndex < messagePosition && sampleIndex < numSamples)
        {
            float sample = processSample();
            leftChannel[sampleIndex] = sample;
            if (rightChannel)
                rightChannel[sampleIndex] = sample;
            sampleIndex++;
        }
        
        handleMidiMessage(message);
    }
    
    while (sampleIndex < numSamples)
    {
        float sample = processSample();
        leftChannel[sampleIndex] = sample;
        if (rightChannel)
            rightChannel[sampleIndex] = sample;
        sampleIndex++;
    }
}

void KndlSynth::handleMidiMessage(const juce::MidiMessage& message)
{
    auto& logger = Logger::getInstance();
    
    if (message.isNoteOn())
    {
        logger.logMidiEvent("NoteOn", message.getNoteNumber(), message.getFloatVelocity());
        voiceManager.noteOn(message.getNoteNumber(), message.getFloatVelocity());
    }
    else if (message.isNoteOff())
    {
        logger.logMidiEvent("NoteOff", message.getNoteNumber(), 0.0f);
        voiceManager.noteOff(message.getNoteNumber());
    }
    else if (message.isAllNotesOff() || message.isAllSoundOff())
    {
        KNDL_LOG_INFO("MIDI: All notes off");
        voiceManager.allNotesOff();
    }
    else if (message.isController())
    {
        if (message.getControllerNumber() == 1)
        {
            modWheelValue = message.getControllerValue() / 127.0f;
            KNDL_LOG_DEBUG("MIDI: ModWheel=" + juce::String(modWheelValue, 2));
        }
    }
}

float KndlSynth::processSample()
{
    // 1. Feed non-LFO sources into mod matrix (from previous sample state)
    modMatrix.setSourceValue(ModSource::ModWheel, modWheelValue);
    modMatrix.setSourceValue(ModSource::AmpEnv, debugInfo.ampEnvValue);
    modMatrix.setSourceValue(ModSource::FilterEnv, debugInfo.filterEnvValue);
    modMatrix.setSourceValue(ModSource::Velocity, voiceManager.getLastVelocity());
    
    // Feed Spellbook outputs (from previous process() call - Spellbook doesn't depend on LFO)
    modMatrix.setSourceValue(ModSource::SpellbookA, spellbook.getOutput(0));
    modMatrix.setSourceValue(ModSource::SpellbookB, spellbook.getOutput(1));
    modMatrix.setSourceValue(ModSource::SpellbookC, spellbook.getOutput(2));
    modMatrix.setSourceValue(ModSource::SpellbookD, spellbook.getOutput(3));
    
    // Feed LFO values from previous sample (so rate mod can affect THIS sample's LFO tick)
    modMatrix.setSourceValue(ModSource::LFO1, lfo1.getCurrentValue());
    modMatrix.setSourceValue(ModSource::LFO2, lfo2.getCurrentValue());
    
    // 2. Advance smoothers exactly once per sample
    modMatrix.updateSmoothing();
    
    // 3. Apply LFO rate modulation BEFORE processing LFOs
    float lfo1RateMod = modMatrix.getModulationAmount(ModDestination::LFO1Rate);
    float lfo2RateMod = modMatrix.getModulationAmount(ModDestination::LFO2Rate);
    if (std::abs(lfo1RateMod) > 0.001f)
        lfo1.setRate(*lfo1RateParam + lfo1RateMod * 10.0f);
    if (std::abs(lfo2RateMod) > 0.001f)
        lfo2.setRate(*lfo2RateParam + lfo2RateMod * 10.0f);
    
    // 4. Now process LFOs with correct modulated rate
    float lfo1Value = lfo1.process();
    float lfo2Value = lfo2.process();
    
    // 5. Update LFO source values for next sample's mod matrix
    modMatrix.setSourceValue(ModSource::LFO1, lfo1Value);
    modMatrix.setSourceValue(ModSource::LFO2, lfo2Value);
    
    // 6. Process Spellbook modulator
    spellbook.process();
    float sbA = spellbook.getOutput(0);
    float sbB = spellbook.getOutput(1);
    float sbC = spellbook.getOutput(2);
    float sbD = spellbook.getOutput(3);
    
    // Apply all modulation destinations
    float osc1PitchMod = modMatrix.getModulationAmount(ModDestination::Osc1Pitch);
    float osc2PitchMod = modMatrix.getModulationAmount(ModDestination::Osc2Pitch);
    voiceManager.applyPitchMod(osc1PitchMod);
    voiceManager.applyOsc2PitchMod(osc2PitchMod);
    
    voiceManager.setOsc1LevelMod(modMatrix.getModulationAmount(ModDestination::Osc1Level));
    voiceManager.setOsc2LevelMod(modMatrix.getModulationAmount(ModDestination::Osc2Level));
    voiceManager.setSubLevelMod(modMatrix.getModulationAmount(ModDestination::SubLevel));
    voiceManager.setFilterCutoffMod(modMatrix.getModulationAmount(ModDestination::FilterCutoff));
    voiceManager.setFilterResoMod(modMatrix.getModulationAmount(ModDestination::FilterResonance));
    voiceManager.setAmpLevelMod(modMatrix.getModulationAmount(ModDestination::AmpLevel));
    
    float output = voiceManager.process();
    
    // Update debug info from voice manager
    const auto& voiceDebug = voiceManager.getDebugInfo();
    debugInfo.osc1Value = voiceDebug.osc1Value;
    debugInfo.osc2Value = voiceDebug.osc2Value;
    debugInfo.subValue = voiceDebug.subValue;
    debugInfo.mixedOsc = voiceDebug.mixedOsc;
    debugInfo.filterInput = voiceDebug.filterInput;
    debugInfo.filterOutput = voiceDebug.filterOutput;
    debugInfo.filterCutoff = voiceDebug.filterCutoff;
    debugInfo.filterResonance = *filterResonanceParam;
    debugInfo.ampEnvValue = voiceDebug.ampEnvValue;
    debugInfo.filterEnvValue = voiceDebug.filterEnvValue;
    debugInfo.voiceOutput = voiceDebug.output;
    
    // LFO values
    debugInfo.lfo1Value = lfo1Value;
    debugInfo.lfo2Value = lfo2Value;
    
    // Spellbook values
    debugInfo.spellbookA = sbA;
    debugInfo.spellbookB = sbB;
    debugInfo.spellbookC = sbC;
    debugInfo.spellbookD = sbD;
    
    // Check for NaN/Inf
    debugInfo.hasNaN = !std::isfinite(output);
    debugInfo.hasInf = std::isinf(output);
    
    // Protección contra NaN/Inf
    if (!std::isfinite(output))
    {
        Logger::getInstance().logAudioAnomaly("NaN/Inf in voice output", output);
        output = 0.0f;
    }
    
    // Log DSP values periodically
    Logger::getInstance().logDSPValues(
        debugInfo.osc1Value, debugInfo.osc2Value, debugInfo.subValue,
        debugInfo.filterOutput, debugInfo.ampEnvValue
    );
    
    // Apply effects chain: Distortion -> Chorus -> Delay -> Reverb -> OTT
    output = distortion.process(output);
    output = chorus.process(output);
    output = delay.process(output);
    output = reverb.process(output);
    output = ott.process(output);
    
    // NaN/Inf check after effects chain (critical: delay feedback can propagate NaN)
    if (!std::isfinite(output))
    {
        Logger::getInstance().logAudioAnomaly("NaN/Inf after effects chain", output);
        output = 0.0f;
    }
    
    // Remove DC offset before gain stage
    output = dcBlockerL.process(output);
    
    float gain = masterGain.getNextValue();
    output *= gain;
    
    // Soft clipping final (with less aggressive limiting)
    bool clipped = false;
    if (output > 1.0f)
    {
        output = 1.0f - std::exp(-(output - 1.0f));
        clipped = true;
    }
    else if (output < -1.0f)
    {
        output = -1.0f + std::exp(-(-output - 1.0f));
        clipped = true;
    }
    
    // Log audio stats periodically
    Logger::getInstance().logAudioStats(
        std::abs(output), 
        debugInfo.masterOutput,
        voiceManager.getActiveVoiceCount(),
        clipped
    );
    
    debugInfo.masterOutput = output;
    
    return output;
}

void KndlSynth::cacheParameterPointers()
{
    osc1WaveformParam = parameters.getRawParameterValue(ParamID::OSC1_WAVEFORM);
    osc1LevelParam = parameters.getRawParameterValue(ParamID::OSC1_LEVEL);
    osc1DetuneParam = parameters.getRawParameterValue(ParamID::OSC1_DETUNE);
    osc1OctaveParam = parameters.getRawParameterValue(ParamID::OSC1_OCTAVE);
    
    osc2WaveformParam = parameters.getRawParameterValue(ParamID::OSC2_WAVEFORM);
    osc2LevelParam = parameters.getRawParameterValue(ParamID::OSC2_LEVEL);
    osc2DetuneParam = parameters.getRawParameterValue(ParamID::OSC2_DETUNE);
    osc2OctaveParam = parameters.getRawParameterValue(ParamID::OSC2_OCTAVE);
    
    subLevelParam = parameters.getRawParameterValue(ParamID::SUB_LEVEL);
    subOctaveParam = parameters.getRawParameterValue(ParamID::SUB_OCTAVE);
    
    filterCutoffParam = parameters.getRawParameterValue(ParamID::FILTER_CUTOFF);
    filterResonanceParam = parameters.getRawParameterValue(ParamID::FILTER_RESONANCE);
    filterTypeParam = parameters.getRawParameterValue(ParamID::FILTER_TYPE);
    filterDriveParam = parameters.getRawParameterValue(ParamID::FILTER_DRIVE);
    filterEnvAmountParam = parameters.getRawParameterValue(ParamID::FILTER_ENV_AMOUNT);
    
    ampAttackParam = parameters.getRawParameterValue(ParamID::AMP_ATTACK);
    ampDecayParam = parameters.getRawParameterValue(ParamID::AMP_DECAY);
    ampSustainParam = parameters.getRawParameterValue(ParamID::AMP_SUSTAIN);
    ampReleaseParam = parameters.getRawParameterValue(ParamID::AMP_RELEASE);
    
    filterAttackParam = parameters.getRawParameterValue(ParamID::FILTER_ATTACK);
    filterDecayParam = parameters.getRawParameterValue(ParamID::FILTER_DECAY);
    filterSustainParam = parameters.getRawParameterValue(ParamID::FILTER_SUSTAIN);
    filterReleaseParam = parameters.getRawParameterValue(ParamID::FILTER_RELEASE);
    
    lfo1RateParam = parameters.getRawParameterValue(ParamID::LFO1_RATE);
    lfo1WaveformParam = parameters.getRawParameterValue(ParamID::LFO1_WAVEFORM);
    lfo1SyncParam = parameters.getRawParameterValue(ParamID::LFO1_SYNC);
    
    lfo2RateParam = parameters.getRawParameterValue(ParamID::LFO2_RATE);
    lfo2WaveformParam = parameters.getRawParameterValue(ParamID::LFO2_WAVEFORM);
    lfo2SyncParam = parameters.getRawParameterValue(ParamID::LFO2_SYNC);
    
    masterGainParam = parameters.getRawParameterValue(ParamID::MASTER_GAIN);
    
    // Advanced filters
    filterModeParam = parameters.getRawParameterValue(ParamID::FILTER_MODE);
    formantVowelParam = parameters.getRawParameterValue(ParamID::FORMANT_VOWEL);
    
    // Spellbook
    spellbookShapeParam = parameters.getRawParameterValue(ParamID::SPELLBOOK_SHAPE);
    spellbookRateParam = parameters.getRawParameterValue(ParamID::SPELLBOOK_RATE);
    spellbookSyncParam = parameters.getRawParameterValue(ParamID::SPELLBOOK_SYNC);
    spellbookNumOutputsParam = parameters.getRawParameterValue(ParamID::SPELLBOOK_NUM_OUTPUTS);
    
    // Effects
    distortionEnableParam = parameters.getRawParameterValue(ParamID::DIST_ENABLE);
    distortionDriveParam = parameters.getRawParameterValue(ParamID::DIST_DRIVE);
    distortionMixParam = parameters.getRawParameterValue(ParamID::DIST_MIX);
    
    chorusEnableParam = parameters.getRawParameterValue(ParamID::CHORUS_ENABLE);
    chorusRateParam = parameters.getRawParameterValue(ParamID::CHORUS_RATE);
    chorusDepthParam = parameters.getRawParameterValue(ParamID::CHORUS_DEPTH);
    chorusMixParam = parameters.getRawParameterValue(ParamID::CHORUS_MIX);
    
    delayEnableParam = parameters.getRawParameterValue(ParamID::DELAY_ENABLE);
    delayTimeParam = parameters.getRawParameterValue(ParamID::DELAY_TIME);
    delayFeedbackParam = parameters.getRawParameterValue(ParamID::DELAY_FEEDBACK);
    delayMixParam = parameters.getRawParameterValue(ParamID::DELAY_MIX);
    
    reverbEnableParam = parameters.getRawParameterValue(ParamID::REVERB_ENABLE);
    reverbSizeParam = parameters.getRawParameterValue(ParamID::REVERB_SIZE);
    reverbDampParam = parameters.getRawParameterValue(ParamID::REVERB_DAMP);
    reverbMixParam = parameters.getRawParameterValue(ParamID::REVERB_MIX);
    
    ottEnableParam = parameters.getRawParameterValue(ParamID::OTT_ENABLE);
    ottDepthParam = parameters.getRawParameterValue(ParamID::OTT_DEPTH);
    ottTimeParam = parameters.getRawParameterValue(ParamID::OTT_TIME);
    ottMixParam = parameters.getRawParameterValue(ParamID::OTT_MIX);
    
    // Mod matrix
    for (int i = 0; i < ParamID::NUM_MOD_SLOTS; ++i)
    {
        modSrcParams[static_cast<size_t>(i)] = parameters.getRawParameterValue(ParamID::MOD_SRC_IDS[i]);
        modDstParams[static_cast<size_t>(i)] = parameters.getRawParameterValue(ParamID::MOD_DST_IDS[i]);
        modAmtParams[static_cast<size_t>(i)] = parameters.getRawParameterValue(ParamID::MOD_AMT_IDS[i]);
    }
}

void KndlSynth::updateParametersFromAPVTS()
{
    voiceManager.setOsc1Waveform(static_cast<Waveform>(static_cast<int>(*osc1WaveformParam)));
    voiceManager.setOsc1Level(*osc1LevelParam);
    voiceManager.setOsc1Detune(*osc1DetuneParam);
    voiceManager.setOsc1Octave(static_cast<int>(*osc1OctaveParam));
    
    voiceManager.setOsc2Waveform(static_cast<Waveform>(static_cast<int>(*osc2WaveformParam)));
    voiceManager.setOsc2Level(*osc2LevelParam);
    voiceManager.setOsc2Detune(*osc2DetuneParam);
    voiceManager.setOsc2Octave(static_cast<int>(*osc2OctaveParam));
    
    voiceManager.setSubLevel(*subLevelParam);
    voiceManager.setSubOctave(static_cast<int>(*subOctaveParam));
    
    voiceManager.setFilterCutoff(*filterCutoffParam);
    voiceManager.setFilterResonance(*filterResonanceParam);
    voiceManager.setFilterType(static_cast<FilterType>(static_cast<int>(*filterTypeParam)));
    voiceManager.setFilterDrive(*filterDriveParam);
    voiceManager.setFilterEnvAmount(*filterEnvAmountParam);
    voiceManager.setFilterMode(static_cast<FilterMode>(static_cast<int>(*filterModeParam)));
    voiceManager.setFormantVowel(static_cast<int>(*formantVowelParam));
    
    voiceManager.setAmpEnvelope(*ampAttackParam, *ampDecayParam, *ampSustainParam, *ampReleaseParam);
    voiceManager.setFilterEnvelope(*filterAttackParam, *filterDecayParam, *filterSustainParam, *filterReleaseParam);
    
    lfo1.setRate(*lfo1RateParam);
    lfo1.setWaveform(static_cast<Waveform>(static_cast<int>(*lfo1WaveformParam)));
    lfo1.setSyncEnabled(*lfo1SyncParam > 0.5f);
    
    lfo2.setRate(*lfo2RateParam);
    lfo2.setWaveform(static_cast<Waveform>(static_cast<int>(*lfo2WaveformParam)));
    lfo2.setSyncEnabled(*lfo2SyncParam > 0.5f);
    
    // Spellbook
    spellbook.setShape(static_cast<Spellbook::Shape>(static_cast<int>(*spellbookShapeParam)));
    spellbook.setBaseRate(*spellbookRateParam);
    spellbook.setClockSync(*spellbookSyncParam > 0.5f);
    spellbook.setNumOutputs(static_cast<int>(*spellbookNumOutputsParam));
    
    float gainDb = *masterGainParam;
    float gainLinear = juce::Decibels::decibelsToGain(gainDb);
    masterGain.setTargetValue(gainLinear);
    
    // Update effects
    distortion.setEnabled(*distortionEnableParam > 0.5f);
    distortion.setDrive(*distortionDriveParam);
    distortion.setMix(*distortionMixParam);
    
    chorus.setEnabled(*chorusEnableParam > 0.5f);
    chorus.setRate(*chorusRateParam);
    chorus.setDepth(*chorusDepthParam);
    chorus.setMix(*chorusMixParam);
    
    delay.setEnabled(*delayEnableParam > 0.5f);
    delay.setDelayTime(*delayTimeParam);
    delay.setFeedback(*delayFeedbackParam);
    delay.setMix(*delayMixParam);
    
    reverb.setEnabled(*reverbEnableParam > 0.5f);
    reverb.setRoomSize(*reverbSizeParam);
    reverb.setDamping(*reverbDampParam);
    reverb.setMix(*reverbMixParam);
    
    ott.setEnabled(*ottEnableParam > 0.5f);
    ott.setDepth(*ottDepthParam);
    ott.setTime(*ottTimeParam);
    ott.setMix(*ottMixParam);
    
    // Update modulation matrix connections from APVTS
    for (int i = 0; i < ParamID::NUM_MOD_SLOTS; ++i)
    {
        auto idx = static_cast<size_t>(i);
        auto src = static_cast<ModSource>(static_cast<int>(*modSrcParams[idx]));
        auto dst = static_cast<ModDestination>(static_cast<int>(*modDstParams[idx]));
        float amt = *modAmtParams[idx];
        modMatrix.setConnection(i, src, dst, amt);
    }
}

} // namespace kndl

