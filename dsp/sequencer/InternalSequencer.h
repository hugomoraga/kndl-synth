#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

namespace kndl {

/**
 * InternalSequencer - Built-in step sequencer for testing without MIDI hardware
 * 
 * Generates MIDI noteOn/noteOff messages injected directly into the MidiBuffer.
 * Includes several preset patterns (scales, arpeggios, chords) and tempo control.
 * Sample-accurate timing for tight rhythmic output.
 */
class InternalSequencer
{
public:
    // Available sequence patterns
    enum class Pattern
    {
        MinorScale,       // C minor scale ascending/descending
        MajorArpeggio,    // C major triad arpeggio
        MinorArpeggio,    // C minor triad arpeggio
        Fifths,           // Power chord fifths pattern
        Chromatic,        // Chromatic run
        Random,           // Random notes from scale
        ChordStabs,       // Chord stabs (multiple notes)
        Drone,            // Single sustained note
        NumPatterns
    };

    InternalSequencer() { buildPattern(currentPattern); }

    void setSampleRate(double sr)
    {
        sampleRate = sr;
        recalcTimings();
    }

    void setEnabled(bool e)
    {
        if (enabled == e) return;
        enabled = e;

        if (!enabled)
        {
            // Reset state when disabled
            pendingNoteOff = true;
        }
    }

    bool isEnabled() const { return enabled; }

    void setTempo(double bpm)
    {
        tempo = juce::jlimit(40.0, 300.0, bpm);
        recalcTimings();
    }

    double getTempo() const { return tempo; }

    void setGateLength(float gate)
    {
        gateLength = juce::jlimit(0.1f, 0.95f, gate);
        recalcTimings();
    }

    void setBaseOctave(int octave)
    {
        baseOctave = juce::jlimit(1, 7, octave);
        buildPattern(currentPattern);
    }

    int getBaseOctave() const { return baseOctave; }

    void setVelocity(int vel)
    {
        velocity = static_cast<juce::uint8>(juce::jlimit(1, 127, vel));
    }

    void setPattern(Pattern p)
    {
        currentPattern = p;
        buildPattern(p);
        currentStep = 0;
    }

    Pattern getPattern() const { return currentPattern; }
    int getCurrentStep() const { return currentStep; }
    int getNumSteps() const { return static_cast<int>(sequence.size()); }

    static juce::String getPatternName(Pattern p)
    {
        switch (p)
        {
            case Pattern::MinorScale:     return "Minor Scale";
            case Pattern::MajorArpeggio:  return "Maj Arp";
            case Pattern::MinorArpeggio:  return "Min Arp";
            case Pattern::Fifths:         return "Fifths";
            case Pattern::Chromatic:       return "Chromatic";
            case Pattern::Random:         return "Random";
            case Pattern::ChordStabs:     return "Chords";
            case Pattern::Drone:          return "Drone";
            case Pattern::NumPatterns:
            default:                      return "???";
        }
    }

    /**
     * Process a block: inject MIDI note events into the buffer.
     * Call this BEFORE the synth processes the buffer.
     */
    void processBlock(juce::MidiBuffer& midiMessages, int numSamples)
    {
        if (!enabled && !pendingNoteOff) return;

        for (int i = 0; i < numSamples; ++i)
        {
            // Handle pending note-off when sequencer is disabled
            if (!enabled && pendingNoteOff)
            {
                sendNoteOff(midiMessages, i);
                pendingNoteOff = false;
                sampleCounter = 0;
                noteIsOn = false;
                return;
            }

            sampleCounter++;

            // Note-off at gate point
            if (noteIsOn && sampleCounter >= samplesPerGate)
            {
                sendNoteOff(midiMessages, i);
                noteIsOn = false;
            }

            // New step at beat boundary
            if (sampleCounter >= samplesPerStep)
            {
                sampleCounter = 0;

                // Advance step
                currentStep = (currentStep + 1) % static_cast<int>(sequence.size());

                // Send note-on for new step
                const auto& step = sequence[static_cast<size_t>(currentStep)];
                if (!step.notes.empty())
                {
                    for (int note : step.notes)
                    {
                        if (note >= 0 && note <= 127)
                        {
                            midiMessages.addEvent(
                                juce::MidiMessage::noteOn(midiChannel, note, velocity), i);
                        }
                    }
                    currentNotes = step.notes;
                    noteIsOn = true;
                }
            }
        }
    }

private:
    // A single step can trigger multiple notes (chords)
    struct Step
    {
        std::vector<int> notes;
    };

