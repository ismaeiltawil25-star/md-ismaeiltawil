#pragma once

// ThemeIcons — icons drawn with QPainter (no image files, sharp at any DPI):
//   * the night-mode toolbar toggle (a crescent moon), coloured from the palette;
//   * theme card previews for Preferences > Appearance.

#include "app/appearance/ReaderAppearanceSettings.h"

#include <QColor>
#include <QIcon>

namespace mdbr::settings {

[[nodiscard]] QIcon nightModeIcon(const QColor &color);

// A theme card preview: a little page in the palette's background colour with
// "Aa" in its text colour and a link-coloured line. With `secondHalf`, the
// right half shows a second palette (used for "Follow Windows": day | night).
[[nodiscard]] QIcon themePreviewIcon(const ReaderAppearanceSettings &palette,
                                     const ReaderAppearanceSettings *secondHalf = nullptr);

} // namespace mdbr::settings
