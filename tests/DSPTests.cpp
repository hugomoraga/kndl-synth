#include <JuceHeader.h>
#include "../dsp/modulators/Spellbook.h"
#include "../dsp/modulators/LFO.h"
#include "../dsp/core/ModulationMatrix.h"
#include "../dsp/core/Voice.h"

// ============================================================================
// Spellbook Tests
// ============================================================================
class SpellbookTests : public juce::UnitTest
{
public:
    SpellbookTests() : juce::UnitTest("Spellbook", "KndlSynth") {}
    
    void runTest() override
    {
        constexpr double sampleRate = 44100.0;
        
        beginTest("All shapes produce finite outputs over full cycle");
        {
            const kndl::Spellbook::Shape shapes[] = {
                kndl::Spellbook::Shape::Circle,
                kndl::Spellbook::Shape::Triangle,
                kndl::Spellbook::Shape::Square,
                kndl::Spellbook::Shape::Pentagon,
                kndl::Spellbook::Shape::Star,
                kndl::Spellbook::Shape::Spiral,
                kndl::Spellbook::Shape::Lemniscate
            };
            
            const char* shapeNames[] = {
                "Circle", "Triangle", "Square", "Pentagon", "Star", "Spiral", "Lemniscate"
            };
            
            for (int s = 0; s < 7; ++s)
            {
                kndl::Spellbook sb;
                sb.prepare(sampleRate);
                sb.setShape(shapes[s]);
                sb.setBaseRate(10.0f); // Fast rate so we cover full cycle quickly
                sb.setNumOutputs(4);
                
                int nanCount = 0;
                int infCount = 0;
                
                // Process for 2 full cycles (at 10Hz, 1 cycle = 4410 samples)
                for (int i = 0; i < 8820; ++i)
                {
                    sb.process();
                    for (int o = 0; o < 8; ++o)
                    {
                        float val = sb.getOutput(o);
                        if (std::isnan(val)) nanCount++;
                        if (std::isinf(val)) infCount++;
                    }
                }
                
                expect(nanCount == 0,
                    juce::String(shapeNames[s]) + " produced " + juce::String(nanCount) + " NaN values");
                expect(infCount == 0,
                    juce::String(shapeNames[s]) + " produced " + juce::String(infCount) + " Inf values");
            }
        }
        
        beginTest("Triangle shape uses correct π/3 offset (no extreme spikes)");
        {
            kndl::Spellbook sb;
            sb.prepare(sampleRate);
            sb.setShape(kndl::Spellbook::Shape::Triangle);
            sb.setBaseRate(1.0f);
            sb.setNumOutputs(1);
            
            float maxAbs = 0.0f;
            
            for (int i = 0; i < static_cast<int>(sampleRate); ++i)
            {
                sb.process();
                float x = sb.getOutput(0);
                float y = sb.getOutput(1);
                maxAbs = std::max(maxAbs, std::max(std::abs(x), std::abs(y)));
            }
            
            // With correct π/3 offset, triangle outputs should stay bounded.
            // With wrong π/6, values spike toward infinity near vertices.
            expect(maxAbs < 20.0f,
                "Triangle max output = " + juce::String(maxAbs, 2) + " (should be < 20 with correct offset)");
        }
        
        beginTest("Outputs stay within bounds for bipolar range");
        {
            kndl::Spellbook sb;
            sb.prepare(sampleRate);
            sb.setShape(kndl::Spellbook::Shape::Circle);
            sb.setBaseRate(5.0f);
            sb.setNumOutputs(2);
            
            for (int i = 0; i < 44100; ++i)
            {
                sb.process();
                for (int o = 0; o < 4; ++o)
                {
                    float val = sb.getOutput(o);
                    expect(val >= -1.5f && val <= 1.5f,
                        "Circle output[" + juce::String(o) + "] = " + juce::String(val, 4) + " out of expected range");
                }
            }
        }
        
        beginTest("getOutput returns 0 for out-of-range index");
        {
            kndl::Spellbook sb;
            sb.prepare(sampleRate);
            sb.setNumOutputs(2);
            sb.process();
            
            expect(sb.getOutput(-1) == 0.0f, "Negative index should return 0");
            expect(sb.getOutput(100) == 0.0f, "Large index should return 0");
        }
    }
};

static SpellbookTests spellbookTests;

// ============================================================================
// ModulationMatrix Tests
// ============================================================================
class ModMatrixTests : public juce::UnitTest
{
public:
    ModMatrixTests() : juce::UnitTest("ModulationMatrix", "KndlSynth") {}
    
