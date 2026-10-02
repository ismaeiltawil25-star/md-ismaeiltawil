#include "ReaderAppearanceRepository.h"
#include "ReaderAppearanceDefaults.h"

#include <QSettings>
#include <QRegularExpression>

static const char *kGroup     = "readerAppearance";
static const char *kSchemaVer = "schemaVersion";
static const char *kCustomEn  = "customEnabled";

static const char *kColorKeys[] = {
    "colors/pageBackground",
    "colors/mainText",
    "colors/headingText",
    "colors/mutedText",
    "colors/linkText",
    "colors/inlineCodeText",
    "colors/inlineCodeBackground",
    "colors/codeBlockText",
    "colors/codeBlockBackground",
    "colors/blockquoteText",
    "colors/blockquoteBorder",
    "colors/horizontalRule",
};

static constexpr int ColorCount = sizeof(kColorKeys) / sizeof(kColorKeys[0]);

static QString defaultColorForIndex(int i)
{
    auto def = ReaderAppearanceDefaults::nightPalette();
    switch (i) {
        case 0:  return def.pageBackground;
        case 1:  return def.mainText;
        case 2:  return def.headingText;
        case 3:  return def.mutedText;
        case 4:  return def.linkText;
        case 5:  return def.inlineCodeText;
        case 6:  return def.inlineCodeBackground;
        case 7:  return def.codeBlockText;
        case 8:  return def.codeBlockBackground;
        case 9:  return def.blockquoteText;
        case 10: return def.blockquoteBorder;
        case 11: return def.horizontalRule;
        default: return QString();
    }
}

static bool isValidHex(const QString &color)
{
    static const QRegularExpression re(QStringLiteral("^#[0-9a-fA-F]{6}$"));
    return re.match(color).hasMatch();
}

ReaderAppearanceSettings ReaderAppearanceRepository::load()
{
    QSettings settings;
    settings.beginGroup(kGroup);

    int ver = settings.value(kSchemaVer, 0).toInt();
    if (ver != SchemaVersion) {
        settings.endGroup();
        return ReaderAppearanceDefaults::nightPalette();
    }

    ReaderAppearanceSettings s;
    s.customEnabled = settings.value(kCustomEn, false).toBool();

    auto def = ReaderAppearanceDefaults::nightPalette();
    QString *colorPtrs[ColorCount] = {
        &s.pageBackground, &s.mainText, &s.headingText, &s.mutedText,
        &s.linkText, &s.inlineCodeText, &s.inlineCodeBackground,
        &s.codeBlockText, &s.codeBlockBackground,
        &s.blockquoteText, &s.blockquoteBorder, &s.horizontalRule,
    };

    for (int i = 0; i < ColorCount; ++i) {
        QString val = settings.value(kColorKeys[i]).toString();
        *colorPtrs[i] = isValidHex(val) ? val : defaultColorForIndex(i);
    }

    settings.endGroup();
    return s;
}

bool ReaderAppearanceRepository::save(const ReaderAppearanceSettings &s)
{
    QSettings settings;
    settings.beginGroup(kGroup);

    settings.setValue(kSchemaVer, SchemaVersion);
    settings.setValue(kCustomEn, s.customEnabled);

    const QString *colorPtrs[ColorCount] = {
        &s.pageBackground, &s.mainText, &s.headingText, &s.mutedText,
        &s.linkText, &s.inlineCodeText, &s.inlineCodeBackground,
        &s.codeBlockText, &s.codeBlockBackground,
        &s.blockquoteText, &s.blockquoteBorder, &s.horizontalRule,
    };

    for (int i = 0; i < ColorCount; ++i)
        settings.setValue(kColorKeys[i], *colorPtrs[i]);

    settings.endGroup();
    settings.sync();
    return settings.status() == QSettings::NoError;
}

void ReaderAppearanceRepository::clear()
{
    QSettings settings;
    settings.remove(kGroup);
    settings.sync();
}

mdbr::appearance::ThemeMode ReaderAppearanceRepository::loadThemeMode()
{
    using namespace mdbr::appearance;
    QSettings settings;
    settings.beginGroup(kGroup);
    const QString stored = settings.value(QStringLiteral("themeMode")).toString();
    const bool hasLegacyColors = settings.contains(kSchemaVer);
    settings.endGroup();

    if (!stored.isEmpty())
        return themeModeFromId(stored);
    return hasLegacyColors ? modeForLegacyColors(load()) : ThemeMode::System;
}

void ReaderAppearanceRepository::saveThemeMode(mdbr::appearance::ThemeMode mode)
{
    QSettings settings;
    settings.beginGroup(kGroup);
    settings.setValue(QStringLiteral("themeMode"), mdbr::appearance::themeModeId(mode));
    settings.endGroup();
    settings.sync();
}
