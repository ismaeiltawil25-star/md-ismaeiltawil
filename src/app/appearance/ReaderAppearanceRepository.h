#ifndef READERAPPEARANCEREPOSITORY_H
#define READERAPPEARANCEREPOSITORY_H

#include "ReaderAppearanceSettings.h"
#include "ReaderTheme.h"

class ReaderAppearanceRepository
{
public:
    static ReaderAppearanceSettings load();
    static bool save(const ReaderAppearanceSettings &settings);
    static void clear();

    // Theme mode (System/Day/Night/Custom). Settings saved before modes existed
    // are migrated from their stored colours; a fresh install follows Windows.
    static mdbr::appearance::ThemeMode loadThemeMode();
    static void saveThemeMode(mdbr::appearance::ThemeMode mode);

    static constexpr int SchemaVersion = 1;
};

#endif
