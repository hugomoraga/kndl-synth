#pragma once

#include <JuceHeader.h>
#include <FontData.h>

namespace kndl::ui {

/**
 * FontManager - Carga y cachea las fuentes embebidas (Inconsolata).
 * Singleton para asegurar que los Typefaces se crean una sola vez.
 */
class FontManager
{
public:
    static FontManager& getInstance()
    {
        static FontManager instance;
        return instance;
    }
    
    /** Get Inconsolata Regular at specified size */
    juce::Font getRegular(float size) const
    {
        if (regularTypeface)
            return juce::Font(juce::FontOptions(regularTypeface).withHeight(size));
        return juce::Font(juce::FontOptions(size));
    }
    
    /** Get Inconsolata Bold at specified size */
    juce::Font getBold(float size) const
    {
        if (boldTypeface)
            return juce::Font(juce::FontOptions(boldTypeface).withHeight(size));
        return juce::Font(juce::FontOptions(size, juce::Font::bold));
    }
    
    bool isLoaded() const { return regularTypeface != nullptr && boldTypeface != nullptr; }

private:
    FontManager()
    {
        // Load Inconsolata Regular from binary data
        regularTypeface = juce::Typeface::createSystemTypefaceFor(
            FontData::InconsolataRegular_ttf,
            FontData::InconsolataRegular_ttfSize);
        
        // Load Inconsolata Bold from binary data
        boldTypeface = juce::Typeface::createSystemTypefaceFor(
            FontData::InconsolataBold_ttf,
            FontData::InconsolataBold_ttfSize);
    }
    
    juce::Typeface::Ptr regularTypeface;
    juce::Typeface::Ptr boldTypeface;
    
    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;
};

} // namespace kndl::ui
