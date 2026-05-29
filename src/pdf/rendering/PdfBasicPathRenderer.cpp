#include "PdfBasicPathRenderer.h"
#include <cmath>

using W = PdfPathRenderWarning;

static PdfPathRenderResult fail(W w, const QString &msg) {
    PdfPathRenderResult r; r.warning = w; r.diagnosticSummary = msg; return r;
}

static bool isFinite(double v) { return std::isfinite(v); }

QTransform PdfBasicPathRenderer::segmentCtmToQt(const PdfMatrix &m) {
    return QTransform(m.a, m.b, m.c, m.d, m.e, m.f);
}

QPointF PdfBasicPathRenderer::mapPdfPoint(
    double pdfX, double pdfY, const PdfMatrix &ctm,
    double scale, int rotation,
    double centerX, double centerY, int imgW, int imgH) {
    double x1 = pdfX * ctm.a + pdfY * ctm.c + ctm.e;
    double y1 = pdfX * ctm.b + pdfY * ctm.d + ctm.f;
    double x2 = x1 - centerX;
    double y2 = y1 - centerY;
    double x3, y3;
    switch (rotation) {
    case 0:   x3 = x2;  y3 = y2; break;
    case 90:  x3 = -y2; y3 = x2; break;
    case 180: x3 = -x2; y3 = -y2; break;
    case 270: x3 = y2;  y3 = -x2; break;
    default:  x3 = x2;  y3 = y2; break;
    }
    double x4 = x3 * scale;
    double y4 = -y3 * scale;
    double x5 = x4 + imgW / 2.0;
    double y5 = y4 + imgH / 2.0;
    return QPointF(x5, y5);
}

QTransform PdfBasicPathRenderer::buildPageTransform(
    double scale, int rotation,
    double centerX, double centerY, int imgW, int imgH) {
    QTransform t;
    t.translate(imgW / 2.0, imgH / 2.0);
    t.rotate(static_cast<double>(rotation));
    t.scale(scale, -scale);
    t.translate(-centerX, -centerY);
    return t;
}

void PdfBasicPathRenderer::applyPaintMode(QPainter &painter, QPainterPath &path,
                                           PdfPaintMode mode, const PdfPathRenderSettings &s) {
    switch (mode) {
    case PdfPaintMode::None: return;
    case PdfPaintMode::Stroke:
        painter.setPen(QPen(s.defaultStrokeColor, s.defaultLineWidthPx));
        painter.setBrush(Qt::NoBrush); break;
    case PdfPaintMode::CloseStroke:
        path.closeSubpath();
        painter.setPen(QPen(s.defaultStrokeColor, s.defaultLineWidthPx));
        painter.setBrush(Qt::NoBrush); break;
    case PdfPaintMode::Fill:
        painter.setPen(Qt::NoPen); painter.setBrush(s.defaultFillColor); break;
    case PdfPaintMode::FillEvenOdd:
        path.setFillRule(Qt::OddEvenFill);
        painter.setPen(Qt::NoPen); painter.setBrush(s.defaultFillColor); break;
    case PdfPaintMode::FillStroke:
        painter.setPen(QPen(s.defaultStrokeColor, s.defaultLineWidthPx));
        painter.setBrush(s.defaultFillColor); break;
    case PdfPaintMode::FillStrokeEvenOdd:
        path.setFillRule(Qt::OddEvenFill);
        painter.setPen(QPen(s.defaultStrokeColor, s.defaultLineWidthPx));
        painter.setBrush(s.defaultFillColor); break;
    case PdfPaintMode::CloseFillStroke:
        path.closeSubpath();
        painter.setPen(QPen(s.defaultStrokeColor, s.defaultLineWidthPx));
        painter.setBrush(s.defaultFillColor); break;
    case PdfPaintMode::CloseFillStrokeEvenOdd:
        path.closeSubpath(); path.setFillRule(Qt::OddEvenFill);
        painter.setPen(QPen(s.defaultStrokeColor, s.defaultLineWidthPx));
        painter.setBrush(s.defaultFillColor); break;
    }
    painter.drawPath(path);
}

