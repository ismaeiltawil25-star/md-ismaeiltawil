#include "ReaderAppearanceController.h"
#include "ReaderAppearanceRepository.h"
#include "ReaderAppearanceDefaults.h"

ReaderAppearanceController::ReaderAppearanceController(QObject *parent)
    : QObject(parent)
{
    m_settings = ReaderAppearanceRepository::load();
    m_mode = ReaderAppearanceRepository::loadThemeMode();
}

void ReaderAppearanceController::load()
{
    auto loaded = ReaderAppearanceRepository::load();
    if (loaded != m_settings) {
        m_settings = loaded;
        emit appearanceChanged(m_settings);
    }
}

const ReaderAppearanceSettings &ReaderAppearanceController::settings() const
{
    return m_settings;
}

void ReaderAppearanceController::applySettings(const ReaderAppearanceSettings &newSettings)
{
    if (newSettings == m_settings)
        return;

    m_settings = newSettings;
    ReaderAppearanceRepository::save(m_settings);
    emit appearanceChanged(m_settings);
}

void ReaderAppearanceController::restoreDefaults()
{
    applySettings(ReaderAppearanceDefaults::nightPalette());
}

void ReaderAppearanceController::clearStoredSettings()
{
    ReaderAppearanceRepository::clear();
    auto defaults = ReaderAppearanceDefaults::nightPalette();
    if (m_settings != defaults) {
        m_settings = defaults;
        emit appearanceChanged(m_settings);
    }
}

ReaderAppearanceSettings ReaderAppearanceController::effectiveSettings() const
{
    return mdbr::appearance::resolvePalette(m_mode, m_settings, m_systemDark);
}

void ReaderAppearanceController::setThemeMode(mdbr::appearance::ThemeMode mode)
{
    if (mode == m_mode)
        return;
    const ReaderAppearanceSettings before = effectiveSettings();
    m_mode = mode;
    ReaderAppearanceRepository::saveThemeMode(mode);
    emit themeModeChanged(mode);
    if (effectiveSettings() != before)
        emit appearanceChanged(effectiveSettings());
}

void ReaderAppearanceController::setSystemIsDark(bool dark)
{
    if (dark == m_systemDark)
        return;
    const ReaderAppearanceSettings before = effectiveSettings();
    m_systemDark = dark;
    if (effectiveSettings() != before)
        emit appearanceChanged(effectiveSettings());
}
