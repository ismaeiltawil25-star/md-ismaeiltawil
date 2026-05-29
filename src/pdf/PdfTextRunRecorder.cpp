#include "PdfTextRunRecorder.h"

using T = PdfContentOperationType;
using D = PdfTextRunDiagnostic;

static void addDiag(PdfTextRunResult &r, D d, const PdfContentOperation &op, const QString &msg) {
    PdfTextRunDiagnosticEntry e;
    e.diagnostic = d; e.operatorName = op.operatorName;
    e.offset = op.offset; e.operationIndex = r.runs.size(); e.message = msg;
    r.diagnostics.append(e);
}

static void snapRun(PdfTextRun &run, const PdfTextRunState &s) {
    run.fontName = s.fontName; run.fontSize = s.fontSize;
    run.leading = s.leading; run.wordSpacing = s.wordSpacing; run.charSpacing = s.charSpacing;
    run.ctmSnapshot = s.ctm;
    run.textMatrixSnapshot = s.textMatrix; run.lineMatrixSnapshot = s.lineMatrix;
}

PdfTextRunResult PdfTextRunRecorder::record(const QList<PdfContentOperation> &operations) {
    PdfTextRunResult result;
    PdfTextRunState &s = result.finalState;
    QList<PdfTextRunState> stack;

    auto push = [&] { stack.append(s); };
    auto pop = [&] { if (!stack.isEmpty()) s = stack.takeLast(); };

    for (int i = 0; i < operations.size(); ++i) {
        const auto &op = operations[i];

        switch (op.type) {
        case T::ConcatMatrix:
            s.ctm = s.ctm * PdfMatrix::fromCm(op.numeric[0],op.numeric[1],op.numeric[2],
                                               op.numeric[3],op.numeric[4],op.numeric[5]);
            break;
        case T::PushState: push(); break;
        case T::PopState: pop(); break;

        case T::BeginText:
            if (s.textObject) { addDiag(result, D::NestedBT, op, QStringLiteral("BT inside BT")); }
            else { s.textObject = true; s.textMatrix = PdfMatrix::identity(); s.lineMatrix = PdfMatrix::identity(); }
            break;
        case T::EndText:
            if (!s.textObject) { addDiag(result, D::ETWithoutBT, op, QStringLiteral("ET outside BT")); }
            else { s.textObject = false; }
            break;

        case T::SetFont:
            s.fontName = op.nameOrString; s.fontSize = op.fontSize; s.hasFont = true;
            break;

        case T::MoveTextPosition:
            if (!s.textObject) { addDiag(result, D::TextPositionOutsideBT, op, QStringLiteral("Td outside BT")); }
            else { s.textMatrix = s.lineMatrix * PdfMatrix::translation(op.numeric[0],op.numeric[1]);
                   s.lineMatrix = s.textMatrix; }
            break;
        case T::MoveTextPositionSetLeading:
            if (!s.textObject) { addDiag(result, D::TextPositionOutsideBT, op, QStringLiteral("TD outside BT")); }
            else { s.textMatrix = s.lineMatrix * PdfMatrix::translation(op.numeric[0],op.numeric[1]);
                   s.lineMatrix = s.textMatrix; s.leading = -op.numeric[1]; }
            break;
        case T::SetTextMatrix:
            if (!s.textObject) { addDiag(result, D::TextPositionOutsideBT, op, QStringLiteral("Tm outside BT")); }
            else { s.textMatrix = s.lineMatrix = PdfMatrix::fromCm(
                       op.numeric[0],op.numeric[1],op.numeric[2],op.numeric[3],op.numeric[4],op.numeric[5]); }
            break;
        case T::NextLine:
            if (!s.textObject) { addDiag(result, D::TextPositionOutsideBT, op, QStringLiteral("T* outside BT")); }
            else { s.textMatrix = s.lineMatrix * PdfMatrix::translation(0, -s.leading);
                   s.lineMatrix = s.textMatrix; }
            break;

        case T::ShowText:
        case T::ShowTextWithOffsets:
        case T::NextLineShowText:
        case T::NextLineShowTextWithSpacing:
        {
            // Validate context
            if (!s.textObject) {
                addDiag(result, D::TextShowOutsideBT, op, QStringLiteral("Text show outside BT"));
                break;
            }
            if (!s.hasFont) {
                addDiag(result, D::NoFontSelected, op, QStringLiteral("No font selected"));
                break;
            }

            // Apply pre-show behavior
            if (op.type == T::NextLineShowText) {
                s.textMatrix = s.lineMatrix * PdfMatrix::translation(0, -s.leading);
                s.lineMatrix = s.textMatrix;
            } else if (op.type == T::NextLineShowTextWithSpacing) {
                s.wordSpacing = op.numeric[0];
                s.charSpacing = op.numeric[1];
                s.textMatrix = s.lineMatrix * PdfMatrix::translation(0, -s.leading);
                s.lineMatrix = s.textMatrix;
            }

            // Build run
            PdfTextRun run;
            run.offset = op.offset; run.length = op.length;
            if (op.type == T::ShowTextWithOffsets) {
                run.runType = PdfTextRunType::TJ;
                bool anyValid = false;
                for (const auto &elem : op.operands[0].arrayElements) {
                    if (elem.type == PdfContentOperandType::String) {
                        PdfTJElement e; e.isString = true; e.stringBytes = elem.bytes;
                        e.offset = elem.offset; e.length = elem.length;
                        run.tjElements.append(e); anyValid = true;
                    } else if (elem.type == PdfContentOperandType::Integer ||
                               elem.type == PdfContentOperandType::Real) {
                        PdfTJElement e; e.isString = false;
                        e.numericOffset = elem.type == PdfContentOperandType::Integer
                            ? static_cast<double>(elem.intValue) : elem.realValue;
                        e.offset = elem.offset; e.length = elem.length;
                        run.tjElements.append(e); anyValid = true;
                    } else {
                        addDiag(result, D::InvalidTJElement, op,
                                 QStringLiteral("Invalid TJ element type"));
                    }
                }
                if (!anyValid) break;
            } else {
                if (op.type == T::ShowText) run.runType = PdfTextRunType::Tj;
                else if (op.type == T::NextLineShowText) run.runType = PdfTextRunType::Quote;
                else run.runType = PdfTextRunType::DblQuote;
                run.textBytes = op.nameOrString;
            }
            snapRun(run, s);
            result.runs.append(run);
            break;
        }
        default: break;
        }
    }
    return result;
}
