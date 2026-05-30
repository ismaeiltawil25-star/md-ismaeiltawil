#include <QtTest/QTest>
#include "PdfPageImageRenderer.h"
#include "PdfDocumentTestHelpers.h"
#include "PdfTestHelpers.h"

class PdfPageImageRendererTest : public QObject { Q_OBJECT
private slots:
    void testRenderPageBlankPage();
    void testRenderPageSimplePath();
    void testRenderPageTextOnly();
    void testRenderPageMixedTextAndPath();
    void testRenderPageInvalidPageIndex();
    void testRenderPageMissingMediaBoxFallback();
    void testRenderPageContentPipelineFailure();
    void testRenderPageContentFailureDoesNotBecomeNoRenderablePaths();
    void testRenderPageMissingMediaBoxNotNoRenderablePaths();
    void testRenderPageHighDpiSizeGuard();
    void testRenderPageInvalidSettings();
    void testRenderPageImageOwnsPixels();
    void testRenderPageEndToEndFromDocumentLoader();

    void testRenderPagePathOnlyBehaviorUnchanged();
    void testRenderPageTextDiagnosticsPreserved();
    void testRenderPageNoPathsAllTextSkippedIsNotNone();
    void testRenderPageTextOnlyEndToEnd();
    void testRenderPageTextNonFatalWhenPathsExist();
};

void PdfPageImageRendererTest::testRenderPageBlankPage() {
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::NoRenderablePaths);
    QVERIFY(res.ok()); QVERIFY(!res.image.isNull());
    QCOMPARE(res.textRenderResult.warning, PdfBasicTextRenderWarning::NoTextRuns);
}
void PdfPageImageRendererTest::testRenderPageSimplePath() {
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::None);
    QCOMPARE(PdfTestHelpers::countNonBgPixels(res.image) > 0, true);
}
void PdfPageImageRendererTest::testRenderPageTextOnly() {
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("BT /F1 12 Tf (Hello) Tj ET", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::None);
    QVERIFY(res.ok());
    QVERIFY(!res.image.isNull());
    QVERIFY(res.textRenderResult.runsRendered > 0);
    QVERIFY(PdfTestHelpers::countNonBgPixels(res.image) > 0);
}
void PdfPageImageRendererTest::testRenderPageMixedTextAndPath() {
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("BT /F1 12 Tf (X) Tj ET 0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::None);
    QCOMPARE(PdfTestHelpers::countNonBgPixels(res.image) > 0, true);
    QVERIFY(res.textRenderResult.runsRendered > 0);
}
void PdfPageImageRendererTest::testRenderPageInvalidPageIndex() {
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, -1);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::PlanBuildFailure);
    QVERIFY(!res.ok()); QVERIFY(res.image.isNull());
}
void PdfPageImageRendererTest::testRenderPageMissingMediaBoxFallback() {
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("0 0 m 100 100 l S", false, "");
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::PlanBuildFailure);
    QVERIFY(!res.ok());
    QVERIFY(!res.image.isNull());
}
void PdfPageImageRendererTest::testRenderPageContentPipelineFailure() {
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("BT ET", false, " /MediaBox [0 0 612 792]");
    auto *rs = const_cast<PdfRawStreamObject*>(doc.streamStore.get(4, 0));
    rs->filters = {{"ASCIIHexDecode"}};
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::PlanBuildFailure);
    QVERIFY(!res.ok());
}
void PdfPageImageRendererTest::testRenderPageContentFailureDoesNotBecomeNoRenderablePaths() {
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("BT ET", false, " /MediaBox [0 0 612 792]");
    auto *rs = const_cast<PdfRawStreamObject*>(doc.streamStore.get(4, 0));
    rs->filters = {{"ASCIIHexDecode"}};
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::PlanBuildFailure);
    QCOMPARE(res.planResult.ok(), false);
}
void PdfPageImageRendererTest::testRenderPageMissingMediaBoxNotNoRenderablePaths() {
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("", false, "");
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::PlanBuildFailure);
    QVERIFY(!res.ok());
}
void PdfPageImageRendererTest::testRenderPageHighDpiSizeGuard() {
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]");
    PdfPathRenderSettings s; s.dpi = 7200;
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0, s);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::PathRenderFailure);
    QVERIFY(!res.ok());
}
void PdfPageImageRendererTest::testRenderPageInvalidSettings() {
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]");
    PdfPathRenderSettings s; s.dpi = 0;
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0, s);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::PathRenderFailure);
    QVERIFY(!res.ok());
}
void PdfPageImageRendererTest::testRenderPageImageOwnsPixels() {
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QVERIFY(!res.image.isNull());
    QVERIFY(res.image.bytesPerLine() > 0);
}
void PdfPageImageRendererTest::testRenderPageEndToEndFromDocumentLoader() {
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::None);
    QCOMPARE(PdfTestHelpers::countNonBgPixels(res.image) > 0, true);
}

void PdfPageImageRendererTest::testRenderPagePathOnlyBehaviorUnchanged() {
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::None);
    QCOMPARE(res.pathResult.warning, PdfPathRenderWarning::None);
    QCOMPARE(res.textRenderResult.warning, PdfBasicTextRenderWarning::NoTextRuns);
    QVERIFY(PdfTestHelpers::countNonBgPixels(res.image) > 0);
}

void PdfPageImageRendererTest::testRenderPageTextDiagnosticsPreserved() {
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("BT /F1 12 Tf <FEFF0048> Tj ET", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QCOMPARE(res.textRenderResult.warning, PdfBasicTextRenderWarning::SkippedRun);
    QVERIFY(!res.textRenderResult.diagnostics.isEmpty());
    QVERIFY(!res.image.isNull());
}

void PdfPageImageRendererTest::testRenderPageNoPathsAllTextSkippedIsNotNone() {
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("BT /F1 0 Tf (Hello) Tj ET", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::NoRenderablePaths);
    QCOMPARE(res.textRenderResult.runsRendered, 0);
    QVERIFY(res.ok());
}

void PdfPageImageRendererTest::testRenderPageTextOnlyEndToEnd() {
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("BT /F1 12 Tf (Hello) Tj ET", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::None);
    QVERIFY(res.textRenderResult.runsRendered > 0);
    QVERIFY(PdfTestHelpers::countNonBgPixels(res.image) > 0);
    QVERIFY(res.ok());
}

void PdfPageImageRendererTest::testRenderPageTextNonFatalWhenPathsExist() {
    auto doc = PdfTestHelpers::loadMinimalPdfDocument("0 0 m 100 100 l S BT /F1 12 Tf <FEFF0048> Tj ET",
                         false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::None);
    QCOMPARE(res.textRenderResult.warning, PdfBasicTextRenderWarning::SkippedRun);
    QVERIFY(!res.textRenderResult.diagnostics.isEmpty());
    QVERIFY(PdfTestHelpers::countNonBgPixels(res.image) > 0);
}

QTEST_MAIN(PdfPageImageRendererTest)
#include "PdfPageImageRendererTest.moc"
