#include "PdfPageRenderPlanBuilder.h"

PdfPageRenderPlanBuilder::PdfPageRenderPlanBuilder(PdfStreamDecoder &decoder)
    : m_decoder(decoder) {}

PdfPageRenderPlanResult PdfPageRenderPlanBuilder::build(
    const PdfDocument &document, int pageIndex) {
    PdfPageRenderPlanResult result;

    // 1. Geometry
    PdfPageGeometryResolver geoResolver;
    result.geometryResult = geoResolver.resolve(document, pageIndex);

    if (result.geometryResult.warning == PdfPageGeometryWarning::InvalidPageIndex) {
        result.warning = PdfPageRenderPlanWarning::InvalidPageIndex;
        result.diagnosticSummary = QStringLiteral("Invalid page index: %1").arg(pageIndex);
        return result;
    }

    // 2. Content pipeline
    PdfPageContentPipeline pipeline(m_decoder);
    result.contentResult = pipeline.analyze(document, pageIndex);

    // 3. Page identity
    if (result.geometryResult.geometry.pageRef.objNum > 0 &&
        result.contentResult.pageRef.objNum > 0) {
        if (result.geometryResult.geometry.pageRef.objNum != result.contentResult.pageRef.objNum ||
            result.geometryResult.geometry.pageRef.genNum != result.contentResult.pageRef.genNum) {
            result.warning = PdfPageRenderPlanWarning::PageIdentityMismatch;
            result.diagnosticSummary = QStringLiteral("Page identity mismatch: geom=%1 %2 R, content=%3 %4 R")
                .arg(result.geometryResult.geometry.pageRef.objNum)
                .arg(result.geometryResult.geometry.pageRef.genNum)
                .arg(result.contentResult.pageRef.objNum)
                .arg(result.contentResult.pageRef.genNum);
        }
    }

    // 4. Determine warning
    if (result.warning == PdfPageRenderPlanWarning::None) {
        if (!result.geometryResult.ok())
            result.warning = PdfPageRenderPlanWarning::GeometryFailure;
        else if (!result.contentResult.ok())
            result.warning = PdfPageRenderPlanWarning::ContentPipelineFailure;
    }

    // 5. Build plan
    result.plan.pageIndex = pageIndex;
    result.plan.pageRef = result.geometryResult.geometry.pageRef;
    result.plan.geometry = result.geometryResult.geometry;
    result.plan.paths = result.contentResult.pathResult.records;
    result.plan.textRuns = result.contentResult.textRunResult.runs;
    result.plan.graphicsStateResult = result.contentResult.graphicsStateResult;
    result.plan.hasContent = !result.plan.paths.isEmpty() || !result.plan.textRuns.isEmpty();

    // Diagnostic summary
    if (result.diagnosticSummary.isEmpty()) {
        if (result.warning == PdfPageRenderPlanWarning::GeometryFailure)
            result.diagnosticSummary = QStringLiteral("Geometry: %1").arg(result.geometryResult.diagnosticMessage);
        else if (result.warning == PdfPageRenderPlanWarning::ContentPipelineFailure)
            result.diagnosticSummary = QStringLiteral("Content: %1").arg(result.contentResult.diagnosticSummary());
    }

    return result;
}
