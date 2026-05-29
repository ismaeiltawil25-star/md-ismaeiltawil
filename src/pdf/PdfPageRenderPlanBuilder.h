#pragma once

#include "PdfPageGeometryResolver.h"
#include "PdfPageContentPipeline.h"
#include "PdfStreamDecoder.h"
#include <QString>

struct PdfPageRenderPlan {
    int pageIndex = -1;
    PdfRef pageRef;

    PdfPageGeometry geometry;

    QList<PdfPathRecord> paths;
    QList<PdfTextRun> textRuns;

    PdfGraphicsStateResult graphicsStateResult;

    bool hasContent = false;
};

enum class PdfPageRenderPlanWarning : uint8_t {
    None,
    InvalidPageIndex,
    GeometryFailure,
    ContentPipelineFailure,
    PageIdentityMismatch,
};

struct PdfPageRenderPlanResult {
    PdfPageRenderPlan plan;

    PdfPageGeometryResult geometryResult;
    PdfPageContentPipelineResult contentResult;

    PdfPageRenderPlanWarning warning = PdfPageRenderPlanWarning::None;
    QString diagnosticSummary;

    bool ok() const { return warning == PdfPageRenderPlanWarning::None; }
};

class PdfPageRenderPlanBuilder {
public:
    explicit PdfPageRenderPlanBuilder(PdfStreamDecoder &decoder);
    PdfPageRenderPlanResult build(const PdfDocument &document, int pageIndex);

private:
    PdfStreamDecoder &m_decoder;
};
