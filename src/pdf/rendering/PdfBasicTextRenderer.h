#pragma once

#include "PdfPageRenderPlanBuilder.h"
#include "PdfBasicPathRenderer.h"
#include <QFont>
#include <QPainter>
#include <QString>
#include <QList>
#include <cstdint>

enum class PdfBasicTextRenderWarning : uint8_t {
    None,
    InvalidSettings,
    NoTextRuns,
    SkippedRun,
    UnsupportedEncoding,
    InvalidFontSize,
};

struct PdfBasicTextRenderDiagnostic {
    PdfBasicTextRenderWarning warning;
    int runIndex = -1;
    QString message;
};

struct PdfBasicTextRenderResult {
    int runsRendered = 0;
    int runsSkipped = 0;
    QList<PdfBasicTextRenderDiagnostic> diagnostics;
    PdfBasicTextRenderWarning warning = PdfBasicTextRenderWarning::None;
    QString diagnosticSummary;

    bool ok() const;
};

class PdfBasicTextRenderer {
public:
    PdfBasicTextRenderResult render(QPainter &painter,
                                     const PdfPageRenderPlan &plan,
                                     const PdfPathRenderSettings &settings);

    static QFont fallbackFont(double fontSizePt, double dpi);
    static QString decodeTextBytes(const QByteArray &rawBytes,
                                    bool &ok, QString &diag);
};
