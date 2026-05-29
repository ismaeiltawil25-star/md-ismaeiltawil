#pragma once

#include "PdfMatrix.h"
#include "PdfContentStreamInterpreter.h"
#include <QByteArray>
#include <QList>
#include <QSharedPointer>
#include <QString>
#include <cstdint>

struct PdfTJElement {
    bool isString = false;
    QByteArray stringBytes;
    double numericOffset = 0.0;
    qsizetype offset = 0;
    qsizetype length = 0;
};

enum class PdfTextRunType : uint8_t { Tj, TJ, Quote, DblQuote };

struct PdfTextRun {
    PdfTextRunType runType;
    QByteArray textBytes;
    QList<PdfTJElement> tjElements;

    QByteArray fontName;
    double fontSize = 0.0;
    double leading = 0.0;
    double wordSpacing = 0.0;
    double charSpacing = 0.0;

    PdfMatrix ctmSnapshot;
    PdfMatrix textMatrixSnapshot;
    PdfMatrix lineMatrixSnapshot;

    qsizetype offset = 0;
    qsizetype length = 0;
};

struct PdfTextRunState {
    bool textObject = false;
    PdfMatrix ctm;
    PdfMatrix textMatrix;
    PdfMatrix lineMatrix;
    QByteArray fontName;
    double fontSize = 0.0;
    bool hasFont = false;
    double leading = 0.0;
    double wordSpacing = 0.0;
    double charSpacing = 0.0;
};

enum class PdfTextRunDiagnostic : uint8_t {
    None, TextShowOutsideBT, TextPositionOutsideBT,
    NestedBT, ETWithoutBT, NoFontSelected, InvalidTJElement,
};

struct PdfTextRunDiagnosticEntry {
    PdfTextRunDiagnostic diagnostic;
    QByteArray operatorName;
    qsizetype offset;
    int operationIndex;
    QString message;
};

struct PdfTextRunResult {
    QList<PdfTextRun> runs;
    QList<PdfTextRunDiagnosticEntry> diagnostics;
    PdfTextRunState finalState;
    bool hasDiagnostics() const { return !diagnostics.isEmpty(); }
};

class PdfTextRunRecorder {
public:
    PdfTextRunResult record(const QList<PdfContentOperation> &operations);
};
