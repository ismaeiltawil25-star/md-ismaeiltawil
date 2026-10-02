#pragma once

// PageLayoutRepository — loads and saves PageLayoutSettings with QSettings
// (group "pageLayout"). Unknown or corrupt values fall back to defaults, so a
// bad settings file can never break document display.

#include "PageLayoutSettings.h"

namespace mdbr::layout {

class PageLayoutRepository
{
public:
    [[nodiscard]] static PageLayoutSettings load();
    static void save(const PageLayoutSettings &settings);
    static void clear();
};

} // namespace mdbr::layout
