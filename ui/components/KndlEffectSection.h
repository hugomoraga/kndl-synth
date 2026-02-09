#pragma once

#include <JuceHeader.h>
#include "../skins/Theme.h"
#include "KndlKnob.h"

namespace kndl::ui {

/**
 * KndlEffectSection - Sección de efecto individual con toggle y controles.
 */
class KndlEffectSection : public juce::Component
{
public:
    KndlEffectSection(const juce::String& name, std::initializer_list<juce::String> knobLabels)
        : effectName(name)
    {
        addAndMakeVisible(enableButton);
        
        enableButton.setButtonText("OFF");
        enableButton.setClickingTogglesState(true);
        enableButton.setToggleState(false, juce::dontSendNotification);
        
        enableButton.onClick = [this]() {
            bool on = enableButton.getToggleState();
            enableButton.setButtonText(on ? "ON" : "OFF");
            for (auto& knob : knobs)
                knob->setEnabled(on);
            repaint();
        };
        
        // Crear knobs según las labels
        for (const auto& label : knobLabels)
        {
            auto knob = std::make_unique<KndlKnob>(label);
            knob->setEnabled(false);
            addAndMakeVisible(*knob);
            knobs.push_back(std::move(knob));
        }
    }
    
    void setTheme(const Theme* newTheme)
    {
        theme = newTheme;
        
        for (auto& knob : knobs)
            knob->setTheme(newTheme);
        
        if (theme)
        {
            enableButton.setColour(juce::TextButton::buttonColourId, theme->getPanelBackground());
            enableButton.setColour(juce::TextButton::buttonOnColourId, theme->getAccentSecondary().withAlpha(0.3f));
            enableButton.setColour(juce::TextButton::textColourOffId, theme->getTextMuted());
            enableButton.setColour(juce::TextButton::textColourOnId, theme->getAccentSecondary());
        }
        
        repaint();
    }
    
    // Getters para attachments
    juce::Slider& getKnob(size_t index)
    {
        jassert(index < knobs.size());
        return *knobs[index];
    }
    
    juce::Button& getEnableButton() { return enableButton; }
    
    size_t getNumKnobs() const { return knobs.size(); }
    
    void paint(juce::Graphics& g) override
    {
        if (!theme) return;
        
        auto bounds = getLocalBounds().toFloat();
        bool on = enableButton.getToggleState();
        
        // Background
        g.setColour(theme->getPanelBackground().withAlpha(on ? 0.9f : 0.5f));
        g.fillRoundedRectangle(bounds, 4.0f);
        
        // Border
        g.setColour(on ? theme->getAccentSecondary().withAlpha(0.4f) : theme->getPanelBorder());
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
        
        // Title
        g.setColour(on ? theme->getTextPrimary() : theme->getTextMuted());
        g.setFont(theme->getLabelFont());
        g.drawText(effectName, 8, 6, 80, 16, juce::Justification::left);
        
        // Active indicator
        if (on)
        {
            g.setColour(theme->getAccentSecondary());
            g.fillRoundedRectangle(bounds.getWidth() - 8.0f, 8.0f, 4.0f, 12.0f, 2.0f);
        }
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        
        // Header area
        auto header = bounds.removeFromTop(26);
        header.removeFromLeft(80);  // Space for title
        enableButton.setBounds(header.removeFromRight(40).reduced(4, 4));
        
        // Knobs area
        auto content = bounds.reduced(4, 2);
        
        if (knobs.empty()) return;
        
        int knobSize = 36;
        int knobHeight = knobSize + 14;
        int knobGap = 4;
        
        int totalWidth = static_cast<int>(knobs.size()) * knobSize + (static_cast<int>(knobs.size()) - 1) * knobGap;
        int startX = content.getX() + (content.getWidth() - totalWidth) / 2;
        int knobY = content.getY() + (content.getHeight() - knobHeight) / 2;
        
        for (size_t i = 0; i < knobs.size(); ++i)
        {
            knobs[i]->setBounds(startX + static_cast<int>(i) * (knobSize + knobGap), knobY, knobSize, knobHeight);
        }
    }
    
private:
    const Theme* theme = nullptr;
    juce::String effectName;
    juce::TextButton enableButton;
    std::vector<std::unique_ptr<KndlKnob>> knobs;
};

} // namespace kndl::ui
