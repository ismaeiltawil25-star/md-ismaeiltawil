#pragma once

// PrintIcons — toolbar icons drawn with QPainter, so they need no image files,
// stay sharp at any DPI, and take their colour from the current palette
// (readable on light and dark system themes).

#include <QColor>
#include <QIcon>

namespace mdbr::print {

[[nodiscard]] QIcon printIcon(const QColor &color);
[[nodiscard]] QIcon printPreviewIcon(const QColor &color);

} // namespace mdbr::print
