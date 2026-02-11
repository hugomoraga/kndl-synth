#pragma once

#include <JuceHeader.h>
#include "../skins/Theme.h"
#include "../../dsp/core/ModulationMatrix.h"
#include "../../dsp/core/Parameters.h"

namespace kndl::ui {

/**
 * KndlModMatrixRow - Una fila interactiva de la mod matrix.
 * [SRC] -> [DST] [======AMT======]
 */
class KndlModMatrixRow : public juce::Component
{
public:
    KndlModMatrixRow(int slotIndex)
        : slot(slotIndex)
    {
        // Source combo
        srcCombo.addItem("---", 1);
        srcCombo.addItem("LFO1", 2);
        srcCombo.addItem("LFO2", 3);
        srcCombo.addItem("AENV", 4);
        srcCombo.addItem("FENV", 5);
        srcCombo.addItem("VEL", 6);
        srcCombo.addItem("MOD", 7);
        srcCombo.addItem("AT", 8);
        srcCombo.addItem("SB.A", 9);
        srcCombo.addItem("SB.B", 10);
        srcCombo.addItem("SB.C", 11);
        srcCombo.addItem("SB.D", 12);
        srcCombo.setSelectedId(1, juce::dontSendNotification);
        srcCombo.setScrollWheelEnabled(false);
        addAndMakeVisible(srcCombo);
        
        // Destination combo
        dstCombo.addItem("---", 1);
        dstCombo.addItem("OSC1.P", 2);
        dstCombo.addItem("OSC2.P", 3);
        dstCombo.addItem("OSC1.L", 4);
        dstCombo.addItem("OSC2.L", 5);
        dstCombo.addItem("SUB.L", 6);
        dstCombo.addItem("FLT.C", 7);
        dstCombo.addItem("FLT.R", 8);
        dstCombo.addItem("AMP.L", 9);
        dstCombo.addItem("LFO1.R", 10);
        dstCombo.addItem("LFO2.R", 11);
        dstCombo.setSelectedId(1, juce::dontSendNotification);
        dstCombo.setScrollWheelEnabled(false);
        addAndMakeVisible(dstCombo);
        
        // Amount slider (horizontal, bipolar -1..+1)
        amtSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        amtSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 36, 14);
        amtSlider.setRange(-1.0, 1.0, 0.01);
        amtSlider.setValue(0.0, juce::dontSendNotification);
        amtSlider.setScrollWheelEnabled(true);
        addAndMakeVisible(amtSlider);
    }
    
    void setTheme(const Theme* newTheme)
    {
        theme = newTheme;
        if (!theme) return;
        
        auto bgCol = juce::Colour(0xff0d0d12);
        auto textCol = theme->getTextSecondary();
        auto accentCol = theme->getAccentPrimary();
        
        // Style combos
        for (auto* combo : { &srcCombo, &dstCombo })
        {
            combo->setColour(juce::ComboBox::backgroundColourId, bgCol);
            combo->setColour(juce::ComboBox::textColourId, textCol);
            combo->setColour(juce::ComboBox::outlineColourId, theme->getPanelBorder().withAlpha(0.4f));
            combo->setColour(juce::ComboBox::arrowColourId, theme->getTextMuted());
            combo->setColour(juce::ComboBox::focusedOutlineColourId, accentCol.withAlpha(0.5f));
        }
        
        // Style amount slider
        amtSlider.setColour(juce::Slider::backgroundColourId, bgCol);
        amtSlider.setColour(juce::Slider::trackColourId, accentCol.withAlpha(0.4f));
        amtSlider.setColour(juce::Slider::thumbColourId, accentCol);
        amtSlider.setColour(juce::Slider::textBoxTextColourId, theme->getTextMuted());
        amtSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        amtSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        
        if (theme)
        {
            srcCombo.setLookAndFeel(nullptr);
            dstCombo.setLookAndFeel(nullptr);
        }
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        int gap = 3;
        int totalW = bounds.getWidth();
        // Give combos ~30% each, slider gets the rest
        int comboW = juce::jmax(65, (totalW - gap * 2) * 30 / 100);
        
        srcCombo.setBounds(bounds.removeFromLeft(comboW));
        bounds.removeFromLeft(gap);
        dstCombo.setBounds(bounds.removeFromLeft(comboW));
        bounds.removeFromLeft(gap);
        amtSlider.setBounds(bounds);
    }
    
