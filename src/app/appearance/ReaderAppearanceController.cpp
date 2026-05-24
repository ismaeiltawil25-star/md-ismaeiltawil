#include "ReaderAppearanceController.h"
#include "ReaderAppearanceRepository.h"
#include "ReaderAppearanceDefaults.h"

ReaderAppearanceController::ReaderAppearanceController(QObject *parent)
    : QObject(parent)
{
    m_settings = ReaderAppearanceRepository::load();
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
