#include <QtTest/QTest>
#include <QtZlib/zlib.h>
#include "PdfPageContentPipeline.h"
#include "PdfDocumentLoader.h"

static QByteArray zlibCompress(const QByteArray &input) {
    uLongf bound = compressBound(static_cast<uLongf>(input.size()));
    QByteArray compressed(static_cast<qsizetype>(bound), '\0');
    uLongf destLen = bound;
    int ret = compress2(reinterpret_cast<Bytef*>(compressed.data()), &destLen,
        reinterpret_cast<const Bytef*>(input.constData()), static_cast<uLongf>(input.size()),
        Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK) return {};
    compressed.resize(static_cast<qsizetype>(destLen));
    return compressed;
}

static PdfDocument buildDocWithContent(const QByteArray &contentStream, bool flateEncode = false) {
    QByteArray streamBody = flateEncode ? zlibCompress(contentStream) : contentStream;
    QByteArray lengthStr = QByteArray::number(streamBody.size());
    QByteArray filterLine;
    if (flateEncode) filterLine = " /Filter /FlateDecode";

    QByteArray pdf;
    pdf += "%PDF-1.7\n";
    pdf += "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    pdf += "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n";
    pdf += "3 0 obj\n<< /Type /Page /Parent 2 0 R /Contents 4 0 R >>\nendobj\n";
    pdf += "4 0 obj\n<< /Length " + lengthStr + filterLine + " >>\nstream\n" + streamBody + "\nendstream\nendobj\n";

    qsizetype xrefOffset = pdf.size();
    pdf += "xref\n0 5\n0000000000 65535 f \n";
    auto compute = [&](int n) {
        qsizetype p = pdf.indexOf(QByteArray::number(n) + " 0 obj");
        return QString("%1").arg(p, 10, 10, QLatin1Char('0')).toUtf8();
    };
    for (int i = 1; i <= 4; ++i) pdf += compute(i) + " 00000 n\n";
    pdf += "trailer\n<< /Size 5 /Root 1 0 R >>\n";
    pdf += "startxref\n" + QByteArray::number(xrefOffset) + "\n%%EOF\n";
    return PdfDocumentLoader().load(pdf).document;
}

static PdfDocument buildDocWithContentsArray(const QByteArray &part1, const QByteArray &part2) {
    QByteArray stream1 = part1;
    QByteArray stream2 = part2;
    QByteArray len1 = QByteArray::number(stream1.size());
    QByteArray len2 = QByteArray::number(stream2.size());

    QByteArray pdf;
    pdf += "%PDF-1.7\n";
    pdf += "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    pdf += "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n";
    pdf += "3 0 obj\n<< /Type /Page /Parent 2 0 R /Contents [4 0 R 5 0 R] >>\nendobj\n";
    pdf += "4 0 obj\n<< /Length " + len1 + " >>\nstream\n" + stream1 + "\nendstream\nendobj\n";
    pdf += "5 0 obj\n<< /Length " + len2 + " >>\nstream\n" + stream2 + "\nendstream\nendobj\n";

    qsizetype xrefOffset = pdf.size();
    pdf += "xref\n0 6\n0000000000 65535 f \n";
    auto compute = [&](int n) {
        qsizetype p = pdf.indexOf(QByteArray::number(n) + " 0 obj");
        return QString("%1").arg(p, 10, 10, QLatin1Char('0')).toUtf8();
    };
    for (int i = 1; i <= 5; ++i) pdf += compute(i) + " 00000 n\n";
    pdf += "trailer\n<< /Size 6 /Root 1 0 R >>\n";
    pdf += "startxref\n" + QByteArray::number(xrefOffset) + "\n%%EOF\n";
    return PdfDocumentLoader().load(pdf).document;
}

class PdfPageContentPipelineTest : public QObject { Q_OBJECT
private slots:
    void testAnalyzeBlankPage();
    void testAnalyzeIdentityTextContent();
    void testAnalyzePathContent();
    void testAnalyzeMixedTextAndPath();
    void testAnalyzeFlateDecodeContent();
    void testAnalyzeInvalidPageIndex();
    void testAnalyzeUnsupportedFilter();
    void testAnalyzeMalformedContent();
    void testAnalyzeUnknownOperatorNonFatal();
    void testAnalyzeDanglingOperandsNonFatal();
    void testAnalyzeTokenizerUnsupportedInlineImageFatal();
    void testAnalyzeInterpreterFatalStillRunsRecorders();
    void testAnalyzePathDiagnosticsPreserved();
    void testAnalyzeTextDiagnosticsPreserved();
    void testAnalyzePageIdentityPreserved();
    void testAnalyzeEndToEndFromDocumentLoader();
};

