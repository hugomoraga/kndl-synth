#pragma once

#include <JuceHeader.h>
#include <vector>
#include <initializer_list>

namespace kndl::ui {

/**
 * Size unit for Grid rows.
 * - px(n): fixed pixel size
 * - fr(n): fractional/proportional weight (like CSS Grid's fr unit)
 */
struct Size
{
    enum Type { Fixed, Fraction };
    Type type;
    int value;
};

inline Size px(int v) { return { Size::Fixed, v }; }
inline Size fr(int v) { return { Size::Fraction, v }; }

/**
 * Grid - Declarative row/col layout system inspired by Bootstrap and CSS Grid.
 *
 * Two core operations:
 *   cols({spans...})  - horizontal split using a 12-column grid
 *   rows({sizes...})  - vertical split using px() and fr() units
 *
 * Nesting via sub(index) to get a child Grid from any cell.
 *
 * Usage:
 *   auto g = Grid(bounds, 6);
 *   auto main = g.rows({px(50), fr(65), fr(35)});
 *   auto mid = main.sub(1).cols({3, 4, 5});
 *   osc1Section.setBounds(mid[0]);
 *   filterPanel.setBounds(mid[1]);
 */
class KndlGrid
{
public:
    /**
     * Create a KndlGrid from a bounding rectangle and gap size.
     */
    KndlGrid(juce::Rectangle<int> area, int gapSize = 6)
        : bounds(area), gridGap(gapSize)
    {
        cells.push_back(area);
    }

    /**
     * Split horizontally into columns using a 12-based grid.
     * Spans must sum to 12 (or any total - they're proportional).
     * Example: cols({3, 5, 4}) = 25%, 42%, 33%
     */
    KndlGrid cols(std::initializer_list<int> spans) const
    {
        KndlGrid result(bounds, gridGap);
        result.cells.clear();

        int totalSpan = 0;
        for (int s : spans)
            totalSpan += s;

        if (totalSpan <= 0) return result;

        int numGaps = static_cast<int>(spans.size()) - 1;
        int totalGapWidth = numGaps * gridGap;
        int availableWidth = bounds.getWidth() - totalGapWidth;

        int x = bounds.getX();
        int idx = 0;
        int numSpans = static_cast<int>(spans.size());

        for (auto it = spans.begin(); it != spans.end(); ++it, ++idx)
        {
            int spanWidth;
            if (idx == numSpans - 1)
            {
                // Last column takes remaining space to avoid rounding errors
                spanWidth = bounds.getRight() - x;
            }
            else
            {
                spanWidth = availableWidth * (*it) / totalSpan;
            }

            result.cells.push_back(juce::Rectangle<int>(
                x, bounds.getY(), spanWidth, bounds.getHeight()));

            x += spanWidth + gridGap;
        }

        return result;
    }

    /**
     * Split vertically using a mix of px() (fixed) and fr() (proportional) sizes.
     * Example: rows({px(50), fr(65), fr(35)})
     */
    KndlGrid rows(std::initializer_list<Size> sizes) const
    {
        KndlGrid result(bounds, gridGap);
        result.cells.clear();

        int numGaps = static_cast<int>(sizes.size()) - 1;
        int totalGapHeight = numGaps * gridGap;

        // First pass: calculate fixed space and total fraction weight
        int fixedTotal = 0;
        int fractionTotal = 0;

        for (const auto& s : sizes)
        {
            if (s.type == Size::Fixed)
                fixedTotal += s.value;
            else
                fractionTotal += s.value;
        }

        int availableForFraction = bounds.getHeight() - totalGapHeight - fixedTotal;
        if (availableForFraction < 0) availableForFraction = 0;

        // Second pass: build cells
        int y = bounds.getY();
        int idx = 0;
        int numSizes = static_cast<int>(sizes.size());

        for (auto it = sizes.begin(); it != sizes.end(); ++it, ++idx)
        {
            int rowHeight;

            if (idx == numSizes - 1)
            {
                // Last row takes remaining space
                rowHeight = bounds.getBottom() - y;
            }
            else if (it->type == Size::Fixed)
            {
                rowHeight = it->value;
            }
            else
            {
                rowHeight = (fractionTotal > 0)
                    ? availableForFraction * it->value / fractionTotal
                    : 0;
            }

            result.cells.push_back(juce::Rectangle<int>(
                bounds.getX(), y, bounds.getWidth(), rowHeight));

            y += rowHeight + gridGap;
        }

        return result;
    }

    /**
     * Split into N equal columns.
     */
    KndlGrid equalCols(int n) const
    {
        std::vector<int> spans(static_cast<size_t>(n), 1);
        KndlGrid result(bounds, gridGap);
        result.cells.clear();

        int numGaps = n - 1;
        int totalGapWidth = numGaps * gridGap;
        int availableWidth = bounds.getWidth() - totalGapWidth;

        int x = bounds.getX();
        for (int i = 0; i < n; ++i)
        {
            int colWidth = (i == n - 1)
                ? bounds.getRight() - x
                : availableWidth / n;

            result.cells.push_back(juce::Rectangle<int>(
                x, bounds.getY(), colWidth, bounds.getHeight()));

            x += colWidth + gridGap;
        }

        return result;
    }

    /**
     * Split into N equal rows.
     */
    KndlGrid equalRows(int n) const
    {
        KndlGrid result(bounds, gridGap);
        result.cells.clear();

        int numGaps = n - 1;
        int totalGapHeight = numGaps * gridGap;
        int availableHeight = bounds.getHeight() - totalGapHeight;

        int y = bounds.getY();
        for (int i = 0; i < n; ++i)
        {
            int rowHeight = (i == n - 1)
                ? bounds.getBottom() - y
                : availableHeight / n;

            result.cells.push_back(juce::Rectangle<int>(
                bounds.getX(), y, bounds.getWidth(), rowHeight));

            y += rowHeight + gridGap;
        }

        return result;
    }

    /**
     * Access a cell by index.
     */
    juce::Rectangle<int> operator[](int i) const
    {
        if (i >= 0 && i < static_cast<int>(cells.size()))
            return cells[static_cast<size_t>(i)];
        return {};
    }

    /**
     * Get a child Grid from a cell, for nesting.
     * Optionally override the gap size.
     */
    KndlGrid sub(int i, int newGap = -1) const
    {
        auto cellBounds = (*this)[i];
        return KndlGrid(cellBounds, newGap >= 0 ? newGap : gridGap);
    }

    /**
     * Number of cells in this Grid.
     */
    int count() const
    {
        return static_cast<int>(cells.size());
    }

    /**
     * Get the raw bounds of this Grid.
     */
    juce::Rectangle<int> getBounds() const { return bounds; }

private:
    juce::Rectangle<int> bounds;
    std::vector<juce::Rectangle<int>> cells;
    int gridGap;
};

} // namespace kndl::ui
