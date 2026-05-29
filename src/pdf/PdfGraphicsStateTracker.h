#pragma once

#include "PdfMatrix.h"
#include "PdfContentStreamInterpreter.h"
#include <QByteArray>
#include <QList>
#include <QString>
#include <cstdint>

struct PdfGraphicsState {
    PdfMatrix ctm;

    bool textObject = false;
    PdfMatrix textMatrix;
    PdfMatrix lineMatrix;
    QByteArray fontName;
    double fontSize = 0.0;
    double leading = 0.0;
    double wordSpacing = 0.0;
    double charSpacing = 0.0;

    double lineWidth = 1.0;
    int lineCap = 0;
    int lineJoin = 0;
    double miterLimit = 10.0;

    double strokeGray = 0.0;
    double fillGray = 0.0;
    double strokeR = 0.0, strokeG = 0.0, strokeB = 0.0;
    double fillR = 0.0, fillG = 0.0, fillB = 0.0;
};

enum class PdfGraphicsStateDiagnostic : uint8_t {
    None,
    StateStackUnderflow,
    NestedBT,
    ETWithoutBT,
    TextShowingOutsideBT,
    TextPositionOutsideBT,
};

struct PdfGraphicsStateDiagnosticEntry {
    PdfGraphicsStateDiagnostic diagnostic = PdfGraphicsStateDiagnostic::None;
    QByteArray operatorName;
    qsizetype offset = 0;
    int operationIndex = -1;
    QString message;
};

struct PdfGraphicsStateResult {
    PdfGraphicsState state;
    QList<PdfGraphicsStateDiagnosticEntry> diagnostics;
    int genericOpCount = 0;
    int unknownOpCount = 0;
    int textShowCount = 0;

    bool hasDiagnostics() const { return !diagnostics.isEmpty(); }
};

class PdfGraphicsStateTracker {
public:
    PdfGraphicsStateResult execute(const QList<PdfContentOperation> &operations);
};
