#pragma once

#include <JuceHeader.h>
#include "../skins/Theme.h"
#include <array>

namespace kndl::ui {

/**
 * KndlScope - Osciloscopio estilo terminal/sci-fi con control de zoom
 * Muestra la forma de onda con estética retro-futurista
 */
class KndlScope : public juce::Component
{
public:
    static constexpr int BufferSize = 256;
    
    KndlScope()
    {
        waveBuffer.fill(0.0f);
        
        // Setup zoom slider
        addAndMakeVisible(zoomSlider);
        zoomSlider.setRange(0.1, 10.0, 0.1);
        zoomSlider.setValue(1.0);
        zoomSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        zoomSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        zoomSlider.onValueChange = [this]() { repaint(); };
    }
    
    void setTheme(const Theme* newTheme)
    {
        theme = newTheme;
        
        if (theme)
        {
            zoomSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff1a1a25));
            zoomSlider.setColour(juce::Slider::trackColourId, theme->getAccentPrimary().withAlpha(0.5f));
            zoomSlider.setColour(juce::Slider::thumbColourId, theme->getAccentPrimary());
        }
        
        repaint();
    }
    
    void pushSample(float sample)
    {
        waveBuffer[static_cast<size_t>(writeIndex)] = sample;
        writeIndex = (writeIndex + 1) % BufferSize;
    }
    
    void pushBuffer(const float* samples, int numSamples)
    {
        // Downsample to fit buffer
        int step = juce::jmax(1, numSamples / BufferSize);
        for (int i = 0; i < numSamples && (writeIndex < BufferSize); i += step)
        {
            waveBuffer[static_cast<size_t>(writeIndex)] = samples[i];
            writeIndex = (writeIndex + 1) % BufferSize;
        }
    }
    
    void paint(juce::Graphics& g) override
    {
        if (!theme) return;
        
        auto bounds = getLocalBounds();
        auto scopeBounds = bounds.toFloat();
        
        // Reserve space for zoom control at bottom
        int controlHeight = 20;
        scopeBounds.removeFromBottom(static_cast<float>(controlHeight));
        
        // Background with grid
        g.setColour(juce::Colour(0xff0a0a0f));
        g.fillRoundedRectangle(scopeBounds, 4.0f);
        
        // Grid lines
        g.setColour(juce::Colour(0xff1a1a25));
        int gridSize = 16;
        for (float x = scopeBounds.getX(); x < scopeBounds.getRight(); x += gridSize)
            g.drawVerticalLine(static_cast<int>(x), scopeBounds.getY(), scopeBounds.getBottom());
        for (float y = scopeBounds.getY(); y < scopeBounds.getBottom(); y += gridSize)
            g.drawHorizontalLine(static_cast<int>(y), scopeBounds.getX(), scopeBounds.getRight());
        
        // Center line
        g.setColour(juce::Colour(0xff2a2a35));
        g.drawHorizontalLine(static_cast<int>(scopeBounds.getCentreY()), scopeBounds.getX(), scopeBounds.getRight());
        
        // Waveform with zoom
        juce::Path wavePath;
        float centerY = scopeBounds.getCentreY();
        float baseAmplitude = scopeBounds.getHeight() * 0.4f;
        float zoomFactor = static_cast<float>(zoomSlider.getValue());
        float amplitude = baseAmplitude * zoomFactor;
        
        bool started = false;
        for (int i = 0; i < BufferSize; ++i)
        {
            size_t idx = static_cast<size_t>((writeIndex + i) % BufferSize);
            float x = scopeBounds.getX() + (static_cast<float>(i) / BufferSize) * scopeBounds.getWidth();
            float sampleValue = waveBuffer[idx] * amplitude;
            // Clamp to scope bounds
            sampleValue = juce::jlimit(-scopeBounds.getHeight() * 0.48f, scopeBounds.getHeight() * 0.48f, sampleValue);
            float y = centerY - sampleValue;
            
            if (!started)
            {
                wavePath.startNewSubPath(x, y);
                started = true;
            }
            else
            {
                wavePath.lineTo(x, y);
            }
        }
        
        // Glow effect
        g.setColour(theme->getAccentTertiary().withAlpha(0.15f));
        g.strokePath(wavePath, juce::PathStrokeType(4.0f));
        
        g.setColour(theme->getAccentTertiary().withAlpha(0.4f));
        g.strokePath(wavePath, juce::PathStrokeType(2.0f));
        
        g.setColour(theme->getAccentTertiary());
        g.strokePath(wavePath, juce::PathStrokeType(1.0f));
        
        // Corner brackets (sci-fi frame)
        g.setColour(theme->getAccentPrimary().withAlpha(0.6f));
        int bracketSize = 10;
        // Top-left
        g.drawLine(scopeBounds.getX(), scopeBounds.getY(), scopeBounds.getX() + bracketSize, scopeBounds.getY(), 1.5f);
        g.drawLine(scopeBounds.getX(), scopeBounds.getY(), scopeBounds.getX(), scopeBounds.getY() + bracketSize, 1.5f);
        // Top-right
        g.drawLine(scopeBounds.getRight() - bracketSize, scopeBounds.getY(), scopeBounds.getRight(), scopeBounds.getY(), 1.5f);
        g.drawLine(scopeBounds.getRight(), scopeBounds.getY(), scopeBounds.getRight(), scopeBounds.getY() + bracketSize, 1.5f);
        // Bottom-left
        g.drawLine(scopeBounds.getX(), scopeBounds.getBottom(), scopeBounds.getX() + bracketSize, scopeBounds.getBottom(), 1.5f);
        g.drawLine(scopeBounds.getX(), scopeBounds.getBottom() - bracketSize, scopeBounds.getX(), scopeBounds.getBottom(), 1.5f);
        // Bottom-right
        g.drawLine(scopeBounds.getRight() - bracketSize, scopeBounds.getBottom(), scopeBounds.getRight(), scopeBounds.getBottom(), 1.5f);
        g.drawLine(scopeBounds.getRight(), scopeBounds.getBottom() - bracketSize, scopeBounds.getRight(), scopeBounds.getBottom(), 1.5f);
        
        // Zoom label
        auto controlArea = bounds.removeFromBottom(controlHeight);
        g.setColour(theme->getTextMuted());
        g.setFont(theme->getSmallFont());
        g.drawText("ZOOM", controlArea.removeFromLeft(35), juce::Justification::centredLeft);
        
        // Zoom value
        g.setColour(theme->getTextSecondary());
        g.drawText(juce::String(zoomFactor, 1) + "x", controlArea.removeFromRight(30), juce::Justification::centredRight);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        auto controlArea = bounds.removeFromBottom(20);
        controlArea.removeFromLeft(35);  // Space for "ZOOM" label
        controlArea.removeFromRight(30); // Space for value
        zoomSlider.setBounds(controlArea.reduced(2, 4));
    }
    
