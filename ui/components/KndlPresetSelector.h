#pragma once

#include <JuceHeader.h>
#include "../skins/Theme.h"
#include "../../plugin/PresetManager.h"

namespace kndl::ui {

/**
 * KndlPresetSelector - Componente para seleccionar y gestionar presets.
 */
class KndlPresetSelector : public juce::Component
{
public:
    KndlPresetSelector()
    {
        // Botón anterior
        addAndMakeVisible(prevButton);
        prevButton.setButtonText("<");
        prevButton.onClick = [this]() {
            if (presetManager) {
                presetManager->previousPreset();
                updateDisplay();
            }
        };
        
        // Botón siguiente
        addAndMakeVisible(nextButton);
        nextButton.setButtonText(">");
        nextButton.onClick = [this]() {
            if (presetManager) {
                presetManager->nextPreset();
                updateDisplay();
            }
        };
        
        // Nombre del preset (clickeable para abrir menú)
        addAndMakeVisible(presetNameLabel);
        presetNameLabel.setJustificationType(juce::Justification::centred);
        presetNameLabel.setInterceptsMouseClicks(true, false);
        
        // Botón guardar
        addAndMakeVisible(saveButton);
        saveButton.setButtonText("SAVE");
        saveButton.onClick = [this]() { showSaveDialog(); };
        
        // Botón menú
        addAndMakeVisible(menuButton);
        menuButton.setButtonText("...");
        menuButton.onClick = [this]() { showPresetMenu(); };
    }
    
    void setPresetManager(kndl::PresetManager* manager)
    {
        presetManager = manager;
        updateDisplay();
    }
    
    void setTheme(const Theme* newTheme)
    {
        theme = newTheme;
        
        if (theme)
        {
            presetNameLabel.setColour(juce::Label::textColourId, theme->getTextPrimary());
            presetNameLabel.setFont(theme->getLabelFont().withHeight(14.0f));
            
            auto buttonStyle = [this](juce::TextButton& btn) {
                btn.setColour(juce::TextButton::buttonColourId, theme->getPanelBackground());
                btn.setColour(juce::TextButton::textColourOffId, theme->getTextSecondary());
                btn.setColour(juce::TextButton::textColourOnId, theme->getAccentPrimary());
            };
            
            buttonStyle(prevButton);
            buttonStyle(nextButton);
            buttonStyle(saveButton);
            buttonStyle(menuButton);
        }
        
        repaint();
    }
    
    void updateDisplay()
    {
        if (presetManager)
        {
            presetNameLabel.setText(presetManager->getCurrentPresetName(), juce::dontSendNotification);
            
            int index = presetManager->getCurrentPresetIndex();
            int count = presetManager->getPresetCount();
            
            if (count > 0 && index >= 0)
            {
                presetNameLabel.setText(
                    juce::String(index + 1) + "/" + juce::String(count) + " " + presetManager->getCurrentPresetName(),
                    juce::dontSendNotification
                );
            }
        }
        repaint();
    }
    
    void paint(juce::Graphics& g) override
    {
        if (!theme) return;
        
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(theme->getPanelBackground());
        g.fillRoundedRectangle(bounds, 4.0f);
        
        // Border
        g.setColour(theme->getPanelBorder());
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds().reduced(4, 2);
        
        int buttonWidth = 28;
        int saveWidth = 45;
        int menuWidth = 28;
        
        prevButton.setBounds(bounds.removeFromLeft(buttonWidth));
        bounds.removeFromLeft(4);
        
        menuButton.setBounds(bounds.removeFromRight(menuWidth));
        bounds.removeFromRight(4);
        
        saveButton.setBounds(bounds.removeFromRight(saveWidth));
        bounds.removeFromRight(4);
        
        nextButton.setBounds(bounds.removeFromRight(buttonWidth));
        bounds.removeFromRight(4);
        
        presetNameLabel.setBounds(bounds);
    }
    
    void mouseDown(const juce::MouseEvent& event) override
    {
        if (presetNameLabel.getBounds().contains(event.getPosition()))
        {
            showPresetList();
        }
    }
    
private:
    void showSaveDialog()
    {
        if (!presetManager) return;
        
        auto* alertWindow = new juce::AlertWindow("Save Preset", "Enter preset name:", juce::MessageBoxIconType::NoIcon);
        alertWindow->addTextEditor("name", presetManager->getCurrentPresetName(), "Name:");
        alertWindow->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
        alertWindow->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        
        alertWindow->enterModalState(true, juce::ModalCallbackFunction::create(
            [this, alertWindow](int result) {
                if (result == 1)
                {
                    auto name = alertWindow->getTextEditorContents("name");
                    if (presetManager && name.isNotEmpty())
                    {
                        presetManager->savePreset(name);
                        updateDisplay();
                    }
                }
                delete alertWindow;
            }
        ));
    }
    
