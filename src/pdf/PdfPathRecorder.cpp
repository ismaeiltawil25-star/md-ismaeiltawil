#include "PdfPathRecorder.h"

using T = PdfContentOperationType;

static void addDiag(PdfPathResult &r, PdfPathDiagnostic d,
                     const PdfContentOperation &op, const QString &msg) {
    PdfPathDiagnosticEntry e;
    e.diagnostic = d;
    e.operatorName = op.operatorName;
    e.offset = op.offset;
    e.operationIndex = r.records.size();
    e.message = msg;
    r.diagnostics.append(e);
}

static PdfPathSegment makeSeg(PdfPathSegmentType t, const PdfContentOperation &op,
                               const PdfMatrix &ctm) {
    PdfPathSegment s;
    s.type = t;
    s.offset = op.offset;
    s.length = op.length;
    s.ctmSnapshot = ctm;
    return s;
}

static PdfPaintMode paintModeFrom(PdfContentOperationType t) {
    switch (t) {
    case T::Stroke:                  return PdfPaintMode::Stroke;
    case T::CloseStroke:             return PdfPaintMode::CloseStroke;
    case T::Fill:                    return PdfPaintMode::Fill;
    case T::FillEvenOdd:             return PdfPaintMode::FillEvenOdd;
    case T::FillStroke:              return PdfPaintMode::FillStroke;
    case T::FillStrokeEvenOdd:       return PdfPaintMode::FillStrokeEvenOdd;
    case T::CloseFillStroke:         return PdfPaintMode::CloseFillStroke;
    case T::CloseFillStrokeEvenOdd:  return PdfPaintMode::CloseFillStrokeEvenOdd;
    default: return PdfPaintMode::None;
    }
}

static bool isPaint(T type) {
    return type == T::Stroke || type == T::CloseStroke ||
           type == T::Fill || type == T::FillEvenOdd ||
           type == T::FillStroke || type == T::FillStrokeEvenOdd ||
           type == T::CloseFillStroke || type == T::CloseFillStrokeEvenOdd;
}

static bool isImplicitClosePaint(T type) {
    return type == T::CloseStroke || type == T::CloseFillStroke ||
           type == T::CloseFillStrokeEvenOdd;
}

PdfPathResult PdfPathRecorder::record(const QList<PdfContentOperation> &operations) {
    PdfPathResult result;
    PdfMatrix ctm;
    QList<PdfMatrix> ctmStack;

    QList<PdfPathSegment> currentSegments;
    bool hasCurrentPath = false;
    bool hasOpenSubpath = false;

    auto finalize = [&](PdfPaintMode mode, const PdfContentOperation &paintOp) {
        PdfPathRecord rec;
        rec.segments = currentSegments;
        rec.paintMode = mode;
        rec.ctmSnapshot = ctm;
        rec.paintOffset = paintOp.offset;
        rec.paintLength = paintOp.length;
        result.records.append(rec);
        currentSegments.clear();
        hasCurrentPath = false;
        hasOpenSubpath = false;
    };

    for (const auto &op : operations) {
        switch (op.type) {
        case T::ConcatMatrix:
            ctm = ctm * PdfMatrix::fromCm(
                op.numeric[0], op.numeric[1], op.numeric[2],
                op.numeric[3], op.numeric[4], op.numeric[5]);
            break;

        case T::PushState:
            ctmStack.append(ctm);
            break;

        case T::PopState:
            if (!ctmStack.isEmpty()) ctm = ctmStack.takeLast();
            break;

        case T::MoveTo: {
            auto s = makeSeg(PdfPathSegmentType::MoveTo, op, ctm);
            s.x = op.numeric[0]; s.y = op.numeric[1];
            currentSegments.append(s);
            hasCurrentPath = true;
            hasOpenSubpath = true;
            break;
        }
        case T::LineTo: {
            if (!hasOpenSubpath)
                addDiag(result, PdfPathDiagnostic::SegmentWithoutMove, op,
                         QStringLiteral("l without prior m"));
            auto s = makeSeg(PdfPathSegmentType::LineTo, op, ctm);
            s.x = op.numeric[0]; s.y = op.numeric[1];
            currentSegments.append(s);
            hasCurrentPath = true;
            break;
        }
        case T::CurveTo: {
            if (!hasOpenSubpath)
                addDiag(result, PdfPathDiagnostic::SegmentWithoutMove, op,
                         QStringLiteral("c without prior m"));
            auto s = makeSeg(PdfPathSegmentType::CubicBezier, op, ctm);
            s.x1 = op.numeric[0]; s.y1 = op.numeric[1];
            s.x2 = op.numeric[2]; s.y2 = op.numeric[3];
            s.x3 = op.numeric[4]; s.y3 = op.numeric[5];
            currentSegments.append(s);
            hasCurrentPath = true;
            break;
        }
        case T::Rectangle: {
            auto s = makeSeg(PdfPathSegmentType::Rectangle, op, ctm);
            s.x = op.numeric[0]; s.y = op.numeric[1];
            s.w = op.numeric[2]; s.h = op.numeric[3];
            currentSegments.append(s);
            hasCurrentPath = true;
            hasOpenSubpath = false;
            break;
        }
        case T::ClosePath:
            if (!hasOpenSubpath) {
                addDiag(result, PdfPathDiagnostic::CloseWithoutSubpath, op,
                         QStringLiteral("h without open subpath"));
            } else {
                currentSegments.append(makeSeg(PdfPathSegmentType::CloseSubpath, op, ctm));
                hasOpenSubpath = false;
            }
            break;

        case T::EndPath:
            currentSegments.clear();
            hasCurrentPath = false;
            hasOpenSubpath = false;
            break;

        default:
            if (isPaint(op.type)) {
                if (!hasCurrentPath) {
                    addDiag(result, PdfPathDiagnostic::PaintWithoutPath, op,
                             QStringLiteral("Paint operator with empty path"));
                } else {
                    if (isImplicitClosePaint(op.type) && hasOpenSubpath) {
                        auto cs = makeSeg(PdfPathSegmentType::CloseSubpath, op, ctm);
                        cs.offset = op.offset; cs.length = op.length;
                        currentSegments.append(cs);
                        hasOpenSubpath = false;
                    }
                    finalize(paintModeFrom(op.type), op);
                }
            }
            break;
        }
    }

    result.finalCtm = ctm;
    return result;
}
