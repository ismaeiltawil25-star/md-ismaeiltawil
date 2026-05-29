#include "PdfGraphicsStateTracker.h"

using D = PdfGraphicsStateDiagnostic;

static void addDiag(PdfGraphicsStateResult &r, D d, const PdfContentOperation &op,
                     const QString &msg) {
    PdfGraphicsStateDiagnosticEntry e;
    e.diagnostic = d;
    e.operatorName = op.operatorName;
    e.offset = op.offset;
    e.operationIndex = r.diagnostics.isEmpty() ? 0 : r.diagnostics.size();
    e.message = msg;
    r.diagnostics.append(e);
}

PdfGraphicsStateResult PdfGraphicsStateTracker::execute(
    const QList<PdfContentOperation> &operations) {
    PdfGraphicsStateResult result;
    PdfGraphicsState &s = result.state;
    QList<PdfGraphicsState> stack;

    for (int i = 0; i < operations.size(); ++i) {
        const auto &op = operations[i];

        switch (op.type) {
        // --- Graphics state ---
        case PdfContentOperationType::PushState:
            stack.append(s);
            break;

        case PdfContentOperationType::PopState:
            if (stack.isEmpty()) {
                addDiag(result, D::StateStackUnderflow, op,
                         QStringLiteral("Q without matching q"));
            } else {
                s = stack.takeLast();
            }
            break;

        // --- Transform ---
        case PdfContentOperationType::ConcatMatrix:
            s.ctm = s.ctm * PdfMatrix::fromCm(
                op.numeric[0], op.numeric[1], op.numeric[2],
                op.numeric[3], op.numeric[4], op.numeric[5]);
            break;

        // --- Text boundaries ---
        case PdfContentOperationType::BeginText:
            if (s.textObject) {
                addDiag(result, D::NestedBT, op,
                         QStringLiteral("BT inside active text object"));
            } else {
                s.textObject = true;
                s.textMatrix = PdfMatrix::identity();
                s.lineMatrix = PdfMatrix::identity();
            }
            break;

        case PdfContentOperationType::EndText:
            if (!s.textObject) {
                addDiag(result, D::ETWithoutBT, op,
                         QStringLiteral("ET outside text object"));
            } else {
                s.textObject = false;
            }
            break;

        // --- Text font ---
        case PdfContentOperationType::SetFont:
            s.fontName = op.nameOrString;
            s.fontSize = op.fontSize;
            break;

        // --- Text positioning ---
        case PdfContentOperationType::MoveTextPosition:
            if (!s.textObject) {
                addDiag(result, D::TextPositionOutsideBT, op,
                         QStringLiteral("Td outside text object"));
            } else {
                s.textMatrix = s.lineMatrix * PdfMatrix::translation(op.numeric[0], op.numeric[1]);
                s.lineMatrix = s.textMatrix;
            }
            break;

        case PdfContentOperationType::MoveTextPositionSetLeading:
            if (!s.textObject) {
                addDiag(result, D::TextPositionOutsideBT, op,
                         QStringLiteral("TD outside text object"));
            } else {
                s.textMatrix = s.lineMatrix * PdfMatrix::translation(op.numeric[0], op.numeric[1]);
                s.lineMatrix = s.textMatrix;
                s.leading = -op.numeric[1];
            }
            break;

        case PdfContentOperationType::SetTextMatrix:
            if (!s.textObject) {
                addDiag(result, D::TextPositionOutsideBT, op,
                         QStringLiteral("Tm outside text object"));
            } else {
                s.textMatrix = PdfMatrix::fromCm(
                    op.numeric[0], op.numeric[1], op.numeric[2],
                    op.numeric[3], op.numeric[4], op.numeric[5]);
                s.lineMatrix = s.textMatrix;
            }
            break;

        case PdfContentOperationType::NextLine:
            if (!s.textObject) {
                addDiag(result, D::TextPositionOutsideBT, op,
                         QStringLiteral("T* outside text object"));
            } else {
                s.textMatrix = s.lineMatrix * PdfMatrix::translation(0, -s.leading);
                s.lineMatrix = s.textMatrix;
            }
            break;

        // --- Text showing ---
        case PdfContentOperationType::ShowText:
        case PdfContentOperationType::ShowTextWithOffsets:
            if (!s.textObject) {
                addDiag(result, D::TextShowingOutsideBT, op,
                         QStringLiteral("Text showing outside text object"));
            } else {
                result.textShowCount++;
            }
            break;

        case PdfContentOperationType::NextLineShowText:
            if (!s.textObject) {
                addDiag(result, D::TextShowingOutsideBT, op,
                         QStringLiteral("' outside text object"));
            } else {
                s.textMatrix = s.lineMatrix * PdfMatrix::translation(0, -s.leading);
                s.lineMatrix = s.textMatrix;
                result.textShowCount++;
            }
            break;

        case PdfContentOperationType::NextLineShowTextWithSpacing:
            if (!s.textObject) {
                addDiag(result, D::TextShowingOutsideBT, op,
                         QStringLiteral("\" outside text object"));
            } else {
                s.wordSpacing = op.numeric[0];
                s.charSpacing = op.numeric[1];
                s.textMatrix = s.lineMatrix * PdfMatrix::translation(0, -s.leading);
                s.lineMatrix = s.textMatrix;
                result.textShowCount++;
            }
            break;

        // --- Path operators: ignored ---
        case PdfContentOperationType::MoveTo:
        case PdfContentOperationType::LineTo:
        case PdfContentOperationType::CurveTo:
        case PdfContentOperationType::Rectangle:
        case PdfContentOperationType::ClosePath:
        case PdfContentOperationType::Stroke:
        case PdfContentOperationType::CloseStroke:
        case PdfContentOperationType::Fill:
        case PdfContentOperationType::FillEvenOdd:
        case PdfContentOperationType::FillStroke:
        case PdfContentOperationType::FillStrokeEvenOdd:
        case PdfContentOperationType::CloseFillStroke:
        case PdfContentOperationType::CloseFillStrokeEvenOdd:
        case PdfContentOperationType::EndPath:
            break;

        // --- Generic ---
        case PdfContentOperationType::Generic:
            result.genericOpCount++;
            break;

        // --- Unknown ---
        case PdfContentOperationType::Unknown:
            result.unknownOpCount++;
            break;
        }
    }

    return result;
}
