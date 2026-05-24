#ifndef DOCUMENTTABSTATE_H
#define DOCUMENTTABSTATE_H

#include <QString>

enum class ContentMode { Html, Plain };

struct DocumentTabState
{
    QString filePath;
    int zoomSteps = 0;
    ContentMode contentMode = ContentMode::Plain;
    QString originalHtml;
    QString originalPlainText;
};

#endif
