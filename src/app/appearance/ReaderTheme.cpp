#include "ReaderTheme.h"

#include "ReaderAppearanceDefaults.h"

namespace mdbr::appearance {

namespace {

// Compares the 12 colour roles only (customEnabled is a legacy flag).
[[nodiscard]] bool sameColors(ReaderAppearanceSettings a, ReaderAppearanceSettings b)
{
    a.customEnabled = b.customEnabled = false;
    // Hex case may differ between stored values and presets.
    const auto lower = [](ReaderAppearanceSettings &s) {
        for (QString *c : {&s.pageBackground, &s.mainText, &s.headingText, &s.mutedText, &s.linkText,
                           &s.inlineCodeText, &s.inlineCodeBackground, &s.codeBlockText,
                           &s.codeBlockBackground, &s.blockquoteText, &s.blockquoteBorder,
                           &s.horizontalRule})
            *c = c->toLower();
    };
    lower(a);
    lower(b);
    return a == b;
}

} // namespace

ReaderAppearanceSettings resolvePalette(ThemeMode mode, const ReaderAppearanceSettings &customColors,
                                        bool systemIsDark)
{
    switch (mode) {
    case ThemeMode::System:
        return systemIsDark ? ReaderAppearanceDefaults::nightPalette() : ReaderAppearanceDefaults::dayPalette();
    case ThemeMode::Day:
        return ReaderAppearanceDefaults::dayPalette();
    case ThemeMode::Night:
        return ReaderAppearanceDefaults::nightPalette();
    case ThemeMode::Custom:
        return customColors;
    }
    return ReaderAppearanceDefaults::nightPalette();
}

bool isDarkPalette(const ReaderAppearanceSettings &palette)
{
    const QString hex = palette.pageBackground;
    if (hex.size() != 7 || !hex.startsWith(u'#'))
        return false;
    bool ok = false;
    const uint rgb = hex.mid(1).toUInt(&ok, 16);
    if (!ok)
        return false;
    const double luminance = 0.2126 * ((rgb >> 16) & 0xFF) + 0.7152 * ((rgb >> 8) & 0xFF) + 0.0722 * (rgb & 0xFF);
    return luminance < 128.0;
}

ThemeMode modeForLegacyColors(const ReaderAppearanceSettings &storedColors)
{
    if (sameColors(storedColors, ReaderAppearanceDefaults::nightPalette()))
        return ThemeMode::Night;
    if (sameColors(storedColors, ReaderAppearanceDefaults::dayPalette()))
        return ThemeMode::Day;
    return ThemeMode::Custom;
}

QString themeModeId(ThemeMode mode)
{
    switch (mode) {
    case ThemeMode::System: return QStringLiteral("system");
    case ThemeMode::Day:    return QStringLiteral("day");
    case ThemeMode::Night:  return QStringLiteral("night");
    case ThemeMode::Custom: return QStringLiteral("custom");
    }
    return QStringLiteral("system");
}

ThemeMode themeModeFromId(const QString &id, ThemeMode fallback)
{
    for (const ThemeMode m : {ThemeMode::System, ThemeMode::Day, ThemeMode::Night, ThemeMode::Custom})
        if (themeModeId(m).compare(id, Qt::CaseInsensitive) == 0)
            return m;
    return fallback;
}

QString themeModeLabel(ThemeMode mode)
{
    switch (mode) {
    case ThemeMode::System: return QStringLiteral("Follow Windows");
    case ThemeMode::Day:    return QStringLiteral("Day");
    case ThemeMode::Night:  return QStringLiteral("Night");
    case ThemeMode::Custom: return QStringLiteral("Custom colors");
    }
    return QString();
}

} // namespace mdbr::appearance
