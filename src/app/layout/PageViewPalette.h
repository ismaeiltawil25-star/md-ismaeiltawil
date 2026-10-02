#pragma once

// PageViewPalette — colours for the page view, derived from the reader's page
// background so it works for ANY theme (day, night, or custom colours).
//
//   page   : the reader page background, unchanged (text colours are untouched)
//   desk   : the surface around and between pages
//   border : a thin outline around each page
//   shadow : a soft drop shadow under each page
//
// Dark pages get a LIGHTER desk and light pages a DARKER desk, and the desk is
// pushed further from the page colour until the WCAG contrast ratio between
// them reaches kMinDeskContrast. Pages therefore stay clearly separated in
// night mode, where "slightly darker than near-black" would be invisible.

#include <QColor>

namespace mdbr::layout {

struct PageViewPalette
{
    static constexpr qreal kMinDeskContrast = 1.25;

    QColor page;
    QColor desk;
    QColor border;
    QColor shadow;

    [[nodiscard]] static PageViewPalette fromPageBackground(const QColor &pageBackground);
};

// WCAG 2.x contrast ratio between two opaque colours (1.0 = identical, 21 = black/white).
[[nodiscard]] qreal contrastRatio(const QColor &a, const QColor &b);

} // namespace mdbr::layout