private:
    const Theme* theme = nullptr;
    std::array<float, BufferSize> waveBuffer;
    int writeIndex = 0;
    juce::Slider zoomSlider;
};

/**
 * KndlDataDisplay - Panel de datos estilo terminal sci-fi con 2 columnas
 */
class KndlDataDisplay : public juce::Component
{
public:
    void setTheme(const Theme* newTheme)
    {
        theme = newTheme;
        repaint();
    }
    
    void setNoteInfo(int midiNote, float velocity, int voiceCount)
    {
        currentNote = midiNote;
        currentVelocity = velocity;
        activeVoices = voiceCount;
    }
    
    void setOscValues(float osc1, float osc2, float sub)
    {
        osc1Value = osc1;
        osc2Value = osc2;
        subValue = sub;
    }
    
    void setFilterValues(float cutoff, float reso, float envAmt)
    {
        filterCutoff = cutoff;
        filterReso = reso;
        filterEnv = envAmt;
    }
    
    void setEnvValues(float ampEnv, float filterEnvVal)
    {
        ampEnvValue = ampEnv;
        filterEnvValue = filterEnvVal;
    }
    
    void setLfoValues(float lfo1, float lfo2)
    {
        lfo1Value = lfo1;
        lfo2Value = lfo2;
    }
    
