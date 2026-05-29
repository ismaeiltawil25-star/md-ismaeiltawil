#include "PdfPageImageRenderer.h"

PdfPageImageRenderer::PdfPageImageRenderer(PdfStreamDecoder &decoder)
    : m_decoder(decoder) {}

PdfPageImageRenderResult PdfPageImageRenderer::renderPage(
    const PdfDocument &document, int pageIndex,
    const PdfPathRenderSettings &settings) {
    PdfPageImageRenderResult result;

    PdfPageRenderPlanBuilder planBuilder(m_decoder);
    result.planResult = planBuilder.build(document, pageIndex);

    if (result.planResult.warning == PdfPageRenderPlanWarning::InvalidPageIndex) {
        result.warning = PdfPageImageRenderWarning::PlanBuildFailure;
        result.diagnosticSummary = result.planResult.diagnosticSummary;
        return result;
    }

    // Render if visibleBox is valid
    if (result.planResult.plan.geometry.visibleBox.isValid()) {
        PdfBasicPathRenderer pathRenderer;
        result.pathResult = pathRenderer.render(result.planResult.plan, settings);
    }

    // Determine warning
    if (result.pathResult.ok() == false)
        result.warning = PdfPageImageRenderWarning::PathRenderFailure;
    else if (!result.planResult.ok())
        result.warning = PdfPageImageRenderWarning::PlanBuildFailure;
    else if (result.pathResult.warning == PdfPathRenderWarning::NoPaths)
        result.warning = PdfPageImageRenderWarning::NoRenderablePaths;

    // Copy image data
    result.image = result.pathResult.image;
    result.imageWidth = result.pathResult.imageWidth;
    result.imageHeight = result.pathResult.imageHeight;

    // Diagnostic summary
    QStringList parts;
    if (result.planResult.diagnosticSummary.size())
        parts.append(QStringLiteral("Plan: %1").arg(result.planResult.diagnosticSummary));
    if (result.pathResult.diagnosticSummary.size())
        parts.append(QStringLiteral("Render: %1").arg(result.pathResult.diagnosticSummary));
    result.diagnosticSummary = parts.join(QStringLiteral("; "));

    return result;
}
