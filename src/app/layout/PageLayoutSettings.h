#pragma once

// PageLayoutSettings — how documents are laid out in the reader.
//
//   Continuous (default): text flows in one long column, as before.
//   Page view:            text is split into printed-size pages (A4 by default)
//                         with margins and visible gaps between pages, so what
//                         you see matches how the document would print.
//
// Plain data only. Geometry lives in PageGeometry, colours in PageViewPalette,
// persistence in PageLayoutRepository.

#include <QSizeF>
#include <QString>

namespace mdbr::layout {

enum class PaperSize
{
    A4,
    A5,
    Letter,
    Legal,
};

enum class PageMargins
{
    Narrow,   // 12.7 mm
    Moderate, // 19.1 mm
    Normal,   // 25.4 mm (Word default)
    Wide,     // 38.1 mm
};

struct PageLayoutSettings
{
    bool pageViewEnabled = false;
    PaperSize paper = PaperSize::A4;
    PageMargins margins = PageMargins::Normal;

    friend bool operator==(const PageLayoutSettings &, const PageLayoutSettings &) = default;
};

// Portrait paper size in millimetres.
[[nodiscard]] QSizeF paperSizeMm(PaperSize paper);
[[nodiscard]] qreal marginMm(PageMargins margins);

// Stable identifiers used for persistence ("A4", "normal", ...).
[[nodiscard]] QString paperId(PaperSize paper);
[[nodiscard]] QString marginsId(PageMargins margins);
[[nodiscard]] PaperSize paperFromId(const QString &id, PaperSize fallback = PaperSize::A4);
[[nodiscard]] PageMargins marginsFromId(const QString &id, PageMargins fallback = PageMargins::Normal);

// Human-readable labels for the settings dialog.
[[nodiscard]] QString paperLabel(PaperSize paper);
[[nodiscard]] QString marginsLabel(PageMargins margins);

} // namespace mdbr::layout