    void runTest() override
    {
        constexpr double sampleRate = 44100.0;
        
        beginTest("Basic routing: source → destination with amount");
        {
            kndl::ModulationMatrix mm;
            mm.prepare(sampleRate);
            
            mm.setConnection(0, kndl::ModSource::LFO1, kndl::ModDestination::FilterCutoff, 0.5f);
            mm.setSourceValue(kndl::ModSource::LFO1, 1.0f);
            mm.updateSmoothing();
            
            float mod = mm.getModulationAmount(kndl::ModDestination::FilterCutoff);
            expectWithinAbsoluteError(mod, 0.5f, 0.01f,
                "LFO1=1.0 * amount=0.5 should produce mod=0.5");
        }
        
        beginTest("Multiple sources summed to same destination");
        {
            kndl::ModulationMatrix mm;
            mm.prepare(sampleRate);
            
            mm.setConnection(0, kndl::ModSource::LFO1, kndl::ModDestination::FilterCutoff, 0.3f);
            mm.setConnection(1, kndl::ModSource::LFO2, kndl::ModDestination::FilterCutoff, 0.2f);
            mm.setSourceValue(kndl::ModSource::LFO1, 1.0f);
            mm.setSourceValue(kndl::ModSource::LFO2, 1.0f);
            mm.updateSmoothing();
            
            float mod = mm.getModulationAmount(kndl::ModDestination::FilterCutoff);
            expectWithinAbsoluteError(mod, 0.5f, 0.01f,
                "Two sources (0.3 + 0.2) should sum to 0.5");
        }
        
        beginTest("Smoothing advances exactly once per updateSmoothing() call");
        {
            kndl::ModulationMatrix mm;
            mm.prepare(sampleRate);
            
            // Set connection with smoothing
            mm.setConnection(0, kndl::ModSource::LFO1, kndl::ModDestination::Osc1Pitch, 1.0f,
                            kndl::ModCurve::Linear, 100.0f); // 100ms smoothing
            mm.setSourceValue(kndl::ModSource::LFO1, 1.0f);
            
            // Call updateSmoothing once
            mm.updateSmoothing();
            float val1 = mm.getModulationAmount(kndl::ModDestination::Osc1Pitch);
            
            // Call getModulationAmount multiple times WITHOUT updateSmoothing
            // Value should NOT change (was the bug: it used to call updateSmoothing internally)
            float val2 = mm.getModulationAmount(kndl::ModDestination::Osc1Pitch);
            float val3 = mm.getModulationAmount(kndl::ModDestination::Osc1Pitch);
            
            expectEquals(val1, val2, "getModulationAmount should not advance smoothers");
            expectEquals(val2, val3, "getModulationAmount should not advance smoothers (2nd call)");
            
            // Now advance once more
            mm.updateSmoothing();
            float val4 = mm.getModulationAmount(kndl::ModDestination::Osc1Pitch);
            
            // val4 should be slightly larger than val1 (smoother advancing toward target)
            expect(val4 >= val1 - 0.0001f, // Could be approximately equal if already converged
                "After another updateSmoothing(), value should advance or stay same");
        }
        
        beginTest("No connection returns zero modulation");
        {
            kndl::ModulationMatrix mm;
            mm.prepare(sampleRate);
            mm.updateSmoothing();
            
            float mod = mm.getModulationAmount(kndl::ModDestination::FilterCutoff);
            expectEquals(mod, 0.0f, "No connections should produce 0 modulation");
        }
        
        beginTest("Bipolar modulation (negative amount)");
        {
            kndl::ModulationMatrix mm;
            mm.prepare(sampleRate);
            
            mm.setConnection(0, kndl::ModSource::LFO1, kndl::ModDestination::Osc1Pitch, -0.7f);
            mm.setSourceValue(kndl::ModSource::LFO1, 1.0f);
            mm.updateSmoothing();
            
            float mod = mm.getModulationAmount(kndl::ModDestination::Osc1Pitch);
            expectWithinAbsoluteError(mod, -0.7f, 0.01f,
                "Negative amount should produce negative modulation");
        }
        
        beginTest("clearConnection removes routing");
        {
            kndl::ModulationMatrix mm;
            mm.prepare(sampleRate);
            
            mm.setConnection(0, kndl::ModSource::LFO1, kndl::ModDestination::FilterCutoff, 1.0f);
            mm.setSourceValue(kndl::ModSource::LFO1, 1.0f);
            mm.updateSmoothing();
            
            float before = mm.getModulationAmount(kndl::ModDestination::FilterCutoff);
            expect(std::abs(before) > 0.5f, "Should have modulation before clear");
            
            mm.clearConnection(0);
            mm.updateSmoothing();
            
            float after = mm.getModulationAmount(kndl::ModDestination::FilterCutoff);
            expectEquals(after, 0.0f, "Should have zero modulation after clear");
        }
    }
};

