#pragma once

#include <JuceHeader.h>
#include "../skins/Theme.h"
#include "KndlKnob.h"

namespace kndl::ui {

/**
 * KndlOscSection - Sección de oscilador individual con controles y toggle.
 * Incluye: título, botón on/off, waveform selector, level, detune, octave.
 */
class KndlOscSection : public juce::Component
{
public:
    KndlOscSection(const juce::String& name)
        : sectionName(name),
          levelKnob("LEVEL"),
          detuneKnob("DETUNE"),
          octaveKnob("OCT")
    {
        addAndMakeVisible(levelKnob);
        addAndMakeVisible(detuneKnob);
        addAndMakeVisible(octaveKnob);
        addAndMakeVisible(waveformSelector);
        addAndMakeVisible(enableButton);
        
        // Setup waveform selector
        waveformSelector.addItem("SIN", 1);
        waveformSelector.addItem("TRI", 2);
        waveformSelector.addItem("SAW", 3);
        waveformSelector.addItem("SQR", 4);
        waveformSelector.setSelectedId(3);  // Default to SAW
        
        // Setup enable button
        enableButton.setButtonText("ON");
        enableButton.setClickingTogglesState(true);  // Important! Makes it a toggle button
        enableButton.setToggleState(true, juce::dontSendNotification);
        enableButton.onClick = [this]() {
            bool enabled = enableButton.getToggleState();
            enableButton.setButtonText(enabled ? "ON" : "OFF");
            levelKnob.setEnabled(enabled);
            detuneKnob.setEnabled(enabled);
            octaveKnob.setEnabled(enabled);
            waveformSelector.setEnabled(enabled);
            repaint();
        };
    }
    
    void setTheme(const Theme* newTheme)
    {
        theme = newTheme;
        levelKnob.setTheme(newTheme);
        detuneKnob.setTheme(newTheme);
        octaveKnob.setTheme(newTheme);
        
        if (theme)
        {
            // Style the combobox
            waveformSelector.setColour(juce::ComboBox::backgroundColourId, theme->getPanelBackground());
            waveformSelector.setColour(juce::ComboBox::textColourId, theme->getTextPrimary());
            waveformSelector.setColour(juce::ComboBox::outlineColourId, theme->getPanelBorder());
            waveformSelector.setColour(juce::ComboBox::arrowColourId, theme->getAccentPrimary());
            
            // Style the button
            enableButton.setColour(juce::TextButton::buttonColourId, theme->getPanelBackground());
            enableButton.setColour(juce::TextButton::buttonOnColourId, theme->getAccentPrimary().withAlpha(0.3f));
            enableButton.setColour(juce::TextButton::textColourOffId, theme->getTextMuted());
            enableButton.setColour(juce::TextButton::textColourOnId, theme->getAccentPrimary());
        }
        
        repaint();
    }
    
    // Getters for attachments
    juce::Slider& getLevelSlider() { return levelKnob; }
    juce::Slider& getDetuneSlider() { return detuneKnob; }
    juce::Slider& getOctaveSlider() { return octaveKnob; }
    juce::ComboBox& getWaveformSelector() { return waveformSelector; }
    juce::Button& getEnableButton() { return enableButton; }
    
    void paint(juce::Graphics& g) override
    {
        if (!theme) return;
        
        auto bounds = getLocalBounds().toFloat();
        bool enabled = enableButton.getToggleState();
        
        // Background
        g.setColour(theme->getPanelBackground().withAlpha(enabled ? 1.0f : 0.5f));
        g.fillRoundedRectangle(bounds, theme->getPanelCornerRadius());
        
        // Border
        g.setColour(enabled ? theme->getAccentPrimary().withAlpha(0.5f) : theme->getPanelBorder());
        g.drawRoundedRectangle(bounds.reduced(0.5f), theme->getPanelCornerRadius(), theme->getBorderWidth());
        
        // Title
        g.setColour(enabled ? theme->getTextPrimary() : theme->getTextMuted());
        g.setFont(theme->getHeaderFont());
        g.drawText(sectionName, 40, 8, 80, 20, juce::Justification::left);
        
        // Active indicator
        if (enabled)
        {
            g.setColour(theme->getAccentPrimary());
            g.fillRoundedRectangle(8.0f, 10.0f, 4.0f, 16.0f, 2.0f);
        }
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        
        // Header area (top 35px)
        auto headerArea = bounds.removeFromTop(35);
        
        // Enable button on the right of header
        enableButton.setBounds(headerArea.removeFromRight(45).reduced(5, 8));
        
        // Waveform selector
        waveformSelector.setBounds(headerArea.removeFromRight(60).reduced(2, 8));
        
        // Content area
        auto contentArea = bounds.reduced(8, 4);
        
        int knobSize = 48;
        int knobHeight = knobSize + 16;
        int knobGap = 6;
        
        // Center the knobs
        int totalWidth = knobSize * 3 + knobGap * 2;
        int startX = contentArea.getX() + (contentArea.getWidth() - totalWidth) / 2;
        int knobY = contentArea.getY() + (contentArea.getHeight() - knobHeight) / 2;
        
        levelKnob.setBounds(startX, knobY, knobSize, knobHeight);
        detuneKnob.setBounds(startX + knobSize + knobGap, knobY, knobSize, knobHeight);
        octaveKnob.setBounds(startX + (knobSize + knobGap) * 2, knobY, knobSize, knobHeight);
    }
    
private:
    const Theme* theme = nullptr;
    juce::String sectionName;
    
