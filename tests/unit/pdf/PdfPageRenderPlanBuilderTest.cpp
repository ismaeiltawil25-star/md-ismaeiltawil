#include <QtTest/QTest>
#include <QtZlib/zlib.h>
#include "PdfPageRenderPlanBuilder.h"
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

static PdfDocument buildDoc(const QByteArray &content, bool flateEncode = false,
                             QByteArray pageExtra = "", QByteArray parentExtra = "") {
    QByteArray streamBody = flateEncode ? zlibCompress(content) : content;
    QByteArray lengthStr = QByteArray::number(streamBody.size());
    QByteArray filterLine;
    if (flateEncode) filterLine = " /Filter /FlateDecode";

    QByteArray pdf;
    pdf += "%PDF-1.7\n";
    pdf += "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";

    QByteArray parentDict = "<< /Type /Pages /Kids [3 0 R] /Count 1" + parentExtra + " >>";
    pdf += "2 0 obj\n" + parentDict + "\nendobj\n";

    QByteArray pageDict = "<< /Type /Page /Parent 2 0 R /Contents 4 0 R" + pageExtra + " >>";
    pdf += "3 0 obj\n" + pageDict + "\nendobj\n";
    pdf += "4 0 obj\n<< /Length " + lengthStr + filterLine + " >>\nstream\n" + streamBody + "\nendstream\nendobj\n";

    qsizetype xo = pdf.size();
    pdf += "xref\n0 5\n0000000000 65535 f \n";
    auto compute = [&](int n) {
        qsizetype p = pdf.indexOf(QByteArray::number(n) + " 0 obj");
        return QString("%1").arg(p, 10, 10, QLatin1Char('0')).toUtf8() + " 00000 n\n";
    };
    pdf += compute(1) + compute(2) + compute(3) + compute(4);
    pdf += "trailer\n<< /Size 5 /Root 1 0 R >>\n";
    pdf += "startxref\n" + QByteArray::number(xo) + "\n%%EOF\n";
    return PdfDocumentLoader().load(pdf).document;
}

class PdfPageRenderPlanBuilderTest : public QObject {
    Q_OBJECT

private slots:
    void testBuildBlankPageWithGeometry();
    void testBuildSimpleTextPage();
    void testBuildPathPage();
    void testBuildMixedTextAndPath();
    void testBuildFlateContent();
    void testBuildInvalidPageIndex();
    void testBuildMissingMediaBoxFallback();
    void testBuildContentPipelineFailure();
    void testBuildInheritedMediaBox();
    void testBuildPageIdentityConsistent();
    void testBuildPlanOwnsData();
    void testBuildEndToEndFromDocumentLoader();
};

