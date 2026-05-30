#include <QtTest/QTest>
#include "PdfPageRenderPlanBuilder.h"
#include "PdfDocumentTestHelpers.h"

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
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("", false, " /MediaBox [0 0 612 792]");
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
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("BT /F1 12 Tf (Hello) Tj ET", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder decoder;
    PdfPageRenderPlanBuilder builder(decoder);
    auto r = builder.build(doc, 0);
    QVERIFY(r.ok());
    QVERIFY(r.plan.hasContent);
    QCOMPARE(r.plan.textRuns.size(), 1);
    QCOMPARE(r.plan.textRuns[0].textBytes, QByteArray("Hello"));
}

void PdfPageRenderPlanBuilderTest::testBuildPathPage() {
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder decoder;
    PdfPageRenderPlanBuilder builder(decoder);
    auto r = builder.build(doc, 0);
    QVERIFY(r.ok());
    QVERIFY(r.plan.hasContent);
    QVERIFY(!r.plan.paths.isEmpty());
    QVERIFY(r.plan.textRuns.isEmpty());
}

void PdfPageRenderPlanBuilderTest::testBuildMixedTextAndPath() {
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("BT /F1 12 Tf (X) Tj ET 0 0 m 100 100 l S",
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
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("BT /F1 12 Tf (Hello) Tj ET", true, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder decoder;
    PdfPageRenderPlanBuilder builder(decoder);
    auto r = builder.build(doc, 0);
    QVERIFY(r.ok());
    QCOMPARE(r.plan.textRuns.size(), 1);
}

void PdfPageRenderPlanBuilderTest::testBuildInvalidPageIndex() {
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder decoder;
    PdfPageRenderPlanBuilder builder(decoder);
    auto r = builder.build(doc, -1);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfPageRenderPlanWarning::InvalidPageIndex);
}

void PdfPageRenderPlanBuilderTest::testBuildMissingMediaBoxFallback() {
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("BT /F1 12 Tf (Hello) Tj ET", false, "");
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
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("BT ET", false, " /MediaBox [0 0 612 792]");
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
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("BT /F1 12 Tf (X) Tj ET", false, "",
                         " /MediaBox [0 0 300 400]");
    PdfStreamDecoder decoder;
    PdfPageRenderPlanBuilder builder(decoder);
    auto r = builder.build(doc, 0);
    QVERIFY(r.ok());
    QCOMPARE(r.plan.geometry.mediaBox.width(), 300.0);
    QCOMPARE(r.plan.textRuns.size(), 1);
}

void PdfPageRenderPlanBuilderTest::testBuildPageIdentityConsistent() {
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("BT ET", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder decoder;
    PdfPageRenderPlanBuilder builder(decoder);
    auto r = builder.build(doc, 0);
    QCOMPARE(r.plan.pageRef.objNum, r.geometryResult.geometry.pageRef.objNum);
    QCOMPARE(r.plan.pageRef.objNum, r.contentResult.pageRef.objNum);
}

void PdfPageRenderPlanBuilderTest::testBuildPlanOwnsData() {
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("BT /F1 12 Tf (Hello) Tj ET", false, " /MediaBox [0 0 612 792]");
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
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("BT /F1 12 Tf (Hello) Tj ET", false, " /MediaBox [0 0 612 792]");
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