    void setSpellbookValues(float a, float b, float c, float d)
    {
        sbA = a;
        sbB = b;
        sbC = c;
        sbD = d;
    }
    
    void setOutputLevel(float level)
    {
        outputLevel = level;
    }
    
    void paint(juce::Graphics& g) override
    {
        if (!theme) return;
        
        auto bounds = getLocalBounds();
        
        // Background
        g.setColour(juce::Colour(0xff0d0d12));
        g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
        
        // Scanline effect
        g.setColour(juce::Colour(0xff151520).withAlpha(0.3f));
        for (int y = 0; y < getHeight(); y += 2)
            g.drawHorizontalLine(y, 0.0f, static_cast<float>(getWidth()));
        
        // Fonts (usar Inconsolata del theme)
        juce::Font monoFont = theme->getValueFont();
        juce::Font monoSmall = theme->getSmallFont();
        juce::Font monoBold = theme->getLabelFont();
        monoBold.setStyleFlags(juce::Font::bold);
        
        int lineHeight = 13;
        int colWidth = bounds.getWidth() / 2 - 10;
        
        // ==================== COLUMN 1 (Left) ====================
        int x1 = 10;
        int y1 = 8;
        
        // === HEADER ===
        g.setColour(theme->getAccentPrimary());
        g.setFont(monoBold);
        g.drawText("SYN.STATUS", x1, y1, colWidth, lineHeight, juce::Justification::left);
        
        // Status indicator
        g.setColour(activeVoices > 0 ? theme->getPositive() : theme->getTextMuted());
        g.fillEllipse(static_cast<float>(x1 + colWidth - 12), static_cast<float>(y1 + 2), 8.0f, 8.0f);
        
        y1 += lineHeight + 4;
        
        // === NOTE INFO ===
        g.setColour(theme->getTextMuted());
        g.setFont(monoSmall);
        g.drawText("NOTE.DATA", x1, y1, colWidth, lineHeight, juce::Justification::left);
        y1 += lineHeight;
        
        g.setFont(monoFont);
        if (currentNote >= 0)
        {
            static const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
            int octave = (currentNote / 12) - 1;
            int noteIdx = currentNote % 12;
            
            g.setColour(theme->getAccentTertiary());
            juce::String noteStr = juce::String(noteNames[noteIdx]) + juce::String(octave);
            g.drawText("N:" + noteStr.paddedRight(' ', 3) + " M:" + juce::String(currentNote).paddedLeft('0', 3), 
                      x1, y1, colWidth, lineHeight, juce::Justification::left);
            y1 += lineHeight;
            
            g.setColour(theme->getTextSecondary());
            g.drawText("V:" + juce::String(static_cast<int>(currentVelocity * 127)).paddedLeft('0', 3) + 
                      " VC:" + juce::String(activeVoices).paddedLeft('0', 2),
                      x1, y1, colWidth, lineHeight, juce::Justification::left);
        }
        else
        {
            g.setColour(theme->getTextMuted());
            g.drawText("N:--- M:---", x1, y1, colWidth, lineHeight, juce::Justification::left);
            y1 += lineHeight;
            g.drawText("V:--- VC:00", x1, y1, colWidth, lineHeight, juce::Justification::left);
        }
        y1 += lineHeight + 6;
        
        // === OSCILLATORS ===
        g.setColour(theme->getTextMuted());
        g.setFont(monoSmall);
        g.drawText("OSC.MATRIX", x1, y1, colWidth, lineHeight, juce::Justification::left);
        y1 += lineHeight;
        
        g.setFont(monoFont);
        drawCompactBar(g, x1, y1, "O1", osc1Value, theme->getAccentPrimary(), colWidth - 10);
        y1 += lineHeight;
        drawCompactBar(g, x1, y1, "O2", osc2Value, theme->getAccentSecondary(), colWidth - 10);
        y1 += lineHeight;
        drawCompactBar(g, x1, y1, "SB", subValue, theme->getAccentTertiary(), colWidth - 10);
        y1 += lineHeight + 6;
        
        // === ENVELOPES ===
        g.setColour(theme->getTextMuted());
        g.setFont(monoSmall);
        g.drawText("ENV.STATE", x1, y1, colWidth, lineHeight, juce::Justification::left);
        y1 += lineHeight;
        
        g.setFont(monoFont);
        drawCompactBar(g, x1, y1, "AMP", ampEnvValue, theme->getPositive(), colWidth - 10);
        y1 += lineHeight;
        drawCompactBar(g, x1, y1, "FLT", filterEnvValue, theme->getWarning(), colWidth - 10);
        
        // ==================== COLUMN 2 (Right) ====================
        int x2 = bounds.getWidth() / 2 + 5;
        int y2 = 8;
        
        // === FILTER SECTION ===
        g.setColour(theme->getAccentSecondary());
        g.setFont(monoBold);
        g.drawText("FLT.PARAMS", x2, y2, colWidth, lineHeight, juce::Justification::left);
        y2 += lineHeight + 4;
        
        g.setFont(monoFont);
        g.setColour(theme->getTextSecondary());
        
        // Cutoff with bar
        g.setColour(theme->getTextMuted());
        g.drawText("CUT:", x2, y2, 30, lineHeight, juce::Justification::left);
        
        // Cutoff bar (log scale visualization)
        float cutoffNorm = juce::jmap(std::log10(juce::jmax(20.0f, filterCutoff)), 
                                       std::log10(20.0f), std::log10(20000.0f), 0.0f, 1.0f);
        drawMiniBar(g, x2 + 32, y2 + 3, 60, 7, cutoffNorm, theme->getAccentSecondary());
        
        g.setColour(theme->getTextPrimary());
        g.drawText(formatFreq(filterCutoff), x2 + 96, y2, 50, lineHeight, juce::Justification::left);
        y2 += lineHeight;
        
        // Resonance
        g.setColour(theme->getTextMuted());
        g.drawText("RES:", x2, y2, 30, lineHeight, juce::Justification::left);
        drawMiniBar(g, x2 + 32, y2 + 3, 60, 7, filterReso, theme->getWarning());
        
        g.setColour(theme->getTextPrimary());
        g.drawText(juce::String(static_cast<int>(filterReso * 100)) + "%", x2 + 96, y2, 50, lineHeight, juce::Justification::left);
        y2 += lineHeight;
        
        // Filter Env Amount
        g.setColour(theme->getTextMuted());
        g.drawText("ENV:", x2, y2, 30, lineHeight, juce::Justification::left);
        drawMiniBar(g, x2 + 32, y2 + 3, 60, 7, std::abs(filterEnv), theme->getAccentTertiary());
        
        g.setColour(theme->getTextPrimary());
        g.drawText(juce::String(static_cast<int>(filterEnv * 100)) + "%", x2 + 96, y2, 50, lineHeight, juce::Justification::left);
        y2 += lineHeight + 6;
        
        // === LFO ===
        g.setColour(theme->getTextMuted());
        g.setFont(monoSmall);
        g.drawText("LFO.CYCLE", x2, y2, colWidth, lineHeight, juce::Justification::left);
        y2 += lineHeight;
        
        g.setFont(monoFont);
        drawBipolarCompact(g, x2, y2, "L1", lfo1Value, theme->getAccentTertiary(), colWidth - 10);
        y2 += lineHeight;
        drawBipolarCompact(g, x2, y2, "L2", lfo2Value, theme->getAccentSecondary(), colWidth - 10);
        y2 += lineHeight + 6;
        
        // === SPELLBOOK ===
        g.setColour(theme->getTextMuted());
        g.setFont(monoSmall);
        g.drawText("SB.OUTPUT", x2, y2, colWidth, lineHeight, juce::Justification::left);
        y2 += lineHeight;
        
        g.setFont(monoFont);
        drawBipolarCompact(g, x2, y2, "SA", sbA, theme->getAccentPrimary(), colWidth - 10);
        y2 += lineHeight;
        drawBipolarCompact(g, x2, y2, "SB", sbB, theme->getAccentSecondary(), colWidth - 10);
        y2 += lineHeight;
        drawBipolarCompact(g, x2, y2, "SC", sbC, theme->getAccentTertiary(), colWidth - 10);
        y2 += lineHeight;
        drawBipolarCompact(g, x2, y2, "SD", sbD, theme->getWarning(), colWidth - 10);
        y2 += lineHeight + 6;
        
        // === OUTPUT METER ===
        g.setColour(theme->getTextMuted());
        g.setFont(monoSmall);
        g.drawText("OUT.LEVEL", x2, y2, colWidth, lineHeight, juce::Justification::left);
        y2 += lineHeight;
        
        drawOutputMeter(g, x2, y2, outputLevel, colWidth - 10);
        
        // === DIVIDER LINE ===
        g.setColour(theme->getPanelBorder().withAlpha(0.3f));
        g.drawLine(static_cast<float>(bounds.getWidth() / 2), 8.0f, 
                   static_cast<float>(bounds.getWidth() / 2), static_cast<float>(bounds.getHeight() - 8), 1.0f);
        
        // === CORNER DECORATIONS ===
        g.setColour(theme->getAccentPrimary().withAlpha(0.4f));
        g.fillRect(0, 0, 3, 10);
        g.fillRect(0, 0, 10, 3);
        g.fillRect(bounds.getWidth() - 3, 0, 3, 10);
        g.fillRect(bounds.getWidth() - 10, 0, 10, 3);
        g.fillRect(0, bounds.getHeight() - 10, 3, 10);
        g.fillRect(0, bounds.getHeight() - 3, 10, 3);
        g.fillRect(bounds.getWidth() - 3, bounds.getHeight() - 10, 3, 10);
        g.fillRect(bounds.getWidth() - 10, bounds.getHeight() - 3, 10, 3);
    }
    
private:
    void drawCompactBar(juce::Graphics& g, int x, int y, const juce::String& label, 
                        float value, juce::Colour color, int totalWidth)
    {
        int labelW = 25;
        int barWidth = totalWidth - labelW - 45;
        int barHeight = 6;
        
        // Label
        g.setColour(theme->getTextMuted());
        g.drawText(label + ":", x, y, labelW, 12, juce::Justification::left);
        
        // Bar background
        int barX = x + labelW;
        g.setColour(juce::Colour(0xff1a1a25));
        g.fillRect(barX, y + 3, barWidth, barHeight);
        
        // Value bar
        float normalizedValue = juce::jlimit(0.0f, 1.0f, std::abs(value));
        int fillWidth = static_cast<int>(normalizedValue * barWidth);
        
        if (fillWidth > 0)
        {
            g.setColour(color.withAlpha(0.7f));
            g.fillRect(barX, y + 3, fillWidth, barHeight);
        }
        
        // Value text
        g.setColour(theme->getTextSecondary());
        g.drawText(juce::String(value, 2), barX + barWidth + 4, y, 40, 12, juce::Justification::left);
    }
    
