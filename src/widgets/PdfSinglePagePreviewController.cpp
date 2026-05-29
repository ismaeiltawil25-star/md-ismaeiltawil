#include "PdfSinglePagePreviewController.h"

PdfSinglePagePreviewController::PdfSinglePagePreviewController(
    PdfSinglePagePreviewWidget *widget, QObject *parent)
    : QObject(parent), m_widget(widget) {}

PdfPageImageRenderResult PdfSinglePagePreviewController::renderDocumentPage(
    const PdfDocument &document, int pageIndex, const PdfPathRenderSettings &settings) {

    PdfPageImageRenderer renderer(m_decoder);
    auto result = renderer.renderPage(document, pageIndex, settings);

    if (!m_widget)
        return result;

    switch (result.warning) {
    case PdfPageImageRenderWarning::None:
        m_widget->setImage(result.image);
        m_widget->setStatusText(QString{});
        break;
    case PdfPageImageRenderWarning::NoRenderablePaths:
        m_widget->setImage(result.image);
        m_widget->setStatusText(QStringLiteral("No renderable content on this page"));
        break;
    case PdfPageImageRenderWarning::PlanBuildFailure:
        if (!result.image.isNull()) {
            m_widget->setImage(result.image);
            m_widget->setStatusText(result.diagnosticSummary.isEmpty()
                ? QStringLiteral("Unable to build PDF page preview")
                : result.diagnosticSummary);
        } else {
            m_widget->clear();
            m_widget->setStatusText(result.diagnosticSummary.isEmpty()
                ? QStringLiteral("Unable to build PDF page preview")
                : result.diagnosticSummary);
        }
        break;
    case PdfPageImageRenderWarning::PathRenderFailure:
        m_widget->clear();
        m_widget->setStatusText(result.diagnosticSummary.isEmpty()
            ? QStringLiteral("Unable to render PDF page preview")
            : result.diagnosticSummary);
        break;
    }

    return result;
}

void PdfSinglePagePreviewController::clear() {
    if (m_widget)
        m_widget->clear();
}