void PdfPageRenderPlanBuilderTest::testBuildBlankPageWithGeometry() {
    auto doc = buildDoc("", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder decoder;
    PdfPageRenderPlanBuilder builder(decoder);
    auto r = builder.build(doc, 0);
    QVERIFY(r.ok());
    QVERIFY(!r.plan.hasContent);
    QCOMPARE(r.plan.pageIndex, 0);
    QCOMPARE(r.plan.geometry.mediaBox.width(), 612.0);
    QVERIFY(r.plan.paths.isEmpty());
    QVERIFY(r.plan.textRuns.isEmpty());
}

void PdfPageRenderPlanBuilderTest::testBuildSimpleTextPage() {
    auto doc = buildDoc("BT /F1 12 Tf (Hello) Tj ET", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder decoder;
    PdfPageRenderPlanBuilder builder(decoder);
    auto r = builder.build(doc, 0);
    QVERIFY(r.ok());
    QVERIFY(r.plan.hasContent);
    QCOMPARE(r.plan.textRuns.size(), 1);
    QCOMPARE(r.plan.textRuns[0].textBytes, QByteArray("Hello"));
}

void PdfPageRenderPlanBuilderTest::testBuildPathPage() {
    auto doc = buildDoc("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder decoder;
    PdfPageRenderPlanBuilder builder(decoder);
    auto r = builder.build(doc, 0);
    QVERIFY(r.ok());
    QVERIFY(r.plan.hasContent);
    QVERIFY(!r.plan.paths.isEmpty());
    QVERIFY(r.plan.textRuns.isEmpty());
}

void PdfPageRenderPlanBuilderTest::testBuildMixedTextAndPath() {
    auto doc = buildDoc("BT /F1 12 Tf (X) Tj ET 0 0 m 100 100 l S",
                         false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder decoder;
    PdfPageRenderPlanBuilder builder(decoder);
    auto r = builder.build(doc, 0);
    QVERIFY(r.ok());
    QVERIFY(r.plan.hasContent);
    QVERIFY(!r.plan.paths.isEmpty());
    QCOMPARE(r.plan.textRuns.size(), 1);
}

void PdfPageRenderPlanBuilderTest::testBuildFlateContent() {
    auto doc = buildDoc("BT /F1 12 Tf (Hello) Tj ET", true, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder decoder;
    PdfPageRenderPlanBuilder builder(decoder);
    auto r = builder.build(doc, 0);
    QVERIFY(r.ok());
    QCOMPARE(r.plan.textRuns.size(), 1);
}

void PdfPageRenderPlanBuilderTest::testBuildInvalidPageIndex() {
    auto doc = buildDoc("", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder decoder;
    PdfPageRenderPlanBuilder builder(decoder);
    auto r = builder.build(doc, -1);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfPageRenderPlanWarning::InvalidPageIndex);
}

void PdfPageRenderPlanBuilderTest::testBuildMissingMediaBoxFallback() {
    auto doc = buildDoc("BT /F1 12 Tf (Hello) Tj ET");
    PdfStreamDecoder decoder;
    PdfPageRenderPlanBuilder builder(decoder);
    auto r = builder.build(doc, 0);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfPageRenderPlanWarning::GeometryFailure);
    QCOMPARE(r.plan.geometry.mediaBox.width(), 612.0);
    QCOMPARE(r.plan.textRuns.size(), 1);
    QVERIFY(r.plan.hasContent);
}

void PdfPageRenderPlanBuilderTest::testBuildContentPipelineFailure() {
    auto doc = buildDoc("BT ET", false, " /MediaBox [0 0 612 792]");
    // Destroy the stream store entry to force resolver failure
    PdfRawStreamObject *rs = const_cast<PdfRawStreamObject*>(doc.streamStore.get(4, 0));
    QVERIFY(rs != nullptr);
    rs->filters = {{"ASCIIHexDecode"}};
    PdfStreamDecoder decoder;
    PdfPageRenderPlanBuilder builder(decoder);
    auto r = builder.build(doc, 0);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfPageRenderPlanWarning::ContentPipelineFailure);
}

void PdfPageRenderPlanBuilderTest::testBuildInheritedMediaBox() {
    auto doc = buildDoc("BT /F1 12 Tf (X) Tj ET", false, "",
                         " /MediaBox [0 0 300 400]");
    PdfStreamDecoder decoder;
    PdfPageRenderPlanBuilder builder(decoder);
    auto r = builder.build(doc, 0);
    QVERIFY(r.ok());
    QCOMPARE(r.plan.geometry.mediaBox.width(), 300.0);
    QCOMPARE(r.plan.textRuns.size(), 1);
}

void PdfPageRenderPlanBuilderTest::testBuildPageIdentityConsistent() {
    auto doc = buildDoc("BT ET", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder decoder;
    PdfPageRenderPlanBuilder builder(decoder);
    auto r = builder.build(doc, 0);
    QCOMPARE(r.plan.pageRef.objNum, r.geometryResult.geometry.pageRef.objNum);
    QCOMPARE(r.plan.pageRef.objNum, r.contentResult.pageRef.objNum);
}

void PdfPageRenderPlanBuilderTest::testBuildPlanOwnsData() {
    auto doc = buildDoc("BT /F1 12 Tf (Hello) Tj ET", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder decoder;
    PdfPageRenderPlanBuilder builder(decoder);
    auto r = builder.build(doc, 0);
    QVERIFY(r.ok());
    PdfPageRenderPlan planCopy = r.plan;
    r = PdfPageRenderPlanResult{};
    QCOMPARE(planCopy.textRuns.size(), 1);
    QCOMPARE(planCopy.textRuns[0].textBytes, QByteArray("Hello"));
    QCOMPARE(planCopy.geometry.mediaBox.width(), 612.0);
}

void PdfPageRenderPlanBuilderTest::testBuildEndToEndFromDocumentLoader() {
    auto doc = buildDoc("BT /F1 12 Tf (Hello) Tj ET", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder decoder;
    PdfPageRenderPlanBuilder builder(decoder);
    auto r = builder.build(doc, 0);
    QVERIFY(r.ok());
    QCOMPARE(r.plan.pageIndex, 0);
    QCOMPARE(r.plan.geometry.mediaBox.width(), 612.0);
    QCOMPARE(r.plan.textRuns.size(), 1);
}

QTEST_MAIN(PdfPageRenderPlanBuilderTest)
#include "PdfPageRenderPlanBuilderTest.moc"