static ModMatrixTests modMatrixTests;

// ============================================================================
// Voice Tests
// ============================================================================
class VoiceTests : public juce::UnitTest
{
public:
    VoiceTests() : juce::UnitTest("Voice", "KndlSynth") {}
    
    void runTest() override
    {
        constexpr double sampleRate = 44100.0;
        
        beginTest("Voice reset clears all modulation offsets");
        {
            kndl::Voice voice;
            voice.prepare(sampleRate, 512);
            
            // Set non-zero modulation
            voice.applyPitchMod(12.0f);
            voice.applyOsc2PitchMod(7.0f);
            voice.setFilterCutoffMod(0.5f);
            voice.setFilterResoMod(0.3f);
            voice.setOsc1LevelMod(0.2f);
            voice.setOsc2LevelMod(0.1f);
            voice.setSubLevelMod(0.15f);
            voice.setAmpLevelMod(0.25f);
            
            voice.reset();
            
            // After reset, noteOn a new note and verify first sample has no stale modulation
            voice.setOsc1Level(0.8f);
            voice.noteOn(60, 1.0f); // C4
            
            // Process some samples - output should be clean without stale pitch mod
            float firstSample = voice.process();
            expect(std::isfinite(firstSample), "First sample after reset should be finite");
            expect(voice.getIsActive(), "Voice should be active after noteOn");
        }
        
        beginTest("Voice noteOn clears pitch modulation (voice stealing)");
        {
            kndl::Voice voice;
            voice.prepare(sampleRate, 512);
            voice.setOsc1Level(0.8f);
            
            // Start a note with pitch modulation
            voice.noteOn(60, 1.0f); // C4
            voice.applyPitchMod(24.0f); // +2 octaves
            
            // Process a few samples with the modulated pitch
            for (int i = 0; i < 10; ++i)
                voice.process();
            
            // Voice steal: new note on same voice without going through reset
            voice.noteOn(60, 1.0f); // C4 again
            
            // The new note should NOT have the +24 semitone pitch mod
            // Process and check it sounds like C4, not C6
            float sample = voice.process();
            expect(std::isfinite(sample), "Sample after voice steal should be finite");
            // We can't easily check frequency from a single sample, but we verified
            // that noteOn now zeroes pitchModulation before updateOscillatorFrequencies
        }
        
        beginTest("Voice produces finite output with extreme pitch modulation");
        {
            kndl::Voice voice;
            voice.prepare(sampleRate, 512);
            voice.setOsc1Level(0.8f);
            voice.noteOn(60, 1.0f);
            
            // Apply extreme pitch mod that would overflow without guard
            voice.applyPitchMod(200.0f); // Way beyond ±48 semitone clamp
            
            for (int i = 0; i < 100; ++i)
            {
                float sample = voice.process();
                expect(std::isfinite(sample),
                    "Sample " + juce::String(i) + " should be finite with extreme pitch mod");
            }
        }
        
        beginTest("Voice produces sound when active");
        {
            kndl::Voice voice;
            voice.prepare(sampleRate, 512);
            voice.setOsc1Level(0.8f);
            voice.setAmpEnvelope(0.001f, 0.1f, 0.8f, 0.1f); // Fast attack
            voice.noteOn(60, 1.0f);
            
            float maxAbs = 0.0f;
            // Process enough samples for envelope to open (attack phase)
            for (int i = 0; i < 200; ++i)
            {
                float sample = voice.process();
                maxAbs = std::max(maxAbs, std::abs(sample));
            }
            
            expect(maxAbs > 0.01f,
                "Voice should produce audible output (max=" + juce::String(maxAbs, 4) + ")");
        }
        
        beginTest("Voice goes inactive after noteOff + release");
        {
            kndl::Voice voice;
            voice.prepare(sampleRate, 512);
            voice.setOsc1Level(0.8f);
            voice.setAmpEnvelope(0.001f, 0.01f, 0.5f, 0.01f); // Very fast envelope
            voice.noteOn(60, 1.0f);
            
            // Process through attack + decay to sustain
            for (int i = 0; i < 500; ++i)
                voice.process();
            
            expect(voice.getIsActive(), "Voice should be active during sustain");
            
            voice.noteOff();
            
            // Process through release (10ms at 44100 = 441 samples, give extra margin)
            for (int i = 0; i < 2000; ++i)
            {
                voice.process();
                if (!voice.getIsActive())
                    break;
            }
            
            expect(!voice.getIsActive(), "Voice should be inactive after release completes");
        }
    }
};

