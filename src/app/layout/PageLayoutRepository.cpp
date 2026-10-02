#include "PageLayoutRepository.h"

#include <QSettings>

namespace mdbr::layout {

namespace {
constexpr auto kGroup = "pageLayout";
constexpr auto kEnabled = "pageViewEnabled";
constexpr auto kPaper = "paper";
constexpr auto kMargins = "margins";
} // namespace

PageLayoutSettings PageLayoutRepository::load()
{
    QSettings store;
    store.beginGroup(kGroup);
    const PageLayoutSettings defaults;

    PageLayoutSettings settings;
    settings.pageViewEnabled = store.value(kEnabled, defaults.pageViewEnabled).toBool();
    settings.paper = paperFromId(store.value(kPaper).toString(), defaults.paper);
    settings.margins = marginsFromId(store.value(kMargins).toString(), defaults.margins);
    return settings;
}

void PageLayoutRepository::save(const PageLayoutSettings &settings)
{
    QSettings store;
    store.beginGroup(kGroup);
    store.setValue(kEnabled, settings.pageViewEnabled);
    store.setValue(kPaper, paperId(settings.paper));
    store.setValue(kMargins, marginsId(settings.margins));
}

void PageLayoutRepository::clear()
{
    QSettings store;
    store.remove(kGroup);
}

} // namespace mdbr::layout
