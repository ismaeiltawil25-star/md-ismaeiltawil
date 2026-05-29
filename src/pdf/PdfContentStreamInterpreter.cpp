#include "PdfContentStreamInterpreter.h"

using T = PdfContentOperationType;
using W = PdfContentStreamInterpreterWarning;

static bool isNumeric(const PdfContentOperand &op) {
    return op.type == PdfContentOperandType::Integer || op.type == PdfContentOperandType::Real;
}
static double toDouble(const PdfContentOperand &op) {
    return op.type == PdfContentOperandType::Integer ? static_cast<double>(op.intValue) : op.realValue;
}
static bool isName(const PdfContentOperand &op) { return op.type == PdfContentOperandType::Name; }
static bool isString(const PdfContentOperand &op) { return op.type == PdfContentOperandType::String; }
static bool isArray(const PdfContentOperand &op) { return op.type == PdfContentOperandType::Array; }

static PdfContentOperation makeOp(T type, const PdfContentOperator &src) {
    PdfContentOperation op;
    op.type = type;
    op.operatorName = src.name;
    op.offset = src.offset;
    op.length = src.length;
    op.operands = src.operands;
    return op;
}

static bool requireCount(const PdfContentOperator &op, int expected, PdfContentInterpreterResult &r) {
    if (op.operands.size() != expected) {
        r.warning = W::InvalidOperandCount;
        r.diagnosticMessage = QStringLiteral("Operator '%1' at offset %2: expected %3 operand(s), got %4")
            .arg(QString::fromUtf8(op.name)).arg(op.offset).arg(expected).arg(op.operands.size());
        return false;
    }
    return true;
}

// ---- Known generic names ----
static bool isGenericName(const QByteArray &name) {
    static const char *names[] = {
        "G","g","RG","rg","K","k",
        "CS","cs","SC","sc","SCN","scn",
        "w","J","j","M","d","ri","i","gs",
        "W","W*",
        "v","y",
        "Do",
        "MP","DP","BMC","BDC","EMC",
        "d0","d1","sh",
        "BX","EX",
        "ID","EI",
        nullptr
    };
    for (int i = 0; names[i]; ++i)
        if (name == names[i]) return true;
    return false;
}

