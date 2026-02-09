#pragma once

#include <JuceHeader.h>
#include "../skins/Theme.h"

namespace kndl::ui {

/**
 * KndlKnob - Rotary knob con modulation ring y glow effects.
 * Soporta temas intercambiables.
 */
class KndlKnob : public juce::Slider
{
public:
    KndlKnob(const juce::String& labelText = "")
        : label(labelText)
    {
        setSliderStyle(juce::Slider::RotaryVerticalDrag);
        setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                           juce::MathConstants<float>::pi * 2.75f, true);
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
    
    void setModulationAmount(float amount)
    {
        modulationAmount = juce::jlimit(-1.0f, 1.0f, amount);
        repaint();
    }
    
    void paint(juce::Graphics& g) override
    {
        if (!theme) return;
        
        auto bounds = getLocalBounds().toFloat();
        auto size = juce::jmin(bounds.getWidth(), bounds.getHeight() - 18.0f);
        auto knobBounds = juce::Rectangle<float>(
            (bounds.getWidth() - size) * 0.5f,
            0.0f,
            size,
            size
        );
        
        auto centre = knobBounds.getCentre();
        auto radius = size * 0.5f - 4.0f;
        
        // === OUTER RING / MODULATION RING ===
        if (std::abs(modulationAmount) > 0.001f && theme->hasGlowEffects())
        {
            float modRadius = radius + 6.0f;
            float startAngle = juce::MathConstants<float>::pi * 1.25f;
            float endAngle = juce::MathConstants<float>::pi * 2.75f;
            float currentAngle = startAngle + (endAngle - startAngle) * 
                                 static_cast<float>((getValue() - getMinimum()) / (getMaximum() - getMinimum()));
            float modAngle = (endAngle - startAngle) * modulationAmount * 0.5f;
            
            juce::Path modPath;
            modPath.addCentredArc(centre.x, centre.y, modRadius, modRadius,
                                  0.0f, currentAngle, currentAngle + modAngle, true);
            
            g.setColour(theme->getKnobModulationRing().withAlpha(0.7f));
            g.strokePath(modPath, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
            
            // Glow
            g.setColour(theme->getKnobModulationRing().withAlpha(0.2f));
            g.strokePath(modPath, juce::PathStrokeType(8.0f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
        }
        
        // === TRACK ARC ===
        {
            juce::Path trackPath;
            trackPath.addCentredArc(centre.x, centre.y, radius, radius,
                                    0.0f,
                                    juce::MathConstants<float>::pi * 1.25f,
                                    juce::MathConstants<float>::pi * 2.75f,
                                    true);
            g.setColour(theme->getKnobBackground());
            g.strokePath(trackPath, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));
        }
        
        // === VALUE ARC ===
        {
            float startAngle = juce::MathConstants<float>::pi * 1.25f;
            float endAngle = juce::MathConstants<float>::pi * 2.75f;
            float valueAngle = startAngle + (endAngle - startAngle) * 
                               static_cast<float>((getValue() - getMinimum()) / (getMaximum() - getMinimum()));
            
            juce::Path valuePath;
            valuePath.addCentredArc(centre.x, centre.y, radius, radius,
                                    0.0f, startAngle, valueAngle, true);
            
            g.setColour(theme->getKnobIndicator());
            g.strokePath(valuePath, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));
        }
        
        // === KNOB BODY ===
        float bodyRadius = radius - 8.0f;
        
        // Shadow
        if (theme->hasDropShadows())
        {
            g.setColour(juce::Colours::black.withAlpha(theme->getShadowOpacity()));
            g.fillEllipse(centre.x - bodyRadius + 2.0f, centre.y - bodyRadius + 2.0f,
                         bodyRadius * 2.0f, bodyRadius * 2.0f);
        }
        
        // Body gradient
        juce::ColourGradient bodyGradient(
            theme->getKnobForeground().brighter(0.1f),
            centre.x, centre.y - bodyRadius,
            theme->getKnobBackground(),
            centre.x, centre.y + bodyRadius,
            false
        );
        g.setGradientFill(bodyGradient);
        g.fillEllipse(centre.x - bodyRadius, centre.y - bodyRadius,
                     bodyRadius * 2.0f, bodyRadius * 2.0f);
        
        // Inner ring
        g.setColour(theme->getPanelBorder());
        g.drawEllipse(centre.x - bodyRadius, centre.y - bodyRadius,
                     bodyRadius * 2.0f, bodyRadius * 2.0f, 1.0f);
        
        // === INDICATOR LINE ===
        {
            float startAngle = juce::MathConstants<float>::pi * 1.25f;
            float endAngle = juce::MathConstants<float>::pi * 2.75f;
            float angle = startAngle + (endAngle - startAngle) * 
                          static_cast<float>((getValue() - getMinimum()) / (getMaximum() - getMinimum()));
            
            float indicatorLength = bodyRadius * 0.6f;
            float indicatorStart = bodyRadius * 0.2f;
            
            juce::Point<float> startPoint(
                centre.x + std::sin(angle) * indicatorStart,
                centre.y - std::cos(angle) * indicatorStart
            );
            juce::Point<float> endPoint(
                centre.x + std::sin(angle) * indicatorLength,
                centre.y - std::cos(angle) * indicatorLength
            );
            
            g.setColour(theme->getKnobIndicator());
            g.drawLine(startPoint.x, startPoint.y, endPoint.x, endPoint.y, 2.5f);
            
            // Glow on indicator
            if (theme->hasGlowEffects())
            {
                g.setColour(theme->getAccentGlow().withAlpha(0.3f));
                g.drawLine(startPoint.x, startPoint.y, endPoint.x, endPoint.y, 5.0f);
            }
        }
        
        // === LABEL ===
        if (label.isNotEmpty())
        {
            g.setColour(theme->getTextSecondary());
            g.setFont(theme->getLabelFont());
            g.drawText(label, 0, static_cast<int>(size) + 2, 
                      static_cast<int>(bounds.getWidth()), 16,
                      juce::Justification::centred);
        }
    }
    
private:
    const Theme* theme = nullptr;
    juce::String label;
    float modulationAmount = 0.0f;
};

} // namespace kndl::ui
