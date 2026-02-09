#pragma once

#include <JuceHeader.h>

namespace kndl::ui {

/**
 * Theme - Define los colores, fuentes y estilos de un skin.
 * Cada skin hereda de esta clase y define su propia paleta.
 */
struct Theme
{
    virtual ~Theme() = default;
    
    // === COLORES DE FONDO ===
    virtual juce::Colour getBackgroundPrimary() const = 0;
    virtual juce::Colour getBackgroundSecondary() const = 0;
    virtual juce::Colour getBackgroundTertiary() const = 0;
    virtual juce::Colour getPanelBackground() const = 0;
    virtual juce::Colour getPanelBorder() const = 0;
    
    // === COLORES DE ACENTO ===
    virtual juce::Colour getAccentPrimary() const = 0;
    virtual juce::Colour getAccentSecondary() const = 0;
    virtual juce::Colour getAccentTertiary() const = 0;
    virtual juce::Colour getAccentGlow() const = 0;
    
    // === COLORES DE TEXTO ===
    virtual juce::Colour getTextPrimary() const = 0;
    virtual juce::Colour getTextSecondary() const = 0;
    virtual juce::Colour getTextMuted() const = 0;
    virtual juce::Colour getTextHighlight() const = 0;
    
    // === COLORES DE CONTROLES ===
    virtual juce::Colour getKnobBackground() const = 0;
    virtual juce::Colour getKnobForeground() const = 0;
    virtual juce::Colour getKnobIndicator() const = 0;
    virtual juce::Colour getKnobModulationRing() const = 0;
    
    virtual juce::Colour getSliderTrack() const = 0;
    virtual juce::Colour getSliderFill() const = 0;
    virtual juce::Colour getSliderThumb() const = 0;
    
    virtual juce::Colour getMeterBackground() const = 0;
    virtual juce::Colour getMeterLow() const = 0;
    virtual juce::Colour getMeterMid() const = 0;
    virtual juce::Colour getMeterHigh() const = 0;
    
    // === COLORES DE ESTADO ===
    virtual juce::Colour getPositive() const = 0;  // OK, active
    virtual juce::Colour getWarning() const = 0;   // Warning
    virtual juce::Colour getNegative() const = 0;  // Error, clip
    
    // === FUENTES ===
    virtual juce::Font getTitleFont() const = 0;
    virtual juce::Font getHeaderFont() const = 0;
    virtual juce::Font getLabelFont() const = 0;
    virtual juce::Font getValueFont() const = 0;
    virtual juce::Font getSmallFont() const = 0;
    
    // === DIMENSIONES ===
    virtual float getKnobSize() const { return 60.0f; }
    virtual float getMacroKnobSize() const { return 80.0f; }
    virtual float getSliderWidth() const { return 30.0f; }
    virtual float getPanelCornerRadius() const { return 8.0f; }
    virtual float getBorderWidth() const { return 1.5f; }
    virtual float getGlowRadius() const { return 8.0f; }
    
    // === TEXTURAS Y EFECTOS ===
    virtual bool hasTexturedBackground() const { return false; }
    virtual bool hasGlowEffects() const { return true; }
    virtual bool hasDropShadows() const { return true; }
    virtual float getShadowOpacity() const { return 0.3f; }
    
    // === NOMBRE DEL TEMA ===
    virtual juce::String getName() const = 0;
};

} // namespace kndl::ui
