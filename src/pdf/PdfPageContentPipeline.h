#pragma once

#include "PdfDocument.h"
#include "PdfPageContentStreamResolver.h"
#include "PdfContentStreamTokenizer.h"
#include "PdfContentStreamInterpreter.h"
#include "PdfGraphicsStateTracker.h"
#include "PdfPathRecorder.h"
#include "PdfTextRunRecorder.h"
#include "PdfStreamDecoder.h"
#include <QByteArray>
#include <QList>
#include <QString>

struct PdfPageContentPipelineResult {
    int pageIndex = -1;
    PdfRef pageRef;

    QByteArray contentBytes;
    PdfPageContentStreamResolverWarning resolverWarning = PdfPageContentStreamResolverWarning::None;
    PdfStreamDecoderWarning decoderWarning = PdfStreamDecoderWarning::None;
    QString resolverDiagnostic;

    QList<PdfContentOperator> tokenizedOperators;
    PdfContentStreamTokenizerWarning tokenizerWarning = PdfContentStreamTokenizerWarning::None;
    QList<PdfContentOperand> danglingOperands;
    QString tokenizerDiagnostic;

    QList<PdfContentOperation> semanticOperations;
    PdfContentStreamInterpreterWarning interpreterWarning = PdfContentStreamInterpreterWarning::None;
    bool hasUnknownOperators = false;
    QString interpreterDiagnostic;

    PdfGraphicsStateResult graphicsStateResult;
    PdfPathResult pathResult;
    PdfTextRunResult textRunResult;

    bool ok() const;
    QString diagnosticSummary() const;
};

class PdfPageContentPipeline {
public:
    explicit PdfPageContentPipeline(PdfStreamDecoder &decoder);
    PdfPageContentPipelineResult analyze(const PdfDocument &document, int pageIndex);

private:
    PdfStreamDecoder &m_decoder;
};