    void drawBipolarCompact(juce::Graphics& g, int x, int y, const juce::String& label,
                            float value, juce::Colour color, int totalWidth)
    {
        int labelW = 25;
        int barWidth = totalWidth - labelW - 45;
        int barHeight = 6;
        int barX = x + labelW;
        int centerX = barX + barWidth / 2;
        
        // Label
        g.setColour(theme->getTextMuted());
        g.drawText(label + ":", x, y, labelW, 12, juce::Justification::left);
        
        // Bar background
        g.setColour(juce::Colour(0xff1a1a25));
        g.fillRect(barX, y + 3, barWidth, barHeight);
        
        // Center line
        g.setColour(juce::Colour(0xff2a2a35));
        g.drawVerticalLine(centerX, static_cast<float>(y + 3), static_cast<float>(y + 3 + barHeight));
        
        // Value bar
        float normalizedValue = juce::jlimit(-1.0f, 1.0f, value);
        int fillWidth = static_cast<int>(std::abs(normalizedValue) * (barWidth / 2));
        
        if (fillWidth > 0)
        {
            g.setColour(color.withAlpha(0.7f));
            if (normalizedValue >= 0)
                g.fillRect(centerX, y + 3, fillWidth, barHeight);
            else
                g.fillRect(centerX - fillWidth, y + 3, fillWidth, barHeight);
        }
        
        // Value text
        g.setColour(theme->getTextSecondary());
        juce::String valStr = (value >= 0 ? "+" : "") + juce::String(value, 2);
        g.drawText(valStr, barX + barWidth + 4, y, 40, 12, juce::Justification::left);
    }
    
