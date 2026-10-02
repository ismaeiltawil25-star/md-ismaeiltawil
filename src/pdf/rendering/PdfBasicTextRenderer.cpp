#include "PdfBasicTextRenderer.h"
#include <QtMath>
#include <cmath>

using W = PdfBasicTextRenderWarning;

static void addDiag(PdfBasicTextRenderResult &r, W w, int idx, const QString &msg) {
    PdfBasicTextRenderDiagnostic d; d.warning = w; d.runIndex = idx; d.message = msg;
    r.diagnostics.append(d);
}

bool PdfBasicTextRenderResult::ok() const {
    return warning == W::None || warning == W::NoTextRuns
        || (warning == W::SkippedRun && runsRendered > 0);
}

QFont PdfBasicTextRenderer::fallbackFont(double fontSizePt, double dpi) {
    double safeF = (std::isfinite(fontSizePt) && fontSizePt > 0.0) ? fontSizePt : 1.0;
    double safeD = (std::isfinite(dpi) && dpi > 0.0) ? dpi : 72.0;
    int px = std::max(1, static_cast<int>(std::ceil(safeF * safeD / 72.0)));
    QFont f("sans-serif"); f.setPixelSize(px); return f;
}

QString PdfBasicTextRenderer::decodeTextBytes(const QByteArray &raw, bool &ok, QString &diag) {
    if (raw.size() >= 2
        && static_cast<unsigned char>(raw[0]) == 0xFE
        && static_cast<unsigned char>(raw[1]) == 0xFF) {
        ok = false;
        diag = QStringLiteral("UTF-16BE BOM detected \u2014 unsupported encoding");
        return {};
    }
    QString s;
    for (int i = 0; i < raw.size(); ++i) {
        unsigned char b = static_cast<unsigned char>(raw[i]);
        if (b <= 0x1F || (b >= 0x7F && b <= 0x9F))
            s += QChar('?');
        else
            s += QLatin1Char(b);
    }
    ok = true;
    return s;
}

PdfBasicTextRenderResult PdfBasicTextRenderer::render(
    QPainter &painter, const PdfPageRenderPlan &plan, const PdfPathRenderSettings &settings) {
    PdfBasicTextRenderResult result;

    if (settings.dpi <= 0.0 || !std::isfinite(settings.dpi))
        goto invalidSettings;
    if (!painter.isActive() || !painter.device()
        || painter.device()->width() <= 0 || painter.device()->height() <= 0)
        goto invalidSettings;

    if (plan.textRuns.isEmpty()) {
        result.warning = W::NoTextRuns;
        result.diagnosticSummary = QStringLiteral("No text runs to render");
        return result;
    }

    {
        painter.save();
        painter.setPen(settings.defaultStrokeColor);

        const PdfPageBox &vb = plan.geometry.visibleBox;
        const double scale = settings.dpi / 72.0;
        const int rotation = plan.geometry.rotation;
        const double cx = (vb.llx + vb.urx) / 2.0;
        const double cy = (vb.lly + vb.ury) / 2.0;
        const int imgW = painter.device()->width();
        const int imgH = painter.device()->height();

        for (int ri = 0; ri < plan.textRuns.size(); ++ri) {
            const auto &run = plan.textRuns[ri];

            if (run.fontSize <= 0.0 || !std::isfinite(run.fontSize)) {
                addDiag(result, W::InvalidFontSize, ri,
                         QStringLiteral("Invalid font size: %1").arg(run.fontSize));
                result.runsSkipped++;
                continue;
            }

            const double fontPx = run.fontSize * settings.dpi / 72.0;
            const double advancePerChar = fontPx * 0.6;

            const double tx = run.textMatrixSnapshot.e;
            const double ty = run.textMatrixSnapshot.f;

            QPointF origin = PdfBasicPathRenderer::mapPdfPoint(
                tx, ty, run.ctmSnapshot, scale, rotation, cx, cy, imgW, imgH);

            painter.setFont(fallbackFont(run.fontSize, settings.dpi));

            if (run.runType == PdfTextRunType::TJ) {
                double x = origin.x();
                bool anyRendered = false;
                for (int ei = 0; ei < run.tjElements.size(); ++ei) {
                    const auto &elem = run.tjElements[ei];
                    if (elem.isString) {
                        bool ok; QString diag;
                        QString s = decodeTextBytes(elem.stringBytes, ok, diag);
                        if (!ok) {
                            addDiag(result, W::UnsupportedEncoding, ri,
                                     QStringLiteral("TJ element %1: %2").arg(ei).arg(diag));
                            continue;
                        }
                        for (int ci = 0; ci < s.length(); ++ci) {
                            painter.drawText(QPointF(x, origin.y()), QString(s[ci]));
                            x += advancePerChar;
                        }
                        anyRendered = true;
                    } else {
                        x += -elem.numericOffset * fontPx / 1000.0;
                    }
                }
                if (anyRendered) result.runsRendered++;
                else result.runsSkipped++;
            } else {
                bool ok; QString diag;
                QString s = decodeTextBytes(run.textBytes, ok, diag);
                if (!ok) {
                    addDiag(result, W::UnsupportedEncoding, ri, diag);
                    result.runsSkipped++;
                    continue;
                }
                double x = origin.x();
                for (int ci = 0; ci < s.length(); ++ci) {
                    painter.drawText(QPointF(x, origin.y()), QString(s[ci]));
                    x += advancePerChar;
                }
                result.runsRendered++;
            }
        }

        painter.restore();
    }

    if (result.runsSkipped > 0 || !result.diagnostics.isEmpty())
        result.warning = W::SkippedRun;

    if (result.warning == W::SkippedRun) {
        if (result.runsSkipped > 0)
            result.diagnosticSummary = QStringLiteral("%1 run(s) skipped: %2")
                .arg(result.runsSkipped)
                .arg(result.diagnostics.isEmpty() ? QString() : result.diagnostics.first().message);
        else
            result.diagnosticSummary = QStringLiteral("Text render diagnostics: %1")
                .arg(result.diagnostics.first().message);
    }

    return result;

invalidSettings:
    result.warning = W::InvalidSettings;
    result.diagnosticSummary = QStringLiteral("Invalid settings");
    return result;
}