PdfPathRenderResult PdfBasicPathRenderer::render(
    const PdfPageRenderPlan &plan, const PdfPathRenderSettings &settings) {

    if (settings.dpi <= 0.0 || !isFinite(settings.dpi))
        return fail(W::InvalidSettings, QStringLiteral("DPI must be positive and finite"));
    if (settings.maxDimension <= 0)
        return fail(W::InvalidSettings, QStringLiteral("maxDimension must be positive"));
    if (settings.defaultLineWidthPx < 0.0 || !isFinite(settings.defaultLineWidthPx))
        return fail(W::InvalidSettings, QStringLiteral("line width must be finite and non-negative"));

    const PdfPageBox &vb = plan.geometry.visibleBox;
    if (!vb.isValid())
        return fail(W::InvalidGeometry, QStringLiteral("visibleBox has zero or negative dimensions"));

    int rotation = plan.geometry.rotation;
    if (rotation != 0 && rotation != 90 && rotation != 180 && rotation != 270)
        return fail(W::InvalidGeometry, QStringLiteral("Unsupported rotation: %1").arg(rotation));

    const double scale = settings.dpi / 72.0;
    double rawW = vb.width(), rawH = vb.height();
    if (rotation == 90 || rotation == 270) std::swap(rawW, rawH);

    int imgW = static_cast<int>(std::ceil(rawW * scale));
    int imgH = static_cast<int>(std::ceil(rawH * scale));
    if (imgW < 1) imgW = 1;
    if (imgH < 1) imgH = 1;

    if (imgW > settings.maxDimension || imgH > settings.maxDimension)
        return fail(W::ImageTooLarge, QStringLiteral("Image %1x%2 exceeds max %3")
            .arg(imgW).arg(imgH).arg(settings.maxDimension));

    const double centerX = (vb.llx + vb.urx) / 2.0;
    const double centerY = (vb.lly + vb.ury) / 2.0;

    auto mapPt = [&](double x, double y, const PdfMatrix &ctm) {
        return mapPdfPoint(x, y, ctm, scale, rotation, centerX, centerY, imgW, imgH);
    };

    QImage img(imgW, imgH, QImage::Format_ARGB32_Premultiplied);
    img.fill(settings.backgroundColor);

    if (plan.paths.isEmpty()) {
        PdfPathRenderResult r;
        r.warning = W::NoPaths;
        r.diagnosticSummary = QStringLiteral("No path records to render");
        r.image = img; r.imageWidth = imgW; r.imageHeight = imgH;
        r.sourceGeometry = vb; r.rotation = rotation;
        return r;
    }

    QPainter painter(&img);
    if (settings.antialiasing)
        painter.setRenderHint(QPainter::Antialiasing);

    for (const auto &rec : plan.paths) {
        QPainterPath path;
        for (const auto &seg : rec.segments) {
            switch (seg.type) {
            case PdfPathSegmentType::MoveTo:
                path.moveTo(mapPt(seg.x, seg.y, seg.ctmSnapshot));
                break;
            case PdfPathSegmentType::LineTo:
                path.lineTo(mapPt(seg.x, seg.y, seg.ctmSnapshot));
                break;
            case PdfPathSegmentType::CubicBezier: {
                auto p1 = mapPt(seg.x1, seg.y1, seg.ctmSnapshot);
                auto p2 = mapPt(seg.x2, seg.y2, seg.ctmSnapshot);
                auto p3 = mapPt(seg.x3, seg.y3, seg.ctmSnapshot);
                path.cubicTo(p1, p2, p3);
                break;
            }
            case PdfPathSegmentType::Rectangle: {
                auto c0 = mapPt(seg.x, seg.y, seg.ctmSnapshot);
                auto c1 = mapPt(seg.x + seg.w, seg.y, seg.ctmSnapshot);
                auto c2 = mapPt(seg.x + seg.w, seg.y + seg.h, seg.ctmSnapshot);
                auto c3 = mapPt(seg.x, seg.y + seg.h, seg.ctmSnapshot);
                path.moveTo(c0); path.lineTo(c1); path.lineTo(c2);
                path.lineTo(c3); path.closeSubpath();
                break;
            }
            case PdfPathSegmentType::CloseSubpath:
                path.closeSubpath();
                break;
            }
        }
        applyPaintMode(painter, path, rec.paintMode, settings);
    }
    painter.end();

    PdfPathRenderResult r;
    r.image = img; r.imageWidth = imgW; r.imageHeight = imgH;
    r.sourceGeometry = vb; r.rotation = rotation;
    return r;
}
