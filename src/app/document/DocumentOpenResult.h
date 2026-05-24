#ifndef DOCUMENTOPENRESULT_H
#define DOCUMENTOPENRESULT_H

#include <QString>
#include <QList>
#include <QUrl>
#include "DocumentTabState.h"
#include "renderers/MarkdownHtmlRenderer.h"

struct DocumentOpenResult
{
    DocumentTabState tabState;
    QString fileName;
    QString errorMessage;
    QString renderWarning;
    QList<StageDiagnostic> diagnostics;
    QUrl baseUrl;

    bool isValid() const { return errorMessage.isEmpty(); }
};

#endif
