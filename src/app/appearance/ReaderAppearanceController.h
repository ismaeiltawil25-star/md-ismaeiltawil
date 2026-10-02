#ifndef READERAPPEARANCECONTROLLER_H
#define READERAPPEARANCECONTROLLER_H

#include <QObject>
#include "ReaderAppearanceSettings.h"
#include "ReaderTheme.h"

class ReaderAppearanceController : public QObject
{
    Q_OBJECT

public:
    explicit ReaderAppearanceController(QObject *parent = nullptr);

    void load();
    // The user's custom colours (used in ThemeMode::Custom).
    const ReaderAppearanceSettings &settings() const;

    // What the reader shows right now: mode + custom colours + Windows theme.
    ReaderAppearanceSettings effectiveSettings() const;
    mdbr::appearance::ThemeMode themeMode() const { return m_mode; }
    bool systemIsDark() const { return m_systemDark; }

public slots:
    void applySettings(const ReaderAppearanceSettings &newSettings);
    void restoreDefaults();
    void clearStoredSettings();
    void setThemeMode(mdbr::appearance::ThemeMode mode);
    // Called by the window when Windows switches light/dark.
    void setSystemIsDark(bool dark);

signals:
    void appearanceChanged(const ReaderAppearanceSettings &settings);
    void themeModeChanged(mdbr::appearance::ThemeMode mode);

private:
    ReaderAppearanceSettings m_settings;
    mdbr::appearance::ThemeMode m_mode = mdbr::appearance::ThemeMode::System;
    bool m_systemDark = true;
};

#endif
