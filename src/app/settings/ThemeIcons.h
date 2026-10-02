#pragma once

// ThemeIcons — the night-mode toolbar icon (a crescent moon), drawn with
// QPainter like the print icons: no image files, sharp at any DPI, coloured
// from the current palette.

#include <QColor>
#include <QIcon>

namespace mdbr::settings {

[[nodiscard]] QIcon nightModeIcon(const QColor &color);

} // namespace mdbr::settings
