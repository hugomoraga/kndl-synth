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
    
    // Prepare effects
    distortion.prepare(newSampleRate, newSamplesPerBlock);
    chorus.prepare(newSampleRate, newSamplesPerBlock);
    delay.prepare(newSampleRate, newSamplesPerBlock);
    reverb.prepare(newSampleRate, newSamplesPerBlock);
    
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
    float lfo1Value = lfo1.process();
    float lfo2Value = lfo2.process();
    
    modMatrix.setSourceValue(ModSource::LFO1, lfo1Value);
    modMatrix.setSourceValue(ModSource::LFO2, lfo2Value);
    modMatrix.setSourceValue(ModSource::ModWheel, modWheelValue);
    
    float pitchMod = modMatrix.getModulationAmount(ModDestination::Osc1Pitch);
    voiceManager.applyPitchMod(pitchMod);
    
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
    debugInfo.ampEnvValue = voiceDebug.ampEnvValue;
    debugInfo.filterEnvValue = voiceDebug.filterEnvValue;
    debugInfo.voiceOutput = voiceDebug.output;
    
    // LFO values
    debugInfo.lfo1Value = lfo1Value;
    debugInfo.lfo2Value = lfo2Value;
    
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
    
    // Apply effects chain: Distortion -> Chorus -> Delay -> Reverb
    output = distortion.process(output);
    output = chorus.process(output);
    output = delay.process(output);
    output = reverb.process(output);
    
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
    
    voiceManager.setAmpEnvelope(*ampAttackParam, *ampDecayParam, *ampSustainParam, *ampReleaseParam);
    voiceManager.setFilterEnvelope(*filterAttackParam, *filterDecayParam, *filterSustainParam, *filterReleaseParam);
    
    lfo1.setRate(*lfo1RateParam);
    lfo1.setWaveform(static_cast<Waveform>(static_cast<int>(*lfo1WaveformParam)));
    lfo1.setSyncEnabled(*lfo1SyncParam > 0.5f);
    
    lfo2.setRate(*lfo2RateParam);
    lfo2.setWaveform(static_cast<Waveform>(static_cast<int>(*lfo2WaveformParam)));
    lfo2.setSyncEnabled(*lfo2SyncParam > 0.5f);
    
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
}

} // namespace kndl

