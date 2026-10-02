#ifndef READERSTYLESHEETBUILDER_H
#define READERSTYLESHEETBUILDER_H

#include <QString>
#include "ReaderAppearanceSettings.h"

class ReaderStylesheetBuilder
{
public:
    static QString buildDocumentStylesheet(const ReaderAppearanceSettings &settings);
    // backgroundOverride replaces the page colour for the widget background
    // (used by page view, where the widget shows the desk around the pages).
    static QString buildWidgetStylesheet(const ReaderAppearanceSettings &settings,
                                         const QString &backgroundOverride = QString());
};

#endif
