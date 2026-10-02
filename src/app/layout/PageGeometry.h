#pragma once

// PageGeometry — converts page settings (millimetres) into on-screen pixels.
//
// The page is drawn at its real physical size for the screen's logical DPI
// (A4 at 96 DPI = 794 × 1123 px), so line and page breaks match print output.
// Zooming changes the text size only; the page stays physically A4, exactly as
// in a word processor's print layout.
//
// Gaps between pages are drawn INSIDE the top/bottom margins of adjacent pages
// (half a gap from each). Text never enters the margins, so it can never be
// painted over a gap. kPageGapPx must therefore stay below twice the smallest margin.

#include "PageLayoutSettings.h"

#include <QSizeF>

namespace mdbr::layout {

struct PageGeometry
{
    static constexpr qreal kPageGapPx = 20.0; // visible gap between two pages

    QSizeF pageSize; // full page including margins, px
    qreal margin = 0; // all four sides, px

    [[nodiscard]] qreal textWidth() const { return pageSize.width() - 2 * margin; }

    // Page i's visible sheet in document coordinates (gap halves removed).
    [[nodiscard]] qreal sheetTop(int pageIndex) const
    {
        return pageIndex * pageSize.height() + kPageGapPx / 2;
    }
    [[nodiscard]] qreal sheetHeight() const { return pageSize.height() - kPageGapPx; }
};

[[nodiscard]] PageGeometry computePageGeometry(const PageLayoutSettings &settings, qreal logicalDpi);

[[nodiscard]] constexpr qreal mmToPx(qreal mm, qreal logicalDpi)
{
    return mm * logicalDpi / 25.4;
}

} // namespace mdbr::layout