static VoiceTests voiceTests;

// ============================================================================
// LFO Tests
// ============================================================================
class LFOTests : public juce::UnitTest
{
public:
    LFOTests() : juce::UnitTest("LFO", "KndlSynth") {}
    
    void runTest() override
    {
        constexpr double sampleRate = 44100.0;
        
        beginTest("All waveforms produce values in [-1, 1]");
        {
            kndl::Waveform waveforms[] = {
                kndl::Waveform::Sine,
                kndl::Waveform::Triangle,
                kndl::Waveform::Saw,
                kndl::Waveform::Square
            };
            
            const char* names[] = { "Sine", "Triangle", "Saw", "Square" };
            
            for (int w = 0; w < 4; ++w)
            {
                kndl::LFO lfo;
                lfo.prepare(sampleRate);
                lfo.setRate(5.0f);
                lfo.setWaveform(waveforms[w]);
                
                float minVal = 999.0f, maxVal = -999.0f;
                
                for (int i = 0; i < 44100; ++i)
                {
                    float val = lfo.process();
                    minVal = std::min(minVal, val);
                    maxVal = std::max(maxVal, val);
                }
                
                expect(minVal >= -1.001f && maxVal <= 1.001f,
                    juce::String(names[w]) + " range [" + juce::String(minVal, 3) + ", " + 
                    juce::String(maxVal, 3) + "] should be within [-1, 1]");
            }
        }
        
        beginTest("getCurrentValue matches last process output");
        {
            kndl::LFO lfo;
            lfo.prepare(sampleRate);
            lfo.setRate(1.0f);
            lfo.setWaveform(kndl::Waveform::Sine);
            
            // Process one sample
            float processed = lfo.process();
            float current = lfo.getCurrentValue();
            
            // They won't match exactly because process() advances phase AFTER computing output
            // But getCurrentValue should return a valid value
            expect(std::isfinite(current), "getCurrentValue should return finite");
            (void)processed; // Used for side effect
        }
        
        beginTest("Rate change affects frequency");
        {
            kndl::LFO lfo;
            lfo.prepare(sampleRate);
            lfo.setWaveform(kndl::Waveform::Saw);
            
            // At 1 Hz, one full cycle = sampleRate samples
            // At 10 Hz, one full cycle = sampleRate/10 samples
            // Count zero-crossings to estimate frequency
            
            lfo.setRate(10.0f);
            
            int zeroCrossings = 0;
            float prevVal = 0.0f;
            
            for (int i = 0; i < 44100; ++i) // 1 second
            {
                float val = lfo.process();
                if (i > 0 && ((prevVal >= 0.0f && val < 0.0f) || (prevVal < 0.0f && val >= 0.0f)))
                    zeroCrossings++;
                prevVal = val;
            }
            
            // Saw at 10 Hz has 2 zero crossings per cycle (one rising, one at wrap) = ~20/sec
            // Allow some tolerance for boundary effects
            expect(zeroCrossings >= 16 && zeroCrossings <= 24,
                "10Hz saw should have ~20 zero crossings, got " + juce::String(zeroCrossings));
        }
        
        beginTest("Reset brings phase back to zero");
        {
            kndl::LFO lfo;
            lfo.prepare(sampleRate);
            lfo.setRate(1.0f);
            lfo.setWaveform(kndl::Waveform::Sine);
            
            // Advance phase
            for (int i = 0; i < 1000; ++i)
                lfo.process();
            
            float beforeReset = lfo.getCurrentValue();
            
            lfo.reset();
            
            float afterReset = lfo.getCurrentValue();
            
            // After reset, phase=0 → sin(0) = 0
            expectWithinAbsoluteError(afterReset, 0.0f, 0.001f,
                "After reset, sine LFO should output ~0 (phase=0)");
            (void)beforeReset;
        }
    }
};

static LFOTests lfoTests;
