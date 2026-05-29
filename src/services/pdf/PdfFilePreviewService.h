#pragma once

#include "PdfPageImageRenderer.h"
#include "PdfDocumentLoader.h"
#include <QImage>
#include <QString>
#include <cstdint>

enum class PdfFilePreviewWarning : uint8_t {
    None,
    FileNotFound,
    FileOpenFailure,
    FileReadFailure,
    EmptyFile,
    DocumentLoadFailure,
    RenderFailure,
    NoRenderablePaths,
};

struct PdfFilePreviewResult {
    QString filePath;
    QImage image;
    int imageWidth = 0, imageHeight = 0;

    PdfDocumentResult documentResult;
    PdfPageImageRenderResult renderResult;

    PdfFilePreviewWarning warning = PdfFilePreviewWarning::None;
    QString diagnosticSummary;

    bool ok() const {
        return warning == PdfFilePreviewWarning::None
            || warning == PdfFilePreviewWarning::NoRenderablePaths;
    }
};

class PdfFilePreviewService {
public:
    PdfFilePreviewResult renderFilePage(const QString &filePath,
                                         int pageIndex = 0,
                                         const PdfPathRenderSettings &settings = {});
};
