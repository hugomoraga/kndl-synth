#pragma once

#include <JuceHeader.h>
#include "../skins/Theme.h"

namespace kndl::ui {

/**
 * KndlPanel - Panel contenedor con título y borde estilizado.
 */
class KndlPanel : public juce::Component
{
public:
    KndlPanel(const juce::String& titleText = "")
        : title(titleText)
    {
    }
    
    void setTheme(const Theme* newTheme)
    {
        theme = newTheme;
        repaint();
    }
    
    void setTitle(const juce::String& newTitle)
    {
        title = newTitle;
        repaint();
    }
    
    juce::Rectangle<int> getContentBounds() const
    {
        auto bounds = getLocalBounds();
        int headerHeight = title.isEmpty() ? 8 : 28;
        return bounds.reduced(8).withTrimmedTop(headerHeight);
    }
    
    void paint(juce::Graphics& g) override
    {
        if (!theme) return;
        
        auto bounds = getLocalBounds().toFloat();
        float cornerRadius = theme->getPanelCornerRadius();
        
        // === SHADOW ===
        if (theme->hasDropShadows())
        {
            g.setColour(juce::Colours::black.withAlpha(theme->getShadowOpacity() * 0.5f));
            g.fillRoundedRectangle(bounds.translated(2.0f, 2.0f), cornerRadius);
        }
        
        // === BACKGROUND ===
        g.setColour(theme->getPanelBackground());
        g.fillRoundedRectangle(bounds, cornerRadius);
        
        // === BORDER ===
        g.setColour(theme->getPanelBorder());
        g.drawRoundedRectangle(bounds.reduced(0.5f), cornerRadius, theme->getBorderWidth());
        
        // === SUBTLE TEXTURE (for Arrakis theme) ===
        if (theme->hasTexturedBackground())
        {
            // Simulated brushed metal / sand texture with noise pattern
            juce::Random rng(42);  // Fixed seed for consistent pattern
            g.setColour(theme->getBackgroundTertiary().withAlpha(0.03f));
            
            for (int i = 0; i < 50; ++i)
            {
                float x = rng.nextFloat() * bounds.getWidth();
                float y = rng.nextFloat() * bounds.getHeight();
                float w = 1.0f + rng.nextFloat() * 3.0f;
                g.fillRect(x, y, w, 1.0f);
            }
        }
        
        // === TITLE ===
        if (title.isNotEmpty())
        {
            auto titleBounds = bounds.removeFromTop(26.0f).reduced(12.0f, 4.0f);
            
            g.setColour(theme->getTextMuted());
            g.setFont(theme->getHeaderFont());
            g.drawText(title, titleBounds, juce::Justification::left);
            
            // Subtle line under title
            g.setColour(theme->getPanelBorder().withAlpha(0.5f));
            g.drawHorizontalLine(static_cast<int>(titleBounds.getBottom() + 2.0f),
                                bounds.getX() + 8.0f,
                                bounds.getRight() - 8.0f);
        }
    }
    
private:
    const Theme* theme = nullptr;
    juce::String title;
};

} // namespace kndl::ui