    juce::ComboBox& getSrcCombo() { return srcCombo; }
    juce::ComboBox& getDstCombo() { return dstCombo; }
    juce::Slider& getAmtSlider() { return amtSlider; }
    int getSlot() const { return slot; }
    
private:
    int slot;
    juce::ComboBox srcCombo;
    juce::ComboBox dstCombo;
    juce::Slider amtSlider;
    const Theme* theme = nullptr;
};

/**
 * KndlModMatrix - UI interactiva para la matriz de modulación.
 * Muestra 8 slots editables con source, destination, y amount.
 */
class KndlModMatrix : public juce::Component
{
public:
    KndlModMatrix(kndl::ModulationMatrix& matrix)
        : modMatrix(matrix)
    {
        for (int i = 0; i < kndl::ParamID::NUM_MOD_SLOTS; ++i)
        {
            rows.push_back(std::make_unique<KndlModMatrixRow>(i));
            addAndMakeVisible(*rows.back());
        }
    }
    
    void setTheme(const Theme* newTheme)
    {
        theme = newTheme;
        for (auto& row : rows)
            row->setTheme(newTheme);
        repaint();
    }
    
    /** Connect a row's ComboBoxes and Slider to APVTS parameters. */
    void connectToAPVTS(juce::AudioProcessorValueTreeState& apvts)
    {
        using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
        using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
        
        srcAttachments.clear();
        dstAttachments.clear();
        amtAttachments.clear();
        
        for (int i = 0; i < kndl::ParamID::NUM_MOD_SLOTS; ++i)
        {
            auto& row = *rows[static_cast<size_t>(i)];
            srcAttachments.push_back(std::make_unique<CA>(apvts, kndl::ParamID::MOD_SRC_IDS[i], row.getSrcCombo()));
            dstAttachments.push_back(std::make_unique<CA>(apvts, kndl::ParamID::MOD_DST_IDS[i], row.getDstCombo()));
            amtAttachments.push_back(std::make_unique<SA>(apvts, kndl::ParamID::MOD_AMT_IDS[i], row.getAmtSlider()));
        }
    }
    
    void paint(juce::Graphics& g) override
    {
        if (!theme) return;
        
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(juce::Colour(0xff0d0d12));
        g.fillRoundedRectangle(bounds, 4.0f);
        
        // Border
        g.setColour(theme->getPanelBorder().withAlpha(0.5f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
        
        // Title
        g.setColour(theme->getAccentTertiary());
        g.setFont(theme->getSmallFont());
        g.drawText("MOD.MATRIX", static_cast<int>(bounds.getX() + 6), static_cast<int>(bounds.getY() + 2), 90, 14, juce::Justification::left);
        
        // Column headers (match dynamic combo widths)
        auto headerY = static_cast<int>(bounds.getY() + 16);
        int innerW = static_cast<int>(bounds.getWidth()) - 12;
        int comboW = juce::jmax(65, (innerW - 6) * 30 / 100);
        g.setColour(theme->getTextMuted().withAlpha(0.6f));
        g.setFont(theme->getSmallFont());
        g.drawText("SRC", static_cast<int>(bounds.getX() + 6), headerY, comboW, 10, juce::Justification::left);
        g.drawText("DST", static_cast<int>(bounds.getX() + 6 + comboW + 3), headerY, comboW, 10, juce::Justification::left);
        g.drawText("AMT", static_cast<int>(bounds.getX() + 6 + (comboW + 3) * 2), headerY, 40, 10, juce::Justification::left);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds().reduced(6, 28);
        int rowHeight = 20;
        int rowGap = 2;
        
        for (auto& row : rows)
        {
            if (bounds.getHeight() < rowHeight) break;
            row->setBounds(bounds.removeFromTop(rowHeight));
            bounds.removeFromTop(rowGap);
        }
    }
    
private:
    [[maybe_unused]] kndl::ModulationMatrix& modMatrix;
    const Theme* theme = nullptr;
    
    std::vector<std::unique_ptr<KndlModMatrixRow>> rows;
    
    // APVTS attachments
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> srcAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> dstAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> amtAttachments;
};

} // namespace kndl::ui
