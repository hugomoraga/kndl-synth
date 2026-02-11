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
    orbit.prepare(newSampleRate);
    noiseModSource.prepare(newSampleRate);
    
    // Prepare effects
    wavefolder.prepare(newSampleRate, newSamplesPerBlock);
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
    
    // Prepare safety limiter
    safetyLimiter.prepare(newSampleRate);
    safetyLimiter.setThreshold(-1.0f);   // Start limiting at -1 dBFS
    safetyLimiter.setCeiling(-0.1f);     // Absolute ceiling at -0.1 dBFS
    
    masterGain.reset(newSampleRate, 0.02);
    
    // Stereo width delay buffer (max ~5ms for Haas effect)
    size_t maxDelay = static_cast<size_t>(newSampleRate * 0.005);
    widthDelayBuffer.resize(maxDelay, 0.0f);
    widthDelayWriteIdx = 0;
    widthDelaySamples = maxDelay / 2; // default ~2.5ms
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
    
    // Get stereo parameters
    float width = (stereoWidthParam != nullptr) ? stereoWidthParam->load() : 0.5f;
    
    // Calculate delay samples for width (0 = mono, 1 = max ~5ms)
    if (!widthDelayBuffer.empty())
        widthDelaySamples = static_cast<size_t>(width * static_cast<float>(widthDelayBuffer.size() - 1));
    
    int sampleIndex = 0;
    
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();
        const int messagePosition = metadata.samplePosition;
        
        while (sampleIndex < messagePosition && sampleIndex < numSamples)
        {
            float mono = processSample();
            
            // Apply stereo: pan + width decorrelation
            float pan = juce::jlimit(-1.0f, 1.0f, debugInfo.panPosition);
            
            // Constant-power pan: center = 0, left = -1, right = +1
            float panAngle = (pan + 1.0f) * 0.5f; // 0 to 1
            float gainL = std::cos(panAngle * juce::MathConstants<float>::halfPi);
            float gainR = std::sin(panAngle * juce::MathConstants<float>::halfPi);
            
            leftChannel[sampleIndex] = mono * gainL;
            
            if (rightChannel)
            {
                // Apply Haas effect for stereo width: delay R channel slightly
                if (width > 0.01f && !widthDelayBuffer.empty() && widthDelaySamples > 0)
                {
                    widthDelayBuffer[widthDelayWriteIdx] = mono;
                    size_t readIdx = (widthDelayWriteIdx + widthDelayBuffer.size() - widthDelaySamples)
                                     % widthDelayBuffer.size();
                    float delayedR = widthDelayBuffer[readIdx];
                    widthDelayWriteIdx = (widthDelayWriteIdx + 1) % widthDelayBuffer.size();
                    
                    // Blend between mono and decorrelated
                    rightChannel[sampleIndex] = delayedR * gainR;
                }
                else
                {
                    rightChannel[sampleIndex] = mono * gainR;
                }
            }
            
            sampleIndex++;
        }
        
        handleMidiMessage(message);
    }
    
    while (sampleIndex < numSamples)
    {
        float mono = processSample();
        
        float pan = juce::jlimit(-1.0f, 1.0f, debugInfo.panPosition);
        float panAngle = (pan + 1.0f) * 0.5f;
        float gainL = std::cos(panAngle * juce::MathConstants<float>::halfPi);
        float gainR = std::sin(panAngle * juce::MathConstants<float>::halfPi);
        
        leftChannel[sampleIndex] = mono * gainL;
        
        if (rightChannel)
        {
            if (width > 0.01f && !widthDelayBuffer.empty() && widthDelaySamples > 0)
            {
                widthDelayBuffer[widthDelayWriteIdx] = mono;
                size_t readIdx = (widthDelayWriteIdx + widthDelayBuffer.size() - widthDelaySamples)
                                 % widthDelayBuffer.size();
                float delayedR = widthDelayBuffer[readIdx];
                widthDelayWriteIdx = (widthDelayWriteIdx + 1) % widthDelayBuffer.size();
                rightChannel[sampleIndex] = delayedR * gainR;
            }
            else
            {
                rightChannel[sampleIndex] = mono * gainR;
            }
        }
        
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
    
    // Feed Orbit outputs
    modMatrix.setSourceValue(ModSource::OrbitA, orbit.getOutput(0));
    modMatrix.setSourceValue(ModSource::OrbitB, orbit.getOutput(1));
    modMatrix.setSourceValue(ModSource::OrbitC, orbit.getOutput(2));
    modMatrix.setSourceValue(ModSource::OrbitD, orbit.getOutput(3));
    
    // Feed Noise mod source (S&H style random modulation)
    float noiseModVal = noiseModSource.process();
    modMatrix.setSourceValue(ModSource::Noise, noiseModVal);
    debugInfo.noiseModValue = noiseModVal;
    
    // Feed LFO values from previous sample
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
    
    // 6. Process Orbit modulator
    orbit.process();
    float sbA = orbit.getOutput(0);
    float sbB = orbit.getOutput(1);
    float sbC = orbit.getOutput(2);
    float sbD = orbit.getOutput(3);
    
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
    
    // New mod destinations
    voiceManager.setNoiseLevelMod(modMatrix.getModulationAmount(ModDestination::NoiseLevel));
    voiceManager.setRingModMixMod(modMatrix.getModulationAmount(ModDestination::RingModMix));
    
    // Pan modulation (stored in debugInfo for processBlock to use)
    debugInfo.panPosition = modMatrix.getModulationAmount(ModDestination::Pan);
    
    float output = voiceManager.process();
    
    // Normalize polyphonic sum to prevent volume explosion with many voices
    int activeVoices = voiceManager.getActiveVoiceCount();
    if (activeVoices > 1)
    {
        float normFactor = 1.0f / std::sqrt(static_cast<float>(activeVoices));
        output *= normFactor;
    }
    
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
    
    // Orbit values
    debugInfo.orbitA = sbA;
    debugInfo.orbitB = sbB;
    debugInfo.orbitC = sbC;
    debugInfo.orbitD = sbD;
    
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
    
    // Apply effects chain: Wavefolder -> Distortion -> Chorus -> Delay -> Reverb -> OTT
    output = wavefolder.process(output);
    output = distortion.process(output);
    output = chorus.process(output);
    output = delay.process(output);
    output = reverb.process(output);
    output = ott.process(output);
    
    // NaN/Inf check after effects chain
    if (!std::isfinite(output))
    {
        Logger::getInstance().logAudioAnomaly("NaN/Inf after effects chain", output);
        output = 0.0f;
    }
    
    // Remove DC offset before gain stage
    output = dcBlockerL.process(output);
    
    float gain = masterGain.getNextValue();
    output *= gain;
    
    // Safety limiter: catches peaks, protects speakers
    output = safetyLimiter.process(output);
    
    // Log audio stats periodically
    Logger::getInstance().logAudioStats(
        std::abs(output), 
        debugInfo.masterOutput,
        voiceManager.getActiveVoiceCount(),
        safetyLimiter.isLimiting()
    );
    
    debugInfo.masterOutput = output;
    debugInfo.gainReductionDb = safetyLimiter.getGainReductionDb();
    debugInfo.isLimiting = safetyLimiter.isLimiting();
    
    return output;
}

