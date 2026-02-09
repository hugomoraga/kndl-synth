#pragma once

#include <JuceHeader.h>
#include "../skins/Theme.h"

namespace kndl::ui {

/**
 * KndlSlider - Vertical ADSR-style slider con soporte de temas.
 */
class KndlSlider : public juce::Slider
{
public:
    KndlSlider(const juce::String& labelText = "")
        : label(labelText)
    {
        setSliderStyle(juce::Slider::LinearVertical);
        setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    }
    
    void setTheme(const Theme* newTheme)
    {
        theme = newTheme;
        repaint();
    }
    
    void setLabel(const juce::String& newLabel)
    {
        label = newLabel;
        repaint();
    }
    
    void paint(juce::Graphics& g) override
    {
        if (!theme) return;
        
        auto bounds = getLocalBounds().toFloat();
        float labelHeight = 16.0f;
        float trackWidth = theme->getSliderWidth();
        
        // Track area
        auto trackBounds = bounds.reduced((bounds.getWidth() - trackWidth) * 0.5f, 0.0f);
        trackBounds.removeFromBottom(labelHeight + 4.0f);
        trackBounds.removeFromTop(4.0f);
        
        float cornerRadius = trackWidth * 0.3f;
        
        // === TRACK BACKGROUND ===
        g.setColour(theme->getSliderTrack());
        g.fillRoundedRectangle(trackBounds, cornerRadius);
        
        // === FILL ===
        float normalizedValue = static_cast<float>((getValue() - getMinimum()) / (getMaximum() - getMinimum()));
        float fillHeight = trackBounds.getHeight() * normalizedValue;
        
        auto fillBounds = trackBounds.withTop(trackBounds.getBottom() - fillHeight);
        
        // Gradient fill
        juce::ColourGradient fillGradient(
            theme->getSliderFill().brighter(0.2f),
            fillBounds.getX(), fillBounds.getY(),
            theme->getSliderFill().darker(0.1f),
            fillBounds.getX(), fillBounds.getBottom(),
            false
        );
        g.setGradientFill(fillGradient);
        g.fillRoundedRectangle(fillBounds, cornerRadius);
        
        // Glow at top of fill
        if (theme->hasGlowEffects() && fillHeight > 5.0f)
        {
            g.setColour(theme->getAccentGlow().withAlpha(0.4f));
            g.fillRoundedRectangle(fillBounds.withHeight(6.0f), cornerRadius);
        }
        
        // === THUMB ===
        float thumbY = trackBounds.getBottom() - fillHeight;
        float thumbHeight = 6.0f;
        auto thumbBounds = juce::Rectangle<float>(
            trackBounds.getX() - 3.0f,
            thumbY - thumbHeight * 0.5f,
            trackBounds.getWidth() + 6.0f,
            thumbHeight
        );
        
        g.setColour(theme->getSliderThumb());
        g.fillRoundedRectangle(thumbBounds, 2.0f);
        
        // === LABEL ===
        if (label.isNotEmpty())
        {
            g.setColour(theme->getTextSecondary());
            g.setFont(theme->getLabelFont());
            g.drawText(label, 0, static_cast<int>(bounds.getHeight() - labelHeight),
                      static_cast<int>(bounds.getWidth()), static_cast<int>(labelHeight),
                      juce::Justification::centred);
        }
    }
    
private:
    const Theme* theme = nullptr;
    juce::String label;
};

} // namespace kndl::ui