void PdfPageContentPipelineTest::testAnalyzeBlankPage() {
    auto doc = buildDocWithContent("");
    PdfStreamDecoder decoder;
    PdfPageContentPipeline pipeline(decoder);
    auto r = pipeline.analyze(doc, 0);
    QVERIFY(r.ok());
    QVERIFY(r.contentBytes.isEmpty());
    QVERIFY(r.tokenizedOperators.isEmpty());
    QVERIFY(r.semanticOperations.isEmpty());
    QVERIFY(r.pathResult.records.isEmpty());
    QVERIFY(r.textRunResult.runs.isEmpty());
    QCOMPARE(r.pageIndex, 0);
}

void PdfPageContentPipelineTest::testAnalyzeIdentityTextContent() {
    auto doc = buildDocWithContent("BT /F1 12 Tf 72 720 Td (Hello) Tj ET");
    PdfStreamDecoder decoder;
    PdfPageContentPipeline pipeline(decoder);
    auto r = pipeline.analyze(doc, 0);
    QVERIFY(r.ok());
    QCOMPARE(r.semanticOperations.size(), 5);
    QCOMPARE(r.textRunResult.runs.size(), 1);
    QCOMPARE(r.textRunResult.runs[0].textBytes, QByteArray("Hello"));
}

void PdfPageContentPipelineTest::testAnalyzePathContent() {
    auto doc = buildDocWithContent("0 0 m 100 100 l S");
    PdfStreamDecoder decoder;
    PdfPageContentPipeline pipeline(decoder);
    auto r = pipeline.analyze(doc, 0);
    QVERIFY(r.ok());
    QVERIFY(!r.pathResult.records.isEmpty());
    QVERIFY(r.textRunResult.runs.isEmpty());
}

void PdfPageContentPipelineTest::testAnalyzeMixedTextAndPath() {
    auto doc = buildDocWithContent("BT /F1 12 Tf (X) Tj ET 0 0 m 100 100 l S");
    PdfStreamDecoder decoder;
    PdfPageContentPipeline pipeline(decoder);
    auto r = pipeline.analyze(doc, 0);
    QVERIFY(r.ok());
    QCOMPARE(r.textRunResult.runs.size(), 1);
    QVERIFY(!r.pathResult.records.isEmpty());
}

void PdfPageContentPipelineTest::testAnalyzeFlateDecodeContent() {
    auto doc = buildDocWithContent("BT /F1 12 Tf (Hello) Tj ET", true);
    PdfStreamDecoder decoder;
    PdfPageContentPipeline pipeline(decoder);
    auto r = pipeline.analyze(doc, 0);
    QVERIFY(r.ok());
    QCOMPARE(r.textRunResult.runs.size(), 1);
    QCOMPARE(r.textRunResult.runs[0].textBytes, QByteArray("Hello"));
}

void PdfPageContentPipelineTest::testAnalyzeInvalidPageIndex() {
    auto doc = buildDocWithContent("");
    PdfStreamDecoder decoder;
    PdfPageContentPipeline pipeline(decoder);
    {
        auto r = pipeline.analyze(doc, -1);
        QVERIFY(!r.ok());
        QCOMPARE(r.pageIndex, -1);
        QCOMPARE(r.resolverWarning, PdfPageContentStreamResolverWarning::InvalidPageIndex);
    }
    {
        auto r = pipeline.analyze(doc, 99);
        QVERIFY(!r.ok());
        QCOMPARE(r.pageIndex, 99);
        QCOMPARE(r.resolverWarning, PdfPageContentStreamResolverWarning::InvalidPageIndex);
    }
}

void PdfPageContentPipelineTest::testAnalyzeUnsupportedFilter() {
    auto doc = buildDocWithContent("BT ET");
    // Modify the stream in streamStore to use an unsupported filter
    PdfRawStreamObject *rs = const_cast<PdfRawStreamObject*>(doc.streamStore.get(4, 0));
    QVERIFY(rs != nullptr);
    rs->filters = {{"ASCIIHexDecode"}};
    PdfStreamDecoder decoder;
    PdfPageContentPipeline pipeline(decoder);
    auto r = pipeline.analyze(doc, 0);
    QVERIFY(!r.ok());
    QCOMPARE(r.resolverWarning, PdfPageContentStreamResolverWarning::DecodeFailure);
    QCOMPARE(r.decoderWarning, PdfStreamDecoderWarning::UnsupportedFilter);
    QVERIFY(r.tokenizedOperators.isEmpty());
}

void PdfPageContentPipelineTest::testAnalyzeMalformedContent() {
    auto doc = buildDocWithContent("BT (Hello");
    PdfStreamDecoder decoder;
    PdfPageContentPipeline pipeline(decoder);
    auto r = pipeline.analyze(doc, 0);
    QVERIFY(!r.ok());
    QCOMPARE(r.tokenizerWarning, PdfContentStreamTokenizerWarning::UnterminatedString);
    QVERIFY(!r.semanticOperations.isEmpty());
    QCOMPARE(r.semanticOperations[0].type, PdfContentOperationType::BeginText);
}

