#pragma once

// ReaderTheme — which colours the reader shows.
//
//   System : follow Windows (dark mode -> Night palette, light -> Day palette),
//            switching live when the Windows setting changes
//   Day    : always the light palette
//   Night  : always the dark palette
//   Custom : the user's own 12 colours (edited in Preferences > Appearance)
//
// The custom colours are stored as ReaderAppearanceSettings (unchanged format);
// the mode is stored next to them. resolvePalette() is the single place that
// decides what is on screen, so menus, toolbar and Preferences always agree.

#include "ReaderAppearanceSettings.h"

#include <QString>

namespace mdbr::appearance {

enum class ThemeMode
{
    System,
    Day,
    Night,
    Custom,
};

// The colours to display for `mode`. `systemIsDark` matters only for System.
[[nodiscard]] ReaderAppearanceSettings resolvePalette(ThemeMode mode,
                                                      const ReaderAppearanceSettings &customColors,
                                                      bool systemIsDark);

// True when the resolved page background is dark (used by the night toggle).
[[nodiscard]] bool isDarkPalette(const ReaderAppearanceSettings &palette);

// For settings written before theme modes existed: Day/Night if the stored
// colours equal a preset, otherwise Custom.
[[nodiscard]] ThemeMode modeForLegacyColors(const ReaderAppearanceSettings &storedColors);

// Simple customisation: the user picks only background, text and link colours;
// the other 9 roles (headings, muted text, code, quotes, rules) are derived
// from them so the page stays consistent. All inputs are "#RRGGBB".
[[nodiscard]] ReaderAppearanceSettings deriveCustomPalette(const QString &background,
                                                           const QString &text,
                                                           const QString &link);

// WCAG contrast ratio of two "#RRGGBB" colours (1 = identical, 21 = max).
[[nodiscard]] double colorContrast(const QString &a, const QString &b);

// Body text is comfortably readable (WCAG AA: 4.5 : 1).
[[nodiscard]] bool hasReadableText(const ReaderAppearanceSettings &palette);

[[nodiscard]] QString themeModeId(ThemeMode mode);  // "system", "day", ...
[[nodiscard]] ThemeMode themeModeFromId(const QString &id, ThemeMode fallback = ThemeMode::System);
[[nodiscard]] QString themeModeLabel(ThemeMode mode); // for menus and Preferences

} // namespace mdbr::appearance