    void drawMiniBar(juce::Graphics& g, int x, int y, int width, int height, float value, juce::Colour color)
    {
        // Background
        g.setColour(juce::Colour(0xff1a1a25));
        g.fillRect(x, y, width, height);
        
        // Fill
        int fillWidth = static_cast<int>(juce::jlimit(0.0f, 1.0f, value) * width);
        if (fillWidth > 0)
        {
            g.setColour(color.withAlpha(0.7f));
            g.fillRect(x, y, fillWidth, height);
        }
    }
    
    void drawOutputMeter(juce::Graphics& g, int x, int y, float level, int totalWidth)
    {
        int meterWidth = totalWidth - 50;
        int meterHeight = 10;
        
        // Background
        g.setColour(juce::Colour(0xff1a1a25));
        g.fillRect(x, y, meterWidth, meterHeight);
        
        // Calculate dB
        float db = juce::Decibels::gainToDecibels(std::abs(level), -60.0f);
        float normalized = juce::jmap(db, -60.0f, 0.0f, 0.0f, 1.0f);
        normalized = juce::jlimit(0.0f, 1.0f, normalized);
        
        // Segments
        int numSegments = 15;
        int segmentWidth = (meterWidth - 2) / numSegments;
        int activeSegments = static_cast<int>(normalized * numSegments);
        
        for (int i = 0; i < numSegments; ++i)
        {
            juce::Colour segColor;
            if (i < 9)
                segColor = theme->getPositive();
            else if (i < 12)
                segColor = theme->getWarning();
            else
                segColor = theme->getNegative();
            
            g.setColour(i < activeSegments ? segColor.withAlpha(0.9f) : segColor.withAlpha(0.15f));
            g.fillRect(x + 1 + i * segmentWidth, y + 1, segmentWidth - 1, meterHeight - 2);
        }
        
        // dB value
        g.setColour(theme->getTextPrimary());
        juce::String dbStr = (level > 0.0001f) ? juce::String(db, 1) + "dB" : "-inf";
        g.drawText(dbStr, x + meterWidth + 4, y - 1, 45, meterHeight + 2, juce::Justification::left);
    }
    
