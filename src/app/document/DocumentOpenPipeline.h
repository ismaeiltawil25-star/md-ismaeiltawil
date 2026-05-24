#ifndef DOCUMENTOPENPIPELINE_H
#define DOCUMENTOPENPIPELINE_H

#include <QString>
#include <functional>
#include "DocumentOpenResult.h"

struct PipelineRenderResult
{
    QString html;
    QString errorMessage;

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
