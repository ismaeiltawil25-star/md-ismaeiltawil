#pragma once

#include "PdfPageRenderPlanBuilder.h"
#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QTransform>
#include <QString>
#include <cstdint>
#include <cmath>

struct PdfPathRenderSettings {
    double dpi = 72.0;
    QColor backgroundColor = Qt::white;
    QColor defaultStrokeColor = Qt::black;
    QColor defaultFillColor = Qt::black;
    double defaultLineWidthPx = 1.0;
    bool antialiasing = false;
    int maxDimension = 4096;
};

enum class PdfPathRenderWarning : uint8_t {
    None,
    NoPaths,
    InvalidGeometry,
    InvalidSettings,
    ImageTooLarge,
};

struct PdfPathRenderResult {
    QImage image;
    int imageWidth = 0, imageHeight = 0;
    PdfPageBox sourceGeometry;
    int rotation = 0;
    PdfPathRenderWarning warning = PdfPathRenderWarning::None;
    QString diagnosticSummary;

    bool ok() const {
        return warning == PdfPathRenderWarning::None
            || warning == PdfPathRenderWarning::NoPaths;
    }
};

class PdfBasicPathRenderer {
public:
    PdfPathRenderResult render(const PdfPageRenderPlan &plan,
                               const PdfPathRenderSettings &settings = {});

    static QTransform segmentCtmToQt(const PdfMatrix &m);
    static QPointF mapPdfPoint(double pdfX, double pdfY,
                                const PdfMatrix &ctm,
                                double scale, int rotation,
                                double centerX, double centerY,
                                int imgW, int imgH);
    static QTransform buildPageTransform(double scale, int rotation,
                                          double centerX, double centerY,
                                          int imgW, int imgH);
    static void applyPaintMode(QPainter &painter, QPainterPath &path,
                                PdfPaintMode mode, const PdfPathRenderSettings &s);
};