    juce::String formatFreq(float freq)
    {
        if (freq >= 1000.0f)
            return juce::String(freq / 1000.0f, 1) + "kHz";
        return juce::String(static_cast<int>(freq)) + "Hz";
    }
    
    const Theme* theme = nullptr;
    
    int currentNote = -1;
    float currentVelocity = 0.0f;
    int activeVoices = 0;
    
    float osc1Value = 0.0f;
    float osc2Value = 0.0f;
    float subValue = 0.0f;
    
    float filterCutoff = 1000.0f;
    float filterReso = 0.0f;
    float filterEnv = 0.0f;
    
    float ampEnvValue = 0.0f;
    float filterEnvValue = 0.0f;
    
    float lfo1Value = 0.0f;
    float lfo2Value = 0.0f;
    
    float sbA = 0.0f;
    float sbB = 0.0f;
    float sbC = 0.0f;
    float sbD = 0.0f;
    
    float outputLevel = 0.0f;
};

/**
 * KndlSpellbookScope - Visualiza la forma del Spellbook como un XY scope estilo terminal.
 * Dibuja la trayectoria completa de la forma geométrica y un punto brillante
 * que indica la posición actual del modulador.
 */
class KndlSpellbookScope : public juce::Component
{
public:
    void setTheme(const Theme* newTheme)
    {
        theme = newTheme;
        repaint();
    }
    
