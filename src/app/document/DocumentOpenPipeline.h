#ifndef DOCUMENTOPENPIPELINE_H
#define DOCUMENTOPENPIPELINE_H

#include <QString>
#include <QList>
#include <functional>
#include "DocumentOpenResult.h"
#include "renderers/MarkdownHtmlRenderer.h"

struct PipelineRenderResult
{
    QString html;
    QString errorMessage;
    QList<StageDiagnostic> diagnostics;

    bool isValid() const { return errorMessage.isEmpty(); }
};

class DocumentOpenPipeline
{
public:
    using RenderFn = std::function<PipelineRenderResult(const QString &)>;

    explicit DocumentOpenPipeline(RenderFn renderFn = nullptr);
    DocumentOpenResult open(const QString &filePath);

private:
    RenderFn m_renderFn;
};

#endif