    KndlKnob levelKnob;
    KndlKnob detuneKnob;
    KndlKnob octaveKnob;
    
    juce::ComboBox waveformSelector;
    juce::TextButton enableButton;
};

/**
 * KndlSubSection - Sección simplificada para el sub-oscilador.
 */
class KndlSubSection : public juce::Component
{
public:
    KndlSubSection()
        : levelKnob("LEVEL"),
          octaveKnob("OCT")
    {
        addAndMakeVisible(levelKnob);
        addAndMakeVisible(octaveKnob);
        addAndMakeVisible(enableButton);
        
        enableButton.setButtonText("OFF");
        enableButton.setClickingTogglesState(true);  // Important! Makes it a toggle button
        enableButton.setToggleState(false, juce::dontSendNotification);  // Sub off by default
        
        // Disable knobs initially since sub is off
        levelKnob.setEnabled(false);
        octaveKnob.setEnabled(false);
        
        enableButton.onClick = [this]() {
            bool enabled = enableButton.getToggleState();
            enableButton.setButtonText(enabled ? "ON" : "OFF");
            levelKnob.setEnabled(enabled);
            octaveKnob.setEnabled(enabled);
            repaint();
        };
    }
    
    void setTheme(const Theme* newTheme)
    {
        theme = newTheme;
        levelKnob.setTheme(newTheme);
        octaveKnob.setTheme(newTheme);
        
        if (theme)
        {
            enableButton.setColour(juce::TextButton::buttonColourId, theme->getPanelBackground());
            enableButton.setColour(juce::TextButton::buttonOnColourId, theme->getAccentTertiary().withAlpha(0.3f));
            enableButton.setColour(juce::TextButton::textColourOffId, theme->getTextMuted());
            enableButton.setColour(juce::TextButton::textColourOnId, theme->getAccentTertiary());
        }
        
        repaint();
    }
    
    juce::Slider& getLevelSlider() { return levelKnob; }
    juce::Slider& getOctaveSlider() { return octaveKnob; }
    juce::Button& getEnableButton() { return enableButton; }
    
    void paint(juce::Graphics& g) override
    {
        if (!theme) return;
        
        auto bounds = getLocalBounds().toFloat();
        bool enabled = enableButton.getToggleState();
        
        // Background
        g.setColour(theme->getPanelBackground().withAlpha(enabled ? 1.0f : 0.5f));
        g.fillRoundedRectangle(bounds, theme->getPanelCornerRadius());
        
        // Border
        g.setColour(enabled ? theme->getAccentTertiary().withAlpha(0.5f) : theme->getPanelBorder());
        g.drawRoundedRectangle(bounds.reduced(0.5f), theme->getPanelCornerRadius(), theme->getBorderWidth());
        
        // Title
        g.setColour(enabled ? theme->getTextPrimary() : theme->getTextMuted());
        g.setFont(theme->getHeaderFont());
        g.drawText("SUB", 40, 8, 60, 20, juce::Justification::left);
        
        // Active indicator
        if (enabled)
        {
            g.setColour(theme->getAccentTertiary());
            g.fillRoundedRectangle(8.0f, 10.0f, 4.0f, 16.0f, 2.0f);
        }
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        
        // Header area
        auto headerArea = bounds.removeFromTop(35);
        enableButton.setBounds(headerArea.removeFromRight(45).reduced(5, 8));
        
        // Content area
        auto contentArea = bounds.reduced(8, 4);
        
        int knobSize = 48;
        int knobHeight = knobSize + 16;
        int knobGap = 10;
        
        // Center the 2 knobs
        int totalWidth = knobSize * 2 + knobGap;
        int startX = contentArea.getX() + (contentArea.getWidth() - totalWidth) / 2;
        int knobY = contentArea.getY() + (contentArea.getHeight() - knobHeight) / 2;
        
        levelKnob.setBounds(startX, knobY, knobSize, knobHeight);
        octaveKnob.setBounds(startX + knobSize + knobGap, knobY, knobSize, knobHeight);
    }
    
private:
    const Theme* theme = nullptr;
    
    KndlKnob levelKnob;
    KndlKnob octaveKnob;
    juce::TextButton enableButton;
};

} // namespace kndl::ui