    void setCurrentXY(float x, float y)
    {
        currentX = x;
        currentY = y;
        
        // Store trail
        trailX[static_cast<size_t>(trailIndex)] = x;
        trailY[static_cast<size_t>(trailIndex)] = y;
        trailIndex = (trailIndex + 1) % TrailLength;
    }
    
    void setShapeName(const juce::String& name) { shapeName = name; }
    
    void paint(juce::Graphics& g) override
    {
        if (!theme) return;
        
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(juce::Colour(0xff0a0a10));
        g.fillRoundedRectangle(bounds, 4.0f);
        
        // Border
        g.setColour(theme->getPanelBorder().withAlpha(0.4f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
        
        // Title
        g.setColour(theme->getAccentTertiary());
        g.setFont(theme->getSmallFont());
        g.drawText("SB.SHAPE", static_cast<int>(bounds.getX() + 4),
                   static_cast<int>(bounds.getY() + 2), 70, 12, juce::Justification::left);
        
        // Shape name
        g.setColour(theme->getTextMuted());
        g.drawText(shapeName, static_cast<int>(bounds.getRight() - 64),
                   static_cast<int>(bounds.getY() + 2), 60, 12, juce::Justification::right);
        
        // Plot area
        auto plotArea = bounds.reduced(6.0f, 16.0f).withTrimmedTop(2.0f);
        float cx = plotArea.getCentreX();
        float cy = plotArea.getCentreY();
        float scale = juce::jmin(plotArea.getWidth(), plotArea.getHeight()) * 0.42f;
        
        // Grid lines (crosshair)
        g.setColour(theme->getPanelBorder().withAlpha(0.15f));
        g.drawHorizontalLine(static_cast<int>(cy), plotArea.getX(), plotArea.getRight());
        g.drawVerticalLine(static_cast<int>(cx), plotArea.getY(), plotArea.getBottom());
        
        // Dotted circle at unit radius
        g.setColour(theme->getPanelBorder().withAlpha(0.1f));
        g.drawEllipse(cx - scale, cy - scale, scale * 2.0f, scale * 2.0f, 0.5f);
        
        // Draw the full shape path (thin, ghostly)
        {
            juce::Path shapePath;
            constexpr int numPoints = 200;
            bool first = true;
            for (int i = 0; i <= numPoints; ++i)
            {
                float phase = static_cast<float>(i) / static_cast<float>(numPoints);
                float sx, sy;
                generateShapePoint(phase, sx, sy);
                
                float px = cx + sx * scale;
                float py = cy - sy * scale; // Y inverted for screen
                
                if (first) { shapePath.startNewSubPath(px, py); first = false; }
                else shapePath.lineTo(px, py);
            }
            g.setColour(theme->getAccentTertiary().withAlpha(0.25f));
            g.strokePath(shapePath, juce::PathStrokeType(1.0f));
        }
        
        // Draw trail (fading dots showing recent positions)
        for (int i = 0; i < TrailLength; ++i)
        {
            int idx = (trailIndex - 1 - i + TrailLength) % TrailLength;
            float alpha = 0.5f * (1.0f - static_cast<float>(i) / static_cast<float>(TrailLength));
            if (alpha <= 0.0f) break;
            
            float tx = cx + trailX[static_cast<size_t>(idx)] * scale;
            float ty = cy - trailY[static_cast<size_t>(idx)] * scale;
            
            float dotSize = 2.0f + 2.0f * (1.0f - static_cast<float>(i) / static_cast<float>(TrailLength));
            g.setColour(theme->getAccentPrimary().withAlpha(alpha));
            g.fillEllipse(tx - dotSize * 0.5f, ty - dotSize * 0.5f, dotSize, dotSize);
        }
        
        // Draw current position (bright dot with glow)
        {
            float px = cx + currentX * scale;
            float py = cy - currentY * scale;
            
            // Glow
            g.setColour(theme->getAccentPrimary().withAlpha(0.2f));
            g.fillEllipse(px - 6.0f, py - 6.0f, 12.0f, 12.0f);
            
            // Core dot
            g.setColour(theme->getAccentPrimary());
            g.fillEllipse(px - 3.0f, py - 3.0f, 6.0f, 6.0f);
            
            // Bright center
            g.setColour(juce::Colours::white.withAlpha(0.8f));
            g.fillEllipse(px - 1.5f, py - 1.5f, 3.0f, 3.0f);
        }
        
        // Corner decorations
        g.setColour(theme->getAccentTertiary().withAlpha(0.3f));
        int x0 = static_cast<int>(bounds.getX()), y0 = static_cast<int>(bounds.getY());
        int w = static_cast<int>(bounds.getWidth()), h = static_cast<int>(bounds.getHeight());
        g.fillRect(x0, y0, 2, 6);
        g.fillRect(x0, y0, 6, 2);
        g.fillRect(x0 + w - 2, y0, 2, 6);
        g.fillRect(x0 + w - 6, y0, 6, 2);
        g.fillRect(x0, y0 + h - 6, 2, 6);
        g.fillRect(x0, y0 + h - 2, 6, 2);
        g.fillRect(x0 + w - 2, y0 + h - 6, 2, 6);
        g.fillRect(x0 + w - 6, y0 + h - 2, 6, 2);
    }
    
private:
    void generateShapePoint(float phase, float& x, float& y)
    {
        float angle = phase * 2.0f * juce::MathConstants<float>::pi;
        
        switch (currentShape)
        {
            case 0: // Circle
                x = std::cos(angle);
                y = std::sin(angle);
                break;
            case 1: // Triangle
            {
                float localAngle = std::fmod(angle, 2.0f * juce::MathConstants<float>::pi / 3.0f);
                float r = 1.0f / std::cos(localAngle - juce::MathConstants<float>::pi / 6.0f);
                r = juce::jlimit(-2.0f, 2.0f, r);
                x = r * std::cos(angle);
                y = r * std::sin(angle);
                break;
            }
            case 2: // Square
            {
                float n = 100.0f;
                float cosA = std::cos(angle);
                float sinA = std::sin(angle);
                x = (cosA >= 0 ? 1.0f : -1.0f) * std::pow(std::abs(cosA), 2.0f / n);
                y = (sinA >= 0 ? 1.0f : -1.0f) * std::pow(std::abs(sinA), 2.0f / n);
                break;
            }
            case 3: // Pentagon
            {
                float localAngle = std::fmod(angle, 2.0f * juce::MathConstants<float>::pi / 5.0f);
                float r = 1.0f / std::cos(localAngle - juce::MathConstants<float>::pi / 5.0f);
                r = juce::jlimit(-2.0f, 2.0f, r);
                x = r * std::cos(angle);
                y = r * std::sin(angle);
                break;
            }
            case 4: // Star
            {
                float starAngle = angle * 2.5f;
                float r = 0.5f + 0.5f * std::sin(starAngle * 2.0f);
                x = r * std::cos(angle);
                y = r * std::sin(angle);
                break;
            }
            case 5: // Spiral
            {
                float r = phase;
                x = r * std::cos(angle);
                y = r * std::sin(angle);
                break;
            }
            case 6: // Lemniscate
            {
                float cos2t = std::cos(2.0f * angle);
                if (cos2t < 0.0f) { x = 0.0f; y = 0.0f; break; }
                float r = std::sqrt(2.0f * cos2t);
                x = r * std::cos(angle);
                y = r * std::sin(angle);
                break;
            }
            default:
                x = std::cos(angle);
                y = std::sin(angle);
                break;
        }
        
        // Clamp for safety
        x = juce::jlimit(-1.5f, 1.5f, x);
        y = juce::jlimit(-1.5f, 1.5f, y);
    }
    
    const Theme* theme = nullptr;
    float currentX = 0.0f;
    float currentY = 0.0f;
    juce::String shapeName = "Circle";
    
    static constexpr int TrailLength = 48;
    std::array<float, TrailLength> trailX {};
    std::array<float, TrailLength> trailY {};
    int trailIndex = 0;
    
public:
    int currentShape = 0; // Matches Spellbook::Shape enum
};

} // namespace kndl::ui
