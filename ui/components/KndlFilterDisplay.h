#pragma once

#include <JuceHeader.h>
#include "../skins/Theme.h"
#include <array>
#include <cmath>

namespace kndl::ui {

/**
 * KndlFilterDisplay - Visualizador ASCII art de la respuesta del filtro.
 * Muestra la respuesta de frecuencia del filtro en tiempo real con estilo terminal.
 */
class KndlFilterDisplay : public juce::Component
{
public:
    static constexpr int Width = 64;  // Ancho en caracteres
    static constexpr int Height = 16; // Alto en líneas
    
    KndlFilterDisplay()
    {
        setSize(Width * 8, Height * 12); // Aproximadamente 8x12 pixels por carácter
    }
    
    void setTheme(const Theme* newTheme)
    {
        theme = newTheme;
        repaint();
    }
    
    void updateFilterResponse(float cutoff, float resonance, int filterType,
                              int filterMode = 0, int formantVowel = 0)
    {
        currentFilterMode = filterMode;
        
        const float sr = 44100.0f;
        const float pi = juce::MathConstants<float>::pi;
        const int numPoints = Width;
        
        for (int i = 0; i < numPoints; ++i)
        {
            // Logarithmic frequency 20Hz - 20kHz
            float freq = 20.0f * std::pow(1000.0f, static_cast<float>(i) / (numPoints - 1));
            float magnitude = 1.0f;
            
            switch (filterMode)
            {
                case 0: // SVF (LP / HP / BP)
                    magnitude = computeSVF(freq, cutoff, resonance, filterType, sr, pi);
                    break;
                    
                case 1: // Formant
                    magnitude = computeFormant(freq, cutoff, resonance, formantVowel, sr, pi);
                    break;
                    
                case 2: // Comb
                    magnitude = computeComb(freq, cutoff, resonance, sr, pi);
                    break;
                    
                case 3: // Notch
                    magnitude = computeNotch(freq, cutoff, resonance, sr, pi);
                    break;
                    
                default:
                    magnitude = 1.0f;
                    break;
            }
            
            float db = 20.0f * std::log10(magnitude + 1e-10f);
            db = juce::jlimit(-60.0f, 12.0f, db);
            response[static_cast<size_t>(i)] = (db + 60.0f) / 72.0f; // Normalize 0-1
        }
        
        repaint();
    }
    
