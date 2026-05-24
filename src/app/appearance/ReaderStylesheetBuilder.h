#ifndef READERSTYLESHEETBUILDER_H
#define READERSTYLESHEETBUILDER_H

#include <QString>
#include "ReaderAppearanceSettings.h"

class ReaderStylesheetBuilder
{
public:
    static QString buildDocumentStylesheet(const ReaderAppearanceSettings &settings);
    static QString buildWidgetStylesheet(const ReaderAppearanceSettings &settings);
};

#endif