    void recalcTimings()
    {
        // Quarter note = one beat at current tempo
        double samplesPerBeat = sampleRate * 60.0 / tempo;
        // Each step is an eighth note (half a beat)
        samplesPerStep = static_cast<int>(samplesPerBeat * 0.5);
        samplesPerGate = static_cast<int>(samplesPerStep * gateLength);

        if (samplesPerStep < 1) samplesPerStep = 1;
        if (samplesPerGate < 1) samplesPerGate = 1;
    }

    void sendNoteOff(juce::MidiBuffer& midiMessages, int samplePosition)
    {
        for (int note : currentNotes)
        {
            if (note >= 0 && note <= 127)
            {
                midiMessages.addEvent(
                    juce::MidiMessage::noteOff(midiChannel, note, (juce::uint8)0),
                    samplePosition);
            }
        }
    }

    void buildPattern(Pattern p)
    {
        sequence.clear();
        int root = 12 * baseOctave; // C at base octave (MIDI note)

        switch (p)
        {
            case Pattern::MinorScale:
            {
                // Natural minor scale up and down
                const int intervals[] = { 0, 2, 3, 5, 7, 8, 10, 12, 10, 8, 7, 5, 3, 2 };
                for (int iv : intervals)
                    sequence.push_back({ { root + iv } });
                break;
            }

            case Pattern::MajorArpeggio:
            {
                const int intervals[] = { 0, 4, 7, 12, 7, 4, 0, -5 };
                for (int iv : intervals)
                    sequence.push_back({ { root + iv } });
                break;
            }

            case Pattern::MinorArpeggio:
            {
                const int intervals[] = { 0, 3, 7, 12, 7, 3, 0, -5 };
                for (int iv : intervals)
                    sequence.push_back({ { root + iv } });
                break;
            }

            case Pattern::Fifths:
            {
                // Power chord movement
                const int roots[] = { 0, 0, 5, 5, 7, 7, 3, 3 };
                for (int r : roots)
                    sequence.push_back({ { root + r, root + r + 7 } });
                break;
            }

            case Pattern::Chromatic:
            {
                for (int i = 0; i < 12; ++i)
                    sequence.push_back({ { root + i } });
                for (int i = 11; i >= 0; --i)
                    sequence.push_back({ { root + i } });
                break;
            }

            case Pattern::Random:
            {
                // Pre-generate a "random" pattern from minor pentatonic
                const int scale[] = { 0, 3, 5, 7, 10, 12, 15, 17 };
                juce::Random rng(42); // Fixed seed for reproducibility
                for (int i = 0; i < 16; ++i)
                {
                    int idx = rng.nextInt(8);
                    sequence.push_back({ { root + scale[idx] } });
                }
                break;
            }

            case Pattern::ChordStabs:
            {
                // i - VI - III - VII chord progression in minor
                sequence.push_back({ { root, root + 3, root + 7 } });           // Cm
                sequence.push_back({ { root, root + 3, root + 7 } });           // Cm
                sequence.push_back({ { root + 8, root + 12, root + 15 } });     // Ab
                sequence.push_back({ { root + 8, root + 12, root + 15 } });     // Ab
                sequence.push_back({ { root + 3, root + 7, root + 10 } });      // Eb
                sequence.push_back({ { root + 3, root + 7, root + 10 } });      // Eb
                sequence.push_back({ { root + 10, root + 14, root + 17 } });    // Bb
                sequence.push_back({ { root + 10, root + 14, root + 17 } });    // Bb
                break;
            }

            case Pattern::Drone:
            {
                // Single sustained note (long gate recommended)
                sequence.push_back({ { root } });
                sequence.push_back({ { root } });
                sequence.push_back({ { root + 7 } }); // Fifth
                sequence.push_back({ { root } });
                break;
            }

            case Pattern::NumPatterns:
            default:
                sequence.push_back({ { root } });
                break;
        }

        // Clamp all notes to valid MIDI range
        for (auto& step : sequence)
        {
            for (auto& note : step.notes)
                note = juce::jlimit(0, 127, note);
        }
    }

    // State
    bool enabled = false;
    double sampleRate = 44100.0;
    double tempo = 120.0;
    float gateLength = 0.75f;
    int baseOctave = 3;
    juce::uint8 velocity = 100;
    int midiChannel = 1;
    Pattern currentPattern = Pattern::MinorArpeggio;

    // Timing
    int samplesPerStep = 11025;  // Eighth note at 120 BPM, 44100 Hz
    int samplesPerGate = 8269;
    int sampleCounter = 0;

    // Playback
    int currentStep = 0;
    bool noteIsOn = false;
    bool pendingNoteOff = false;
    std::vector<int> currentNotes;
    std::vector<Step> sequence;
};

} // namespace kndl
