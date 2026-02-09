#pragma once

#include <JuceHeader.h>

namespace kndl::ui {

/**
 * LayoutSection - Define una sección del layout con su posición relativa.
 */
struct LayoutSection
{
    enum class Position
    {
        TopBar,
        Left,
        Center,
        Right,
        Bottom
    };
    
    Position position;
    float relativeWidth;   // 0.0 - 1.0 para Left/Center/Right
    float relativeHeight;  // 0.0 - 1.0 para TopBar/Bottom
    int minWidth = 100;
    int minHeight = 80;
};

/**
 * LayoutManager - Maneja el layout modular del synth.
 * Permite reorganizar secciones y adaptarse a diferentes tamaños.
 */
class LayoutManager
{
public:
    struct Bounds
    {
        juce::Rectangle<int> topBar;
        juce::Rectangle<int> left;
        juce::Rectangle<int> center;
        juce::Rectangle<int> right;
        juce::Rectangle<int> bottom;
    };
    
    void setMargins(int outer, int inner)
    {
        outerMargin = outer;
        innerMargin = inner;
    }
    
    void setTopBarHeight(float relative)
    {
        topBarRelativeHeight = juce::jlimit(0.05f, 0.2f, relative);
    }
    
    void setBottomHeight(float relative)
    {
        bottomRelativeHeight = juce::jlimit(0.1f, 0.4f, relative);
    }
    
    void setColumnWidths(float left, float center, float right)
    {
        float total = left + center + right;
        leftRelativeWidth = left / total;
        centerRelativeWidth = center / total;
        rightRelativeWidth = right / total;
    }
    
    Bounds calculate(juce::Rectangle<int> totalBounds) const
    {
        Bounds result;
        
        auto working = totalBounds.reduced(outerMargin);
        
        // Top bar
        int topBarHeight = static_cast<int>(working.getHeight() * topBarRelativeHeight);
        result.topBar = working.removeFromTop(topBarHeight);
        working.removeFromTop(innerMargin);
        
        // Bottom section
        int bottomHeight = static_cast<int>(working.getHeight() * bottomRelativeHeight);
        result.bottom = working.removeFromBottom(bottomHeight);
        working.removeFromBottom(innerMargin);
        
        // Middle sections (left, center, right)
        int middleWidth = working.getWidth();
        
        int leftWidth = static_cast<int>(middleWidth * leftRelativeWidth);
        result.left = working.removeFromLeft(leftWidth);
        working.removeFromLeft(innerMargin);
        
        int rightWidth = static_cast<int>(middleWidth * rightRelativeWidth);
        result.right = working.removeFromRight(rightWidth);
        working.removeFromRight(innerMargin);
        
        result.center = working;
        
        return result;
    }
    
    // Presets de layout
    static LayoutManager createStandard()
    {
        LayoutManager lm;
        lm.setMargins(12, 8);
        lm.setTopBarHeight(0.10f);
        lm.setBottomHeight(0.28f);
        lm.setColumnWidths(0.28f, 0.44f, 0.28f);
        return lm;
    }
    
    static LayoutManager createWide()
    {
        LayoutManager lm;
        lm.setMargins(10, 6);
        lm.setTopBarHeight(0.08f);
        lm.setBottomHeight(0.25f);
        lm.setColumnWidths(0.22f, 0.56f, 0.22f);
        return lm;
    }
    
    static LayoutManager createCompact()
    {
        LayoutManager lm;
        lm.setMargins(8, 4);
        lm.setTopBarHeight(0.12f);
        lm.setBottomHeight(0.30f);
        lm.setColumnWidths(0.33f, 0.34f, 0.33f);
        return lm;
    }
    
private:
    int outerMargin = 12;
    int innerMargin = 8;
    float topBarRelativeHeight = 0.10f;
    float bottomRelativeHeight = 0.28f;
    float leftRelativeWidth = 0.28f;
    float centerRelativeWidth = 0.44f;
    float rightRelativeWidth = 0.28f;
};

} // namespace kndl::ui
