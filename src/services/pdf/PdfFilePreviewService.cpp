#include "PdfFilePreviewService.h"
#include <QFile>
#include <QFileInfo>

PdfFilePreviewResult PdfFilePreviewService::renderFilePage(
    const QString &filePath, int pageIndex, const PdfPathRenderSettings &settings) {
    PdfFilePreviewResult result;
    result.filePath = filePath;

    if (filePath.isEmpty()) {
        result.warning = PdfFilePreviewWarning::FileNotFound;
        result.diagnosticSummary = QStringLiteral("No file path provided");
        return result;
    }

    QFileInfo fi(filePath);
    if (!fi.exists()) {
        result.warning = PdfFilePreviewWarning::FileNotFound;
        result.diagnosticSummary = QStringLiteral("File not found: %1").arg(filePath);
        return result;
    }
    if (fi.isDir()) {
        result.warning = PdfFilePreviewWarning::FileOpenFailure;
        result.diagnosticSummary = QStringLiteral("Path is a directory: %1").arg(filePath);
        return result;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.warning = PdfFilePreviewWarning::FileOpenFailure;
        result.diagnosticSummary = QStringLiteral("Cannot open file: %1").arg(filePath);
        return result;
    }

    QByteArray data = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        result.warning = PdfFilePreviewWarning::FileReadFailure;
        result.diagnosticSummary = QStringLiteral("File read error: %1").arg(filePath);
        return result;
    }

    if (data.isEmpty()) {
        result.warning = PdfFilePreviewWarning::EmptyFile;
        result.diagnosticSummary = QStringLiteral("File is empty: %1").arg(filePath);
        return result;
    }

    result.documentResult = PdfDocumentLoader().load(data);
    if (result.documentResult.warning != PdfDocumentLoaderWarning::None) {
        result.warning = PdfFilePreviewWarning::DocumentLoadFailure;
        result.diagnosticSummary = QStringLiteral("Document load failed: %1")
            .arg(result.documentResult.diagnosticMessage);
        return result;
    }

    PdfStreamDecoder decoder;
    PdfPageImageRenderer renderer(decoder);
    result.renderResult = renderer.renderPage(result.documentResult.document, pageIndex, settings);

    result.image = result.renderResult.image;
    result.imageWidth = result.renderResult.imageWidth;
    result.imageHeight = result.renderResult.imageHeight;

    switch (result.renderResult.warning) {
    case PdfPageImageRenderWarning::None:
        result.warning = PdfFilePreviewWarning::None;
        break;
    case PdfPageImageRenderWarning::NoRenderablePaths:
        result.warning = PdfFilePreviewWarning::NoRenderablePaths;
        result.diagnosticSummary = QStringLiteral("No renderable content on this page");
        break;
    default:
        result.warning = PdfFilePreviewWarning::RenderFailure;
        result.diagnosticSummary = QStringLiteral("Render failed: %1")
            .arg(result.renderResult.diagnosticSummary);
        break;
    }

    return result;
}
