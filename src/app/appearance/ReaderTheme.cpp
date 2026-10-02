#include "ReaderTheme.h"

#include "ReaderAppearanceDefaults.h"

#include <QtGlobal>

#include <cmath>

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

struct Rgb
{
    double r = 0, g = 0, b = 0; // 0..255
};

[[nodiscard]] Rgb parseHex(const QString &hex)
{
    bool ok = false;
    const uint v = hex.size() == 7 && hex.startsWith(u'#') ? hex.mid(1).toUInt(&ok, 16) : 0;
    if (!ok)
        return {};
    return {double((v >> 16) & 0xFF), double((v >> 8) & 0xFF), double(v & 0xFF)};
}

[[nodiscard]] QString toHex(const Rgb &c)
{
    const auto clamp = [](double v) { return qBound(0, qRound(v), 255); };
    return QStringLiteral("#%1%2%3")
        .arg(clamp(c.r), 2, 16, QLatin1Char('0'))
        .arg(clamp(c.g), 2, 16, QLatin1Char('0'))
        .arg(clamp(c.b), 2, 16, QLatin1Char('0'));
}

// `amount` of the way from `from` to `to`.
[[nodiscard]] QString mixHex(const QString &from, const QString &to, double amount)
{
    const Rgb a = parseHex(from), b = parseHex(to);
    return toHex({a.r + (b.r - a.r) * amount, a.g + (b.g - a.g) * amount, a.b + (b.b - a.b) * amount});
}

[[nodiscard]] double channel(double c8)
{
    const double c = c8 / 255.0;
    return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
}

[[nodiscard]] double luminance(const QString &hex)
{
    const Rgb c = parseHex(hex);
    return 0.2126 * channel(c.r) + 0.7152 * channel(c.g) + 0.0722 * channel(c.b);
}

} // namespace

ReaderAppearanceSettings deriveCustomPalette(const QString &background, const QString &text,
                                             const QString &link)
{
    ReaderAppearanceSettings s;
    s.customEnabled = true;
    s.pageBackground = background.toLower();
    s.mainText = text.toLower();
    s.linkText = link.toLower();
    s.headingText = s.mainText;
    s.mutedText = mixHex(text, background, 0.35);
    s.inlineCodeText = s.mainText;
    s.inlineCodeBackground = mixHex(background, text, 0.10);
    s.codeBlockText = s.mainText;
    s.codeBlockBackground = mixHex(background, text, 0.06);
    s.blockquoteText = mixHex(text, background, 0.25);
    s.blockquoteBorder = mixHex(background, text, 0.30);
    s.horizontalRule = mixHex(background, text, 0.20);
    return s;
}

double colorContrast(const QString &a, const QString &b)
{
    const double la = luminance(a), lb = luminance(b);
    return (qMax(la, lb) + 0.05) / (qMin(la, lb) + 0.05);
}

bool hasReadableText(const ReaderAppearanceSettings &palette)
{
    return colorContrast(palette.pageBackground, palette.mainText) >= 4.5;
}

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
