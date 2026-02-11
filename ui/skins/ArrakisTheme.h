#pragma once

#include "Theme.h"
#include "FontManager.h"

namespace kndl::ui {

/**
 * ArrakisTheme - Dune-inspired skin
 * 
 * Dark, ancient-futuristic aesthetic inspired by Arrakis:
 * - Weathered metal panels, brushed brass and bronze details
 * - Sand-worn textures, subtle engravings
 * - Warm amber, spice-orange, and soft cyan glowing accents
 * - Mystical, serious, hypnotic, desert-born technology
 */
class ArrakisTheme : public Theme
{
public:
    // === COLORES DE FONDO ===
    // Dark graphite and sand-colored background
    juce::Colour getBackgroundPrimary() const override 
    { 
        return juce::Colour(0xff1a1714);  // Deep desert night
    }
    
    juce::Colour getBackgroundSecondary() const override 
    { 
        return juce::Colour(0xff252019);  // Weathered metal
    }
    
    juce::Colour getBackgroundTertiary() const override 
    { 
        return juce::Colour(0xff2d2820);  // Sand shadow
    }
    
    juce::Colour getPanelBackground() const override 
    { 
        return juce::Colour(0xff1e1b16);  // Brushed bronze dark
    }
    
    juce::Colour getPanelBorder() const override 
    { 
        return juce::Colour(0xff3d352a);  // Oxidized brass edge
    }
    
    // === COLORES DE ACENTO ===
    // Warm amber, spice-orange, and soft cyan
    juce::Colour getAccentPrimary() const override 
    { 
        return juce::Colour(0xffd4a055);  // Spice orange / amber
    }
    
    juce::Colour getAccentSecondary() const override 
    { 
        return juce::Colour(0xffb8860b);  // Dark goldenrod / brass
    }
    
    juce::Colour getAccentTertiary() const override 
    { 
        return juce::Colour(0xff5fb4a2);  // Fremen blue / cyan
    }
    
    juce::Colour getAccentGlow() const override 
    { 
        return juce::Colour(0xffff9f43);  // Warm glow
    }
    
    // === COLORES DE TEXTO ===
    juce::Colour getTextPrimary() const override 
    { 
        return juce::Colour(0xffe8dcc8);  // Sand white
    }
    
    juce::Colour getTextSecondary() const override 
    { 
        return juce::Colour(0xffa89880);  // Weathered text
    }
    
    juce::Colour getTextMuted() const override 
    { 
        return juce::Colour(0xff6b5d4d);  // Faded engraving
    }
    
    juce::Colour getTextHighlight() const override 
    { 
        return juce::Colour(0xfff4d03f);  // Spice highlight
    }
    
    // === COLORES DE CONTROLES ===
    juce::Colour getKnobBackground() const override 
    { 
        return juce::Colour(0xff2a2520);  // Dark bronze
    }
    
    juce::Colour getKnobForeground() const override 
    { 
        return juce::Colour(0xff4a4035);  // Brushed metal
    }
    
    juce::Colour getKnobIndicator() const override 
    { 
        return juce::Colour(0xffd4a055);  // Amber indicator
    }
    
    juce::Colour getKnobModulationRing() const override 
    { 
        return juce::Colour(0xff5fb4a2);  // Cyan modulation
    }
    
    juce::Colour getSliderTrack() const override 
    { 
        return juce::Colour(0xff2a2520);
    }
    
    juce::Colour getSliderFill() const override 
    { 
        return juce::Colour(0xffd4a055);
    }
    
    juce::Colour getSliderThumb() const override 
    { 
        return juce::Colour(0xffe8dcc8);
    }
    
    juce::Colour getMeterBackground() const override 
    { 
        return juce::Colour(0xff1a1714);
    }
    
    juce::Colour getMeterLow() const override 
    { 
        return juce::Colour(0xff5fb4a2);  // Cyan
    }
    
    juce::Colour getMeterMid() const override 
    { 
        return juce::Colour(0xffd4a055);  // Amber
    }
    
    juce::Colour getMeterHigh() const override 
    { 
        return juce::Colour(0xffcc4125);  // Spice red
    }
    
    // === COLORES DE ESTADO ===
    juce::Colour getPositive() const override 
    { 
        return juce::Colour(0xff5fb4a2);  // Fremen blue
    }
    
    juce::Colour getWarning() const override 
    { 
        return juce::Colour(0xffd4a055);  // Amber warning
    }
    
    juce::Colour getNegative() const override 
    { 
        return juce::Colour(0xffcc4125);  // Deep red
    }
    
    // === FUENTES ===
    // Inconsolata - Terminal monospace font for retro-futuristic aesthetic
    // https://fonts.google.com/specimen/Inconsolata
    // Embedded via BinaryData (FontManager singleton)
    juce::Font getTitleFont() const override 
    { 
        return FontManager::getInstance().getBold(28.0f);
    }
    
    juce::Font getHeaderFont() const override 
    { 
        return FontManager::getInstance().getBold(14.0f);
    }
    
    juce::Font getLabelFont() const override 
    { 
        return FontManager::getInstance().getRegular(11.0f);
    }
    
    juce::Font getValueFont() const override 
    { 
        return FontManager::getInstance().getRegular(10.0f);
    }
    
    juce::Font getSmallFont() const override 
    { 
        return FontManager::getInstance().getRegular(9.0f);
    }
    
    // === DIMENSIONES ===
    float getKnobSize() const override { return 56.0f; }
    float getMacroKnobSize() const override { return 72.0f; }
    float getSliderWidth() const override { return 24.0f; }
    float getPanelCornerRadius() const override { return 6.0f; }
    float getBorderWidth() const override { return 1.0f; }
    float getGlowRadius() const override { return 10.0f; }
    
    // === TEXTURAS Y EFECTOS ===
    bool hasTexturedBackground() const override { return true; }
    bool hasGlowEffects() const override { return true; }
    bool hasDropShadows() const override { return true; }
    float getShadowOpacity() const override { return 0.4f; }
    
    // === NOMBRE ===
    juce::String getName() const override { return "Arrakis"; }
};

} // namespace kndl::ui
