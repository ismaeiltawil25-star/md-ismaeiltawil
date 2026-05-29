#pragma once

#include "PdfMatrix.h"
#include "PdfContentStreamInterpreter.h"
#include <QByteArray>
#include <QList>
#include <QString>
#include <cstdint>

enum class PdfPathSegmentType : uint8_t {
    MoveTo,
    LineTo,
    CubicBezier,
    Rectangle,
    CloseSubpath,
};

struct PdfPathSegment {
    PdfPathSegmentType type;
    qsizetype offset = 0;
    qsizetype length = 0;
    PdfMatrix ctmSnapshot;

    double x = 0.0, y = 0.0;
    double x1 = 0.0, y1 = 0.0;
    double x2 = 0.0, y2 = 0.0;
    double x3 = 0.0, y3 = 0.0;
    double w = 0.0, h = 0.0;
};

enum class PdfPaintMode : uint8_t {
    None,
    Stroke,
    CloseStroke,
    Fill,
    FillEvenOdd,
    FillStroke,
    FillStrokeEvenOdd,
    CloseFillStroke,
    CloseFillStrokeEvenOdd,
};

struct PdfPathRecord {
    QList<PdfPathSegment> segments;
    PdfPaintMode paintMode = PdfPaintMode::None;
    PdfMatrix ctmSnapshot;
    qsizetype paintOffset = 0;
    qsizetype paintLength = 0;
};

enum class PdfPathDiagnostic : uint8_t {
    None,
    PaintWithoutPath,
    SegmentWithoutMove,
    CloseWithoutSubpath,
};

struct PdfPathDiagnosticEntry {
    PdfPathDiagnostic diagnostic;
    QByteArray operatorName;
    qsizetype offset;
    int operationIndex;
    QString message;
};

struct PdfPathResult {
    QList<PdfPathRecord> records;
    QList<PdfPathDiagnosticEntry> diagnostics;
    PdfMatrix finalCtm;

    bool hasDiagnostics() const { return !diagnostics.isEmpty(); }
};

class PdfPathRecorder {
public:
    PdfPathResult record(const QList<PdfContentOperation> &operations);
};