void PdfPageContentPipelineTest::testAnalyzeUnknownOperatorNonFatal() {
    auto doc = buildDocWithContent("XZ BT /F1 12 Tf (Hello) Tj ET");
    PdfStreamDecoder decoder;
    PdfPageContentPipeline pipeline(decoder);
    auto r = pipeline.analyze(doc, 0);
    QVERIFY(r.ok());
    QCOMPARE(r.interpreterWarning, PdfContentStreamInterpreterWarning::UnknownOperator);
    QVERIFY(r.hasUnknownOperators);
    QCOMPARE(r.textRunResult.runs.size(), 1);
}

void PdfPageContentPipelineTest::testAnalyzeDanglingOperandsNonFatal() {
    auto doc = buildDocWithContent("BT /F1 12 Tf 72 720");
    PdfStreamDecoder decoder;
    PdfPageContentPipeline pipeline(decoder);
    auto r = pipeline.analyze(doc, 0);
    QVERIFY(r.ok());
    QCOMPARE(r.tokenizerWarning, PdfContentStreamTokenizerWarning::DanglingOperands);
    QCOMPARE(r.danglingOperands.size(), 2);
    QVERIFY(r.semanticOperations.size() >= 2);
}

void PdfPageContentPipelineTest::testAnalyzeTokenizerUnsupportedInlineImageFatal() {
    auto doc = buildDocWithContent("BT BI /W 10");
    PdfStreamDecoder decoder;
    PdfPageContentPipeline pipeline(decoder);
    auto r = pipeline.analyze(doc, 0);
    QVERIFY(!r.ok());
    QCOMPARE(r.tokenizerWarning, PdfContentStreamTokenizerWarning::UnsupportedInlineImage);
}

void PdfPageContentPipelineTest::testAnalyzeInterpreterFatalStillRunsRecorders() {
    auto doc = buildDocWithContent("BT /F1 12 Tf (Hello) Tj ET 1 0 0 1 0 cm");
    PdfStreamDecoder decoder;
    PdfPageContentPipeline pipeline(decoder);
    auto r = pipeline.analyze(doc, 0);
    QVERIFY(!r.ok());
    QCOMPARE(r.interpreterWarning, PdfContentStreamInterpreterWarning::InvalidOperandCount);
    QCOMPARE(r.semanticOperations.size(), 4);
    QCOMPARE(r.textRunResult.runs.size(), 1);
}

void PdfPageContentPipelineTest::testAnalyzePathDiagnosticsPreserved() {
    auto doc = buildDocWithContent("S");
    PdfStreamDecoder decoder;
    PdfPageContentPipeline pipeline(decoder);
    auto r = pipeline.analyze(doc, 0);
    QVERIFY(r.ok());
    QVERIFY(r.pathResult.hasDiagnostics());
}

void PdfPageContentPipelineTest::testAnalyzeTextDiagnosticsPreserved() {
    auto doc = buildDocWithContent("(Hello) Tj");
    PdfStreamDecoder decoder;
    PdfPageContentPipeline pipeline(decoder);
    auto r = pipeline.analyze(doc, 0);
    QVERIFY(r.ok());
    QVERIFY(r.textRunResult.hasDiagnostics());
}

void PdfPageContentPipelineTest::testAnalyzePageIdentityPreserved() {
    auto doc = buildDocWithContent("BT ET");
    PdfStreamDecoder decoder;
    PdfPageContentPipeline pipeline(decoder);
    auto r = pipeline.analyze(doc, 0);
    QCOMPARE(r.pageIndex, 0);
    QVERIFY(r.pageRef.objNum > 0);
}

void PdfPageContentPipelineTest::testAnalyzeEndToEndFromDocumentLoader() {
    auto doc = buildDocWithContent("BT /F1 12 Tf 72 720 Td (Hello) Tj ET");
    PdfStreamDecoder decoder;
    PdfPageContentPipeline pipeline(decoder);
    auto r = pipeline.analyze(doc, 0);
    QVERIFY(r.ok());
    QCOMPARE(r.contentBytes, QByteArray("BT /F1 12 Tf 72 720 Td (Hello) Tj ET"));
    QCOMPARE(r.semanticOperations.size(), 5);
    QCOMPARE(r.textRunResult.runs.size(), 1);
    QCOMPARE(r.textRunResult.runs[0].textBytes, QByteArray("Hello"));
}

QTEST_MAIN(PdfPageContentPipelineTest)
#include "PdfPageContentPipelineTest.moc"
