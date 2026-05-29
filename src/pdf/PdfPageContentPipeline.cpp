#include "PdfPageContentPipeline.h"

static bool isFatalTokenizerWarning(PdfContentStreamTokenizerWarning w) {
    return w != PdfContentStreamTokenizerWarning::None
        && w != PdfContentStreamTokenizerWarning::DanglingOperands;
}

static bool isFatalInterpreterWarning(PdfContentStreamInterpreterWarning w) {
    return w != PdfContentStreamInterpreterWarning::None
        && w != PdfContentStreamInterpreterWarning::UnknownOperator;
}

bool PdfPageContentPipelineResult::ok() const {
    return resolverWarning == PdfPageContentStreamResolverWarning::None
        && !isFatalTokenizerWarning(tokenizerWarning)
        && !isFatalInterpreterWarning(interpreterWarning);
}

QString PdfPageContentPipelineResult::diagnosticSummary() const {
    QStringList parts;
    if (resolverWarning != PdfPageContentStreamResolverWarning::None)
        parts.append(QStringLiteral("Resolver: %1").arg(resolverDiagnostic));
    if (tokenizerWarning != PdfContentStreamTokenizerWarning::None)
        parts.append(QStringLiteral("Tokenizer: %1").arg(tokenizerDiagnostic));
    if (interpreterWarning != PdfContentStreamInterpreterWarning::None)
        parts.append(QStringLiteral("Interpreter: %1").arg(interpreterDiagnostic));
    int gd = graphicsStateResult.diagnostics.size();
    int pd = pathResult.diagnostics.size();
    int td = textRunResult.diagnostics.size();
    if (gd || pd || td)
        parts.append(QStringLiteral("Graphics:%1 Path:%2 Text:%3 diagnostics").arg(gd).arg(pd).arg(td));
    return parts.join(QStringLiteral("; "));
}

PdfPageContentPipeline::PdfPageContentPipeline(PdfStreamDecoder &decoder)
    : m_decoder(decoder) {}

PdfPageContentPipelineResult PdfPageContentPipeline::analyze(
    const PdfDocument &document, int pageIndex) {
    PdfPageContentPipelineResult result;
    result.pageIndex = pageIndex;

    // 1. Resolver
    PdfPageContentStreamResolver resolver(document.store, document.streamStore, m_decoder);
    auto cr = resolver.resolve(document.pageTree, pageIndex);
    result.resolverWarning = cr.warning;
    result.decoderWarning = cr.decoderWarning;
    result.resolverDiagnostic = cr.diagnosticMessage;
    result.pageRef = cr.pageRef;
    if (!cr.ok()) return result;
    result.contentBytes = std::move(cr.contentBytes);

    // 2. Tokenizer
    PdfContentStreamTokenizer tokenizer;
    auto tokResult = tokenizer.parse(result.contentBytes);
    result.tokenizedOperators = tokResult.operators;
    result.tokenizerWarning = tokResult.warning;
    result.danglingOperands = tokResult.danglingOperands;
    result.tokenizerDiagnostic = tokResult.diagnosticMessage;

    // 3. Interpreter
    PdfContentStreamInterpreter interpreter;
    auto intResult = interpreter.interpret(result.tokenizedOperators);
    result.semanticOperations = intResult.operations;
    result.interpreterWarning = intResult.warning;
    result.hasUnknownOperators = intResult.hasUnknownOperators;
    result.interpreterDiagnostic = intResult.diagnosticMessage;

    // 4-6: Recorders
    PdfGraphicsStateTracker gst;
    result.graphicsStateResult = gst.execute(result.semanticOperations);

    PdfPathRecorder pr;
    result.pathResult = pr.record(result.semanticOperations);

    PdfTextRunRecorder trr;
    result.textRunResult = trr.record(result.semanticOperations);

    return result;
}