PdfContentInterpreterResult PdfContentStreamInterpreter::interpret(
    const QList<PdfContentOperator> &operators) {
    PdfContentInterpreterResult result;
    int stateDepth = 0;

    for (const auto &op : operators) {
        const auto &n = op.name;

        // ---- Graphics state ----
        if (n == "q") {
            if (!requireCount(op, 0, result)) return result;
            stateDepth++;
            result.operations.append(makeOp(T::PushState, op));
        } else if (n == "Q") {
            if (!requireCount(op, 0, result)) return result;
            stateDepth--;
            if (stateDepth < 0) {
                result.warning = W::StateStackUnderflow;
                result.diagnosticMessage = QStringLiteral("'Q' at offset %1 without matching 'q'").arg(op.offset);
                return result;
            }
            result.operations.append(makeOp(T::PopState, op));
        }
        // ---- Transform ----
        else if (n == "cm") {
            if (!requireCount(op, 6, result)) return result;
            for (int i = 0; i < 6; ++i) {
                if (!isNumeric(op.operands[i])) {
                    result.warning = W::InvalidOperandType;
                    result.diagnosticMessage = QStringLiteral("'cm' operand %1 at offset %2: expected numeric").arg(i).arg(op.offset);
                    return result;
                }
            }
            auto o = makeOp(T::ConcatMatrix, op);
            for (int i = 0; i < 6; ++i) o.numeric[i] = toDouble(op.operands[i]);
            result.operations.append(o);
        }
        // ---- Text boundaries ----
        else if (n == "BT") {
            if (!requireCount(op, 0, result)) return result;
            result.operations.append(makeOp(T::BeginText, op));
        } else if (n == "ET") {
            if (!requireCount(op, 0, result)) return result;
            result.operations.append(makeOp(T::EndText, op));
        }
        // ---- Text font ----
        else if (n == "Tf") {
            if (!requireCount(op, 2, result)) return result;
            if (!isName(op.operands[0])) {
                result.warning = W::InvalidOperandType;
                result.diagnosticMessage = QStringLiteral("'Tf' operand 0 at offset %1: expected Name").arg(op.offset);
                return result;
            }
            if (!isNumeric(op.operands[1])) {
                result.warning = W::InvalidOperandType;
                result.diagnosticMessage = QStringLiteral("'Tf' operand 1 at offset %1: expected numeric").arg(op.offset);
                return result;
            }
            auto o = makeOp(T::SetFont, op);
            o.nameOrString = op.operands[0].bytes;
            o.fontSize = toDouble(op.operands[1]);
            result.operations.append(o);
        }
        // ---- Text positioning ----
        else if (n == "Td") {
            if (!requireCount(op, 2, result)) return result;
            if (!isNumeric(op.operands[0]) || !isNumeric(op.operands[1])) {
                result.warning = W::InvalidOperandType;
                result.diagnosticMessage = QStringLiteral("'Td' at offset %1: expected numeric operands").arg(op.offset);
                return result;
            }
            auto o = makeOp(T::MoveTextPosition, op);
            o.numeric[0] = toDouble(op.operands[0]);
            o.numeric[1] = toDouble(op.operands[1]);
            result.operations.append(o);
        } else if (n == "TD") {
            if (!requireCount(op, 2, result)) return result;
            if (!isNumeric(op.operands[0]) || !isNumeric(op.operands[1])) {
                result.warning = W::InvalidOperandType;
                result.diagnosticMessage = QStringLiteral("'TD' at offset %1: expected numeric operands").arg(op.offset);
                return result;
            }
            auto o = makeOp(T::MoveTextPositionSetLeading, op);
            o.numeric[0] = toDouble(op.operands[0]);
            o.numeric[1] = toDouble(op.operands[1]);
            result.operations.append(o);
        } else if (n == "Tm") {
            if (!requireCount(op, 6, result)) return result;
            for (int i = 0; i < 6; ++i) {
                if (!isNumeric(op.operands[i])) {
                    result.warning = W::InvalidOperandType;
                    result.diagnosticMessage = QStringLiteral("'Tm' operand %1 at offset %2: expected numeric").arg(i).arg(op.offset);
                    return result;
                }
            }
            auto o = makeOp(T::SetTextMatrix, op);
            for (int i = 0; i < 6; ++i) o.numeric[i] = toDouble(op.operands[i]);
            result.operations.append(o);
        } else if (n == "T*") {
            if (!requireCount(op, 0, result)) return result;
            result.operations.append(makeOp(T::NextLine, op));
        }
        // ---- Text showing ----
        else if (n == "Tj") {
            if (!requireCount(op, 1, result)) return result;
            if (!isString(op.operands[0])) {
                result.warning = W::InvalidOperandType;
                result.diagnosticMessage = QStringLiteral("'Tj' at offset %1: expected String operand").arg(op.offset);
                return result;
            }
            auto o = makeOp(T::ShowText, op);
            o.nameOrString = op.operands[0].bytes;
            result.operations.append(o);
        } else if (n == "TJ") {
            if (!requireCount(op, 1, result)) return result;
            if (!isArray(op.operands[0])) {
                result.warning = W::InvalidOperandType;
                result.diagnosticMessage = QStringLiteral("'TJ' at offset %1: expected Array operand").arg(op.offset);
                return result;
            }
            for (const auto &elem : op.operands[0].arrayElements) {
                auto et = elem.type;
                if (et != PdfContentOperandType::String &&
                    et != PdfContentOperandType::Integer &&
                    et != PdfContentOperandType::Real) {
                    result.warning = W::InvalidOperandType;
                    result.diagnosticMessage = QStringLiteral("'TJ' at offset %1: invalid array element type").arg(op.offset);
                    return result;
                }
            }
            result.operations.append(makeOp(T::ShowTextWithOffsets, op));
        } else if (n == "'") {
            if (!requireCount(op, 1, result)) return result;
            if (!isString(op.operands[0])) {
                result.warning = W::InvalidOperandType;
                result.diagnosticMessage = QStringLiteral("''' at offset %1: expected String operand").arg(op.offset);
                return result;
            }
            auto o = makeOp(T::NextLineShowText, op);
            o.nameOrString = op.operands[0].bytes;
            result.operations.append(o);
        } else if (n == "\"") {
            if (!requireCount(op, 3, result)) return result;
            if (!isNumeric(op.operands[0]) || !isNumeric(op.operands[1]) || !isString(op.operands[2])) {
                result.warning = W::InvalidOperandType;
                result.diagnosticMessage = QStringLiteral("'\"' at offset %1: expected numeric, numeric, String").arg(op.offset);
                return result;
            }
            auto o = makeOp(T::NextLineShowTextWithSpacing, op);
            o.numeric[0] = toDouble(op.operands[0]);
            o.numeric[1] = toDouble(op.operands[1]);
            o.nameOrString = op.operands[2].bytes;
            result.operations.append(o);
        }
        // ---- Path construction ----
        else if (n == "m") {
            if (!requireCount(op, 2, result)) return result;
            if (!isNumeric(op.operands[0]) || !isNumeric(op.operands[1])) {
                result.warning = W::InvalidOperandType;
                result.diagnosticMessage = QStringLiteral("'m' at offset %1: expected numeric operands").arg(op.offset);
                return result;
            }
            auto o = makeOp(T::MoveTo, op);
            o.numeric[0] = toDouble(op.operands[0]); o.numeric[1] = toDouble(op.operands[1]);
            result.operations.append(o);
        } else if (n == "l") {
            if (!requireCount(op, 2, result)) return result;
            if (!isNumeric(op.operands[0]) || !isNumeric(op.operands[1])) {
                result.warning = W::InvalidOperandType;
                result.diagnosticMessage = QStringLiteral("'l' at offset %1: expected numeric operands").arg(op.offset);
                return result;
            }
            auto o = makeOp(T::LineTo, op);
            o.numeric[0] = toDouble(op.operands[0]); o.numeric[1] = toDouble(op.operands[1]);
            result.operations.append(o);
        } else if (n == "c") {
            if (!requireCount(op, 6, result)) return result;
            for (int i = 0; i < 6; ++i) {
                if (!isNumeric(op.operands[i])) {
                    result.warning = W::InvalidOperandType;
                    result.diagnosticMessage = QStringLiteral("'c' operand %1 at offset %2: expected numeric").arg(i).arg(op.offset);
                    return result;
                }
            }
            auto o = makeOp(T::CurveTo, op);
            for (int i = 0; i < 6; ++i) o.numeric[i] = toDouble(op.operands[i]);
            result.operations.append(o);
        } else if (n == "re") {
            if (!requireCount(op, 4, result)) return result;
            for (int i = 0; i < 4; ++i) {
                if (!isNumeric(op.operands[i])) {
                    result.warning = W::InvalidOperandType;
                    result.diagnosticMessage = QStringLiteral("'re' operand %1 at offset %2: expected numeric").arg(i).arg(op.offset);
                    return result;
                }
            }
            auto o = makeOp(T::Rectangle, op);
            for (int i = 0; i < 4; ++i) o.numeric[i] = toDouble(op.operands[i]);
            result.operations.append(o);
        } else if (n == "h") {
            if (!requireCount(op, 0, result)) return result;
            result.operations.append(makeOp(T::ClosePath, op));
        }
        // ---- Path painting ----
        else if (n == "S") { if (!requireCount(op, 0, result)) return result; result.operations.append(makeOp(T::Stroke, op)); }
        else if (n == "s") { if (!requireCount(op, 0, result)) return result; result.operations.append(makeOp(T::CloseStroke, op)); }
        else if (n == "f" || n == "F") { if (!requireCount(op, 0, result)) return result; result.operations.append(makeOp(T::Fill, op)); }
        else if (n == "f*") { if (!requireCount(op, 0, result)) return result; result.operations.append(makeOp(T::FillEvenOdd, op)); }
        else if (n == "B") { if (!requireCount(op, 0, result)) return result; result.operations.append(makeOp(T::FillStroke, op)); }
        else if (n == "B*") { if (!requireCount(op, 0, result)) return result; result.operations.append(makeOp(T::FillStrokeEvenOdd, op)); }
        else if (n == "b") { if (!requireCount(op, 0, result)) return result; result.operations.append(makeOp(T::CloseFillStroke, op)); }
        else if (n == "b*") { if (!requireCount(op, 0, result)) return result; result.operations.append(makeOp(T::CloseFillStrokeEvenOdd, op)); }
        else if (n == "n") { if (!requireCount(op, 0, result)) return result; result.operations.append(makeOp(T::EndPath, op)); }
        // ---- BI unsupported ----
        else if (n == "BI") {
            result.warning = W::UnsupportedInlineImage;
            result.diagnosticMessage = QStringLiteral("'BI' at offset %1: inline images not supported").arg(op.offset);
            return result;
        }
        // ---- Generic (known name, no validation) ----
        else if (isGenericName(n)) {
            result.operations.append(makeOp(T::Generic, op));
        }
        // ---- Unknown ----
        else {
            result.operations.append(makeOp(T::Unknown, op));
            result.hasUnknownOperators = true;
            if (result.warning == W::None)
                result.warning = W::UnknownOperator;
        }
    }

    return result;
}
