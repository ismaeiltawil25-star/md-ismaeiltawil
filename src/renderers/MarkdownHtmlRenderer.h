#ifndef MARKDOWNHTMLRENDERER_H
#define MARKDOWNHTMLRENDERER_H

#include <QString>
#include <QList>
#include "models/MarkdownDocument.h"
#include "renderers/MarkdownExtensions.h"
#include "renderers/RenderPipelineStage.h"

struct StageDiagnostic
{
    RenderStage stage;
    bool success = true;
    QString warning;
    qint64 elapsedMs = 0;
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

    // `extensions` switches optional syntax on/off (all on by default).
    static Result render(const QString &markdown,
                         const mdbr::MarkdownExtensions &extensions = {});
    static Result render(const MarkdownDocument &doc);
};

#endif
