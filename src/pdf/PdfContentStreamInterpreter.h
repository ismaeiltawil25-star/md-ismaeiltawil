#pragma once

#include "PdfContentStreamTokenizer.h"
#include <QByteArray>
#include <QList>
#include <QString>
#include <cstdint>

enum class PdfContentOperationType : uint8_t {
    PushState,
    PopState,
    ConcatMatrix,
    BeginText,
    EndText,
    SetFont,
    MoveTextPosition,
    MoveTextPositionSetLeading,
    SetTextMatrix,
    NextLine,
    ShowText,
    ShowTextWithOffsets,
    NextLineShowText,
    NextLineShowTextWithSpacing,
    MoveTo,
    LineTo,
    CurveTo,
    Rectangle,
    ClosePath,
    Stroke,
    CloseStroke,
    Fill,
    FillEvenOdd,
    FillStroke,
    FillStrokeEvenOdd,
    CloseFillStroke,
    CloseFillStrokeEvenOdd,
    EndPath,
    Generic,
    Unknown,
};

struct PdfContentOperation {
    PdfContentOperationType type;
    QByteArray operatorName;
    qsizetype offset = 0;
    qsizetype length = 0;

    QList<PdfContentOperand> operands;
    double numeric[6] = {};
    QByteArray nameOrString;
    double fontSize = 0.0;
};

enum class PdfContentStreamInterpreterWarning : uint8_t {
    None,
    InvalidOperandCount,
    InvalidOperandType,
    StateStackUnderflow,
    UnsupportedInlineImage,
    UnknownOperator,
};

struct PdfContentInterpreterResult {
    QList<PdfContentOperation> operations;
    PdfContentStreamInterpreterWarning warning = PdfContentStreamInterpreterWarning::None;
    QString diagnosticMessage;
    bool hasUnknownOperators = false;

    bool ok() const {
        return warning == PdfContentStreamInterpreterWarning::None
            || warning == PdfContentStreamInterpreterWarning::UnknownOperator;
    }
};

class PdfContentStreamInterpreter {
public:
    PdfContentInterpreterResult interpret(const QList<PdfContentOperator> &operators);
};
