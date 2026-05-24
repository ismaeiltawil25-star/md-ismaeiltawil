#ifndef MARKDOWNHTMLRENDERER_H
#define MARKDOWNHTMLRENDERER_H

#include <QString>
#include <QList>
#include "models/MarkdownDocument.h"
#include "renderers/RenderPipelineStage.h"

struct StageDiagnostic
{
    RenderStage stage;
    bool success = true;
    QString warning;
};

class MarkdownHtmlRenderer
{
public:
    struct Result
    {
        QString html;
        QString errorMessage;
        QList<StageDiagnostic> diagnostics;

        bool isValid() const { return errorMessage.isEmpty(); }
    };

    static Result render(const QString &markdown);
    static Result render(const MarkdownDocument &doc);
};

#endif
