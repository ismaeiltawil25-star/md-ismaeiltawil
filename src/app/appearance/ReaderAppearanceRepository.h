#ifndef READERAPPEARANCEREPOSITORY_H
#define READERAPPEARANCEREPOSITORY_H

#include "ReaderAppearanceSettings.h"

class ReaderAppearanceRepository
{
public:
    static ReaderAppearanceSettings load();
    static bool save(const ReaderAppearanceSettings &settings);
    static void clear();

    static constexpr int SchemaVersion = 1;
};

#endif