    void paint(juce::Graphics& g) override
    {
        if (!theme) return;
        
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(juce::Colour(0xff0a0a0f));
        g.fillRoundedRectangle(bounds, 4.0f);
        
        // Border
        g.setColour(theme->getPanelBorder().withAlpha(0.5f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
        
        // Title + mode label
        g.setColour(theme->getTextMuted());
        g.setFont(theme->getSmallFont());
        g.drawText("FILT.RESP", static_cast<int>(bounds.getX() + 4), static_cast<int>(bounds.getY() + 2), 60, 12, juce::Justification::left);
        
        static const char* modeNames[] = { "SVF", "FORMANT", "COMB", "NOTCH" };
        int modeIdx = juce::jlimit(0, 3, currentFilterMode);
        g.setColour(theme->getAccentTertiary().withAlpha(0.7f));
        g.drawText(modeNames[modeIdx], static_cast<int>(bounds.getRight() - 54),
                   static_cast<int>(bounds.getY() + 2), 50, 12, juce::Justification::right);
        
        // Drawing area (leave space for title and labels)
        auto drawArea = bounds.reduced(4, 16);
        
        // Grid
        g.setColour(juce::Colour(0xff1a1a25));
        for (int i = 0; i <= Width; ++i)
        {
            float x = drawArea.getX() + (static_cast<float>(i) / Width) * drawArea.getWidth();
            g.drawVerticalLine(static_cast<int>(x), drawArea.getY(), drawArea.getBottom());
        }
        for (int i = 0; i <= Height; ++i)
        {
            float y = drawArea.getY() + (static_cast<float>(i) / Height) * drawArea.getHeight();
            g.drawHorizontalLine(static_cast<int>(y), drawArea.getX(), drawArea.getRight());
        }
        
        // Frequency labels (left side)
        g.setColour(theme->getTextMuted().withAlpha(0.6f));
        juce::Font labelFont = theme->getSmallFont();
        labelFont.setHeight(7.0f);
        g.setFont(labelFont);
        g.drawText("20k", static_cast<int>(drawArea.getX() - 20), static_cast<int>(drawArea.getY() - 8), 18, 8, juce::Justification::right);
        g.drawText("1k", static_cast<int>(drawArea.getX() - 20), static_cast<int>(drawArea.getCentreY() - 4), 18, 8, juce::Justification::right);
        g.drawText("20", static_cast<int>(drawArea.getX() - 20), static_cast<int>(drawArea.getBottom() - 8), 18, 8, juce::Justification::right);
        
        // dB labels (right side)
        g.drawText("+6", static_cast<int>(drawArea.getRight() + 2), static_cast<int>(drawArea.getY() - 8), 18, 8, juce::Justification::left);
        g.drawText("0", static_cast<int>(drawArea.getRight() + 2), static_cast<int>(drawArea.getCentreY() - 4), 18, 8, juce::Justification::left);
        g.drawText("-60", static_cast<int>(drawArea.getRight() + 2), static_cast<int>(drawArea.getBottom() - 8), 18, 8, juce::Justification::left);
        
        // Draw ASCII art response
        drawASCIIResponse(g, drawArea);
    }
    
private:
    void drawASCIIResponse(juce::Graphics& g, juce::Rectangle<float> area)
    {
        if (!theme) return;
        
        // Caracteres ASCII para diferentes niveles
        const char* chars = " .:-=+*#%@";
        const int numChars = 9;
        
        // Calcular tamaño de celda
        float cellWidth = area.getWidth() / Width;
        float cellHeight = area.getHeight() / Height;
        
        // Font para ASCII (Inconsolata para consistencia)
        juce::Font asciiFont(juce::FontOptions("Inconsolata", cellHeight * 0.8f, juce::Font::plain));
        g.setFont(asciiFont);
        
        // Dibujar respuesta punto por punto
        for (int x = 0; x < Width; ++x)
        {
            float responseValue = response[static_cast<size_t>(x)];
            
            // Calcular altura en celdas (invertido: arriba = más dB)
            int heightInCells = static_cast<int>(responseValue * Height);
            heightInCells = juce::jlimit(0, Height - 1, heightInCells);
            
            // Dibujar columna
            for (int y = 0; y < Height; ++y)
            {
                int yPos = Height - 1 - y; // Invertir Y
                
                float cellX = area.getX() + static_cast<float>(x) * cellWidth;
                float cellY = area.getY() + static_cast<float>(y) * cellHeight;
                
                // Determinar qué carácter usar
                int charIndex;
                if (yPos < heightInCells)
                {
                    // Dentro de la respuesta: usar gradiente
                    float normalizedY = static_cast<float>(yPos) / static_cast<float>(heightInCells + 1);
                    charIndex = static_cast<int>(normalizedY * numChars);
                    charIndex = juce::jlimit(0, numChars - 1, charIndex);
                }
                else if (yPos == heightInCells && heightInCells > 0)
                {
                    // Borde superior: usar carácter más brillante
                    charIndex = numChars - 1;
                }
                else
                {
                    // Fuera de la respuesta: espacio
                    continue;
                }
                
                // Color según nivel
                float brightness = static_cast<float>(charIndex) / (numChars - 1);
                juce::Colour cellColor = theme->getAccentPrimary()
                    .withBrightness(0.3f + brightness * 0.7f)
                    .withAlpha(0.6f + brightness * 0.4f);
                
                g.setColour(cellColor);
                g.drawText(juce::String::charToString(chars[charIndex]),
                           static_cast<int>(cellX), static_cast<int>(cellY),
                           static_cast<int>(cellWidth), static_cast<int>(cellHeight),
                           juce::Justification::centred);
            }
        }
        
        // Dibujar línea suave encima del ASCII (opcional, para mejor visualización)
        juce::Path responsePath;
        bool started = false;
        
        for (int x = 0; x < Width; ++x)
        {
            float responseValue = response[static_cast<size_t>(x)];
            float y = area.getBottom() - responseValue * area.getHeight();
            float xPos = area.getX() + (static_cast<float>(x) / Width) * area.getWidth();
            
            if (!started)
            {
                responsePath.startNewSubPath(xPos, y);
                started = true;
            }
            else
            {
                responsePath.lineTo(xPos, y);
            }
        }
        
        // Dibujar línea con color brillante
        g.setColour(theme->getAccentPrimary().withAlpha(0.8f));
        g.strokePath(responsePath, juce::PathStrokeType(1.5f));
    }
    
    // === Filter response computation per mode ===
    
    static float computeSVF(float freq, float cutoff, float resonance, int filterType,
                            float sr, float pi)
    {
        float w  = 2.0f * pi * freq / sr;
        float wc = 2.0f * pi * cutoff / sr;
        float Q  = 0.5f + (1.0f - resonance) * 9.5f;
        float w2 = w * w, wc2 = wc * wc;
        float bw = wc * w / Q;
        float denom = std::sqrt((wc2 - w2) * (wc2 - w2) + bw * bw + 1e-10f);
        
        if (filterType == 0)      return wc2 / denom;           // LP
        else if (filterType == 1) return w2 / denom;            // HP
        else                      return bw / denom;            // BP
    }
    
    static float computeFormant(float freq, float cutoff, float resonance, int vowel,
                                float sr, float pi)
    {
        // Formant frequencies for A E I O U (F1, F2, F3)
        static const float formants[5][3] = {
            { 800.0f,  1200.0f, 2800.0f },  // A
            { 400.0f,  2200.0f, 2800.0f },  // E
            { 350.0f,  2300.0f, 3200.0f },  // I
            { 500.0f,  1000.0f, 2800.0f },  // O
            { 400.0f,   750.0f, 2400.0f },  // U
        };
        vowel = juce::jlimit(0, 4, vowel);
        
        // Shift formants based on cutoff (center around 2kHz)
        float shift = cutoff / 2000.0f;
        float Q = 2.0f + resonance * 18.0f;
        
        float totalMag = 0.0f;
        for (int f = 0; f < 3; ++f)
        {
            float fc = formants[vowel][f] * shift;
            float w  = 2.0f * pi * freq / sr;
            float wc = 2.0f * pi * fc / sr;
            float bw = wc * w / Q;
            float diff = wc * wc - w * w;
            float denom = std::sqrt(diff * diff + bw * bw + 1e-10f);
            totalMag += bw / denom; // BP peaks at formant frequencies
        }
        return totalMag;
    }
    
    static float computeComb(float freq, float cutoff, float resonance,
                             float /*sr*/, float /*pi*/)
    {
        // Comb filter: peaks at multiples of fundamental frequency
        float delay = 1.0f / juce::jmax(20.0f, cutoff); // delay in seconds
        float fb = resonance * 0.95f;
        
        float w = 2.0f * juce::MathConstants<float>::pi * freq * delay;
        // H(f) = 1 / (1 - fb * e^{-jwT})
        float cosW = std::cos(w);
        float denom = std::sqrt((1.0f - fb * cosW) * (1.0f - fb * cosW)
                               + (fb * std::sin(w)) * (fb * std::sin(w)) + 1e-10f);
        return 1.0f / denom;
    }
    
    static float computeNotch(float freq, float cutoff, float resonance,
                              float sr, float pi)
    {
        // Notch = Band-reject: flat everywhere except a dip at cutoff
        float w  = 2.0f * pi * freq / sr;
        float wc = 2.0f * pi * cutoff / sr;
        float Q  = 1.0f + resonance * 19.0f; // narrow notch at high reso
        float w2 = w * w, wc2 = wc * wc;
        float bw = wc * w / Q;
        float denom = std::sqrt((wc2 - w2) * (wc2 - w2) + bw * bw + 1e-10f);
        
        // Notch: passes low and high, rejects center
        float diff = std::abs(wc2 - w2);
        return diff / denom;
    }
    
    const Theme* theme = nullptr;
    int currentFilterMode = 0;
    std::array<float, Width> response { 0.0f };
};

} // namespace kndl::ui