void KndlSynth::cacheParameterPointers()
{
    osc1EnableParam = parameters.getRawParameterValue(ParamID::OSC1_ENABLE);
    osc1WaveformParam = parameters.getRawParameterValue(ParamID::OSC1_WAVEFORM);
    osc1LevelParam = parameters.getRawParameterValue(ParamID::OSC1_LEVEL);
    osc1DetuneParam = parameters.getRawParameterValue(ParamID::OSC1_DETUNE);
    osc1OctaveParam = parameters.getRawParameterValue(ParamID::OSC1_OCTAVE);
    
    osc2EnableParam = parameters.getRawParameterValue(ParamID::OSC2_ENABLE);
    osc2WaveformParam = parameters.getRawParameterValue(ParamID::OSC2_WAVEFORM);
    osc2LevelParam = parameters.getRawParameterValue(ParamID::OSC2_LEVEL);
    osc2DetuneParam = parameters.getRawParameterValue(ParamID::OSC2_DETUNE);
    osc2OctaveParam = parameters.getRawParameterValue(ParamID::OSC2_OCTAVE);
    
    subEnableParam = parameters.getRawParameterValue(ParamID::SUB_ENABLE);
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
    
    // Orbit
    orbitShapeParam = parameters.getRawParameterValue(ParamID::ORBIT_SHAPE);
    orbitRateParam = parameters.getRawParameterValue(ParamID::ORBIT_RATE);
    orbitSyncParam = parameters.getRawParameterValue(ParamID::ORBIT_SYNC);
    orbitNumOutputsParam = parameters.getRawParameterValue(ParamID::ORBIT_NUM_OUTPUTS);
    
    // Noise
    noiseTypeParam = parameters.getRawParameterValue(ParamID::NOISE_TYPE);
    noiseLevelParam = parameters.getRawParameterValue(ParamID::NOISE_LEVEL);
    
    // Ring Mod
    ringModMixParam = parameters.getRawParameterValue(ParamID::RING_MOD_MIX);
    
    // Unison
    unisonVoicesParam = parameters.getRawParameterValue(ParamID::UNISON_VOICES);
    unisonDetuneParam = parameters.getRawParameterValue(ParamID::UNISON_DETUNE);
    
    // Stereo
    stereoWidthParam = parameters.getRawParameterValue(ParamID::STEREO_WIDTH);
    
    // Effects
    wfoldEnableParam = parameters.getRawParameterValue(ParamID::WFOLD_ENABLE);
    wfoldAmountParam = parameters.getRawParameterValue(ParamID::WFOLD_AMOUNT);
    wfoldMixParam = parameters.getRawParameterValue(ParamID::WFOLD_MIX);
    
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
    voiceManager.setOsc1Enable(*osc1EnableParam > 0.5f);
    voiceManager.setOsc1Waveform(static_cast<Waveform>(static_cast<int>(*osc1WaveformParam)));
    voiceManager.setOsc1Level(*osc1LevelParam);
    voiceManager.setOsc1Detune(*osc1DetuneParam);
    voiceManager.setOsc1Octave(static_cast<int>(*osc1OctaveParam));
    
    voiceManager.setOsc2Enable(*osc2EnableParam > 0.5f);
    voiceManager.setOsc2Waveform(static_cast<Waveform>(static_cast<int>(*osc2WaveformParam)));
    voiceManager.setOsc2Level(*osc2LevelParam);
    voiceManager.setOsc2Detune(*osc2DetuneParam);
    voiceManager.setOsc2Octave(static_cast<int>(*osc2OctaveParam));
    
    voiceManager.setSubEnable(*subEnableParam > 0.5f);
    voiceManager.setSubLevel(*subLevelParam);
    voiceManager.setSubOctave(static_cast<int>(*subOctaveParam));
    
    // Noise
    voiceManager.setNoiseType(static_cast<NoiseType>(static_cast<int>(*noiseTypeParam)));
    voiceManager.setNoiseLevel(*noiseLevelParam);
    
    // Ring Mod
    voiceManager.setRingModMix(*ringModMixParam);
    
    // Unison
    voiceManager.setUnisonVoices(static_cast<int>(*unisonVoicesParam));
    voiceManager.setUnisonDetune(*unisonDetuneParam);
    
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
    
    // Orbit
    orbit.setShape(static_cast<Orbit::Shape>(static_cast<int>(*orbitShapeParam)));
    orbit.setBaseRate(*orbitRateParam);
    orbit.setClockSync(*orbitSyncParam > 0.5f);
    orbit.setNumOutputs(static_cast<int>(*orbitNumOutputsParam));
    
    float gainDb = *masterGainParam;
    float gainLinear = juce::Decibels::decibelsToGain(gainDb);
    masterGain.setTargetValue(gainLinear);
    
    // Update effects
    wavefolder.setEnabled(*wfoldEnableParam > 0.5f);
    wavefolder.setAmount(*wfoldAmountParam);
    wavefolder.setMix(*wfoldMixParam);
    
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
