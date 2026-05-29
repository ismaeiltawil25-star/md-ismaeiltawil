#pragma once

#include "PdfPageRenderPlanBuilder.h"
#include "PdfBasicPathRenderer.h"
#include "PdfStreamDecoder.h"
#include <QImage>
#include <QString>
#include <cstdint>

enum class PdfPageImageRenderWarning : uint8_t {
    None,
    PlanBuildFailure,
    PathRenderFailure,
    NoRenderablePaths,
};

struct PdfPageImageRenderResult {
    QImage image;
    int imageWidth = 0, imageHeight = 0;

    PdfPageRenderPlanResult planResult;
    PdfPathRenderResult pathResult;

    PdfPageImageRenderWarning warning = PdfPageImageRenderWarning::None;
    QString diagnosticSummary;

    bool ok() const {
        return warning == PdfPageImageRenderWarning::None
            || warning == PdfPageImageRenderWarning::NoRenderablePaths;
    }
};

class PdfPageImageRenderer {
public:
    explicit PdfPageImageRenderer(PdfStreamDecoder &decoder);
    PdfPageImageRenderResult renderPage(const PdfDocument &document,
                                         int pageIndex,
                                         const PdfPathRenderSettings &settings = {});

private:
    PdfStreamDecoder &m_decoder;
};