    void showPresetMenu()
    {
        if (!presetManager) return;
        
        juce::PopupMenu menu;
        
        menu.addItem(1, "Init (Reset)", true, false);
        menu.addSeparator();
        menu.addItem(2, "Save As...", true, false);
        menu.addItem(3, "Delete Current", presetManager->getCurrentPresetIndex() >= 0, false);
        menu.addSeparator();
        menu.addItem(4, "Open Presets Folder", true, false);
        menu.addItem(5, "Import Preset...", true, false);
        menu.addItem(6, "Export Current...", presetManager->getCurrentPresetIndex() >= 0, false);
        
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&menuButton),
            [this](int result) {
                handleMenuResult(result);
            }
        );
    }
    
    void showPresetList()
    {
        if (!presetManager) return;
        
        juce::PopupMenu menu;
        
        const auto& presets = presetManager->getPresetList();
        int currentIndex = presetManager->getCurrentPresetIndex();
        
        if (presets.empty())
        {
            menu.addItem(-1, "(No presets)", false, false);
        }
        else
        {
            for (size_t i = 0; i < presets.size(); ++i)
            {
                menu.addItem(static_cast<int>(i) + 100, presets[i], true, static_cast<int>(i) == currentIndex);
            }
        }
        
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&presetNameLabel),
            [this](int result) {
                if (result >= 100 && presetManager)
                {
                    presetManager->loadPresetByIndex(result - 100);
                    updateDisplay();
                }
            }
        );
    }
    
    void handleMenuResult(int result)
    {
        if (!presetManager) return;
        
        switch (result)
        {
            case 1: // Init
                presetManager->initPreset();
                updateDisplay();
                break;
                
            case 2: // Save As
                showSaveDialog();
                break;
                
            case 3: // Delete
            {
                auto presetName = presetManager->getCurrentPresetName();
                juce::AlertWindow::showOkCancelBox(
                    juce::MessageBoxIconType::WarningIcon,
                    "Delete Preset",
                    "Delete preset \"" + presetName + "\"?",
                    "Delete",
                    "Cancel",
                    nullptr,
                    juce::ModalCallbackFunction::create([this, presetName](int res) {
                        if (res == 1 && presetManager)
                        {
                            presetManager->deletePreset(presetName);
                            updateDisplay();
                        }
                    })
                );
                break;
            }
                
            case 4: // Open folder
                presetManager->getPresetDirectory().startAsProcess();
                break;
                
            case 5: // Import
            {
                fileChooser = std::make_unique<juce::FileChooser>(
                    "Import Preset", juce::File(), "*.kndl"
                );
                fileChooser->launchAsync(juce::FileBrowserComponent::openMode,
                    [this](const juce::FileChooser& fc) {
                        auto file = fc.getResult();
                        if (file.existsAsFile() && presetManager)
                        {
                            presetManager->importPreset(file);
                            updateDisplay();
                        }
                    }
                );
                break;
            }
                
            case 6: // Export
            {
                fileChooser = std::make_unique<juce::FileChooser>(
                    "Export Preset",
                    juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                        .getChildFile(presetManager->getCurrentPresetName() + ".kndl"),
                    "*.kndl"
                );
                fileChooser->launchAsync(juce::FileBrowserComponent::saveMode,
                    [this](const juce::FileChooser& fc) {
                        auto file = fc.getResult();
                        if (file != juce::File() && presetManager)
                        {
                            presetManager->exportPreset(presetManager->getCurrentPresetName(), file);
                        }
                    }
                );
                break;
            }
        }
    }
    
    const Theme* theme = nullptr;
    kndl::PresetManager* presetManager = nullptr;
    
    juce::TextButton prevButton;
    juce::TextButton nextButton;
    juce::TextButton saveButton;
    juce::TextButton menuButton;
    juce::Label presetNameLabel;
    
    std::unique_ptr<juce::FileChooser> fileChooser;
};

} // namespace kndl::ui
