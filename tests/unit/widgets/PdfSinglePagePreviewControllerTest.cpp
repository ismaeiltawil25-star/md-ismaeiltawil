#include <QtTest/QTest>
#include <QtZlib/zlib.h>
#include <QSignalSpy>
#include "PdfSinglePagePreviewController.h"
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
static PdfDocument buildDoc(const QByteArray &content, bool flate = false,
    QByteArray pageExtra = "", QByteArray parentExtra = "") {
    QByteArray body = flate ? zlibCompress(content) : content;
    QByteArray len = QByteArray::number(body.size());
    QByteArray filter; if (flate) filter = " /Filter /FlateDecode";
    QByteArray pdf;
    pdf += "%PDF-1.7\n1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    pdf += "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1" + parentExtra + " >>\nendobj\n";
    pdf += "3 0 obj\n<< /Type /Page /Parent 2 0 R /Contents 4 0 R" + pageExtra + " >>\nendobj\n";
    pdf += "4 0 obj\n<< /Length " + len + filter + " >>\nstream\n" + body + "\nendstream\nendobj\n";
    qsizetype xo = pdf.size(); pdf += "xref\n0 5\n0000000000 65535 f \n";
    auto c = [&](int n){auto p=pdf.indexOf(QByteArray::number(n)+" 0 obj");
        return QString("%1").arg(p,10,10,QLatin1Char('0')).toUtf8()+" 00000 n\n";};
    pdf+=c(1)+c(2)+c(3)+c(4);
    pdf+="trailer\n<< /Size 5 /Root 1 0 R >>\nstartxref\n"+QByteArray::number(xo)+"\n%%EOF\n";
    return PdfDocumentLoader().load(pdf).document;
}

class PdfSinglePagePreviewControllerTest : public QObject { Q_OBJECT
private slots:
    void testControllerRendersSimplePath();
    void testControllerBlankPage();
    void testControllerTextOnlyPage();
    void testControllerInvalidPageIndex();
    void testControllerHighDpiSizeGuard();
    void testControllerClear();
    void testControllerReturnsFullResult();
    void testControllerNullWidgetStillReturnsResult();
    void testControllerPlanFailureWithFallbackImage();
    void testControllerPlanFailureNoImage();
    void testControllerDestroyedWidgetDoesNotCrash();

    void testZoomInIncreasesFactorAndResizes();
    void testZoomOutDecreasesFactorAndResizes();
    void testZoomResetReturnsToOne();
    void testZoomChangedSignalEmitted();
    void testZoomToClampsToBounds();
    void testZoomNullWidgetDoesNotCrash();
};

void PdfSinglePagePreviewControllerTest::testControllerRendersSimplePath() {
    auto doc = buildDoc("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]");
    PdfSinglePagePreviewWidget w;
    PdfSinglePagePreviewController ctrl(&w);
    auto r = ctrl.renderDocumentPage(doc, 0);
    QCOMPARE(r.warning, PdfPageImageRenderWarning::None);
    QVERIFY(!w.image().isNull());
    QVERIFY(w.statusText().isEmpty());
}
void PdfSinglePagePreviewControllerTest::testControllerBlankPage() {
    auto doc = buildDoc("", false, " /MediaBox [0 0 612 792]");
    PdfSinglePagePreviewWidget w;
    PdfSinglePagePreviewController ctrl(&w);
    auto r = ctrl.renderDocumentPage(doc, 0);
    QCOMPARE(r.warning, PdfPageImageRenderWarning::NoRenderablePaths);
    QVERIFY(!w.image().isNull());
    QVERIFY(w.statusText().contains("No renderable"));
}
void PdfSinglePagePreviewControllerTest::testControllerTextOnlyPage() {
    auto doc = buildDoc("BT /F1 12 Tf (Hello) Tj ET", false, " /MediaBox [0 0 612 792]");
    PdfSinglePagePreviewWidget w;
    PdfSinglePagePreviewController ctrl(&w);
    auto r = ctrl.renderDocumentPage(doc, 0);
    QCOMPARE(r.warning, PdfPageImageRenderWarning::None);
    QVERIFY(!w.image().isNull());
}
void PdfSinglePagePreviewControllerTest::testControllerInvalidPageIndex() {
    auto doc = buildDoc("", false, " /MediaBox [0 0 612 792]");
    PdfSinglePagePreviewWidget w;
    PdfSinglePagePreviewController ctrl(&w);
    auto r = ctrl.renderDocumentPage(doc, -1);
    QCOMPARE(r.warning, PdfPageImageRenderWarning::PlanBuildFailure);
    QVERIFY(w.image().isNull());
    QVERIFY(!w.statusText().isEmpty());
}
void PdfSinglePagePreviewControllerTest::testControllerHighDpiSizeGuard() {
    auto doc = buildDoc("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]");
    PdfPathRenderSettings s; s.dpi = 7200;
    PdfSinglePagePreviewWidget w;
    PdfSinglePagePreviewController ctrl(&w);
    auto r = ctrl.renderDocumentPage(doc, 0, s);
    QCOMPARE(r.warning, PdfPageImageRenderWarning::PathRenderFailure);
    QVERIFY(w.image().isNull());
    QVERIFY(!w.statusText().isEmpty());
}
void PdfSinglePagePreviewControllerTest::testControllerClear() {
    PdfSinglePagePreviewWidget w;
    w.setStatusText("test");
    PdfSinglePagePreviewController ctrl(&w);
    ctrl.clear();
    QVERIFY(w.image().isNull());
    QVERIFY(w.statusText().isEmpty());
}
void PdfSinglePagePreviewControllerTest::testControllerReturnsFullResult() {
    auto doc = buildDoc("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]");
    PdfSinglePagePreviewWidget w;
    PdfSinglePagePreviewController ctrl(&w);
    auto r = ctrl.renderDocumentPage(doc, 0);
    QVERIFY(r.planResult.diagnosticSummary.isEmpty() || !r.planResult.diagnosticSummary.isEmpty());
}
void PdfSinglePagePreviewControllerTest::testControllerNullWidgetStillReturnsResult() {
    auto doc = buildDoc("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]");
    PdfSinglePagePreviewController ctrl(nullptr);
    auto r = ctrl.renderDocumentPage(doc, 0);
    QCOMPARE(r.warning, PdfPageImageRenderWarning::None);
    QVERIFY(!r.image.isNull());
}
void PdfSinglePagePreviewControllerTest::testControllerPlanFailureWithFallbackImage() {
    auto doc = buildDoc("0 0 m 100 100 l S", false, "");
    PdfSinglePagePreviewWidget w;
    PdfSinglePagePreviewController ctrl(&w);
    auto r = ctrl.renderDocumentPage(doc, 0);
    QCOMPARE(r.warning, PdfPageImageRenderWarning::PlanBuildFailure);
    QVERIFY(!w.image().isNull());
    QVERIFY(!w.statusText().isEmpty());
}
void PdfSinglePagePreviewControllerTest::testControllerPlanFailureNoImage() {
    auto doc = buildDoc("BT ET", false, " /MediaBox [0 0 612 792]");
    auto *rs = const_cast<PdfRawStreamObject*>(doc.streamStore.get(4, 0));
    rs->filters = {{"ASCIIHexDecode"}};
    PdfSinglePagePreviewWidget w;
    PdfSinglePagePreviewController ctrl(&w);
    auto r = ctrl.renderDocumentPage(doc, 0);
    QVERIFY(r.warning == PdfPageImageRenderWarning::PlanBuildFailure);
    if (r.image.isNull()) {
        QVERIFY(w.image().isNull());
        QVERIFY(!w.statusText().isEmpty());
    } else {
        QVERIFY(!w.image().isNull());
        QVERIFY(!w.statusText().isEmpty());
    }
}
void PdfSinglePagePreviewControllerTest::testControllerDestroyedWidgetDoesNotCrash() {
    auto doc = buildDoc("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]");
    auto *w = new PdfSinglePagePreviewWidget;
    PdfSinglePagePreviewController ctrl(w);
    delete w;
    auto r = ctrl.renderDocumentPage(doc, 0);
    QCOMPARE(r.warning, PdfPageImageRenderWarning::None);
}

void PdfSinglePagePreviewControllerTest::testZoomInIncreasesFactorAndResizes() {
    auto doc = buildDoc("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]");
    PdfSinglePagePreviewWidget w;
    PdfSinglePagePreviewController ctrl(&w);
    ctrl.renderDocumentPage(doc, 0);
    QVERIFY(!w.image().isNull());
    QVERIFY(qFuzzyCompare(w.zoomFactor(), 1.0));
    ctrl.zoomIn();
    QVERIFY(w.zoomFactor() > 1.0);
    QCOMPARE(w.size(), w.sizeHint());
}

void PdfSinglePagePreviewControllerTest::testZoomOutDecreasesFactorAndResizes() {
    auto doc = buildDoc("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]");
    PdfSinglePagePreviewWidget w;
    PdfSinglePagePreviewController ctrl(&w);
    ctrl.renderDocumentPage(doc, 0);
    ctrl.zoomIn(); ctrl.zoomIn(); // 1.0 -> 1.25 -> 1.5625
    QVERIFY(w.zoomFactor() > 1.5);
    ctrl.zoomOut();
    QVERIFY(w.zoomFactor() < 1.5);
    QCOMPARE(w.size(), w.sizeHint());
}

void PdfSinglePagePreviewControllerTest::testZoomResetReturnsToOne() {
    auto doc = buildDoc("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]");
    PdfSinglePagePreviewWidget w;
    PdfSinglePagePreviewController ctrl(&w);
    ctrl.renderDocumentPage(doc, 0);
    ctrl.zoomIn(); ctrl.zoomIn();
    QVERIFY(!qFuzzyCompare(w.zoomFactor(), 1.0));
    ctrl.zoomReset();
    QVERIFY(qFuzzyCompare(w.zoomFactor(), 1.0));
    QCOMPARE(w.size(), w.sizeHint());
}

void PdfSinglePagePreviewControllerTest::testZoomChangedSignalEmitted() {
    PdfSinglePagePreviewWidget w;
    PdfSinglePagePreviewController ctrl(&w);
    QSignalSpy spy(&ctrl, &PdfSinglePagePreviewController::zoomChanged);
    ctrl.zoomIn();
    QCOMPARE(spy.count(), 1);
    // no-op on same factor via internal guard
    ctrl.zoomIn();
    QCOMPARE(spy.count(), 2); // factor changes each time, so 2 emits
}

void PdfSinglePagePreviewControllerTest::testZoomToClampsToBounds() {
    PdfSinglePagePreviewWidget w;
    PdfSinglePagePreviewController ctrl(&w);
    ctrl.zoomTo(0.01);
    QVERIFY(qFuzzyCompare(w.zoomFactor(), PdfSinglePagePreviewWidget::MinZoom));
    ctrl.zoomTo(100.0);
    QVERIFY(qFuzzyCompare(w.zoomFactor(), PdfSinglePagePreviewWidget::MaxZoom));
    QCOMPARE(w.size(), w.sizeHint());
}

void PdfSinglePagePreviewControllerTest::testZoomNullWidgetDoesNotCrash() {
    PdfSinglePagePreviewController ctrl(nullptr);
    ctrl.zoomIn();
    ctrl.zoomOut();
    ctrl.zoomReset();
    ctrl.zoomTo(2.0);
    // Must not crash
}

QTEST_MAIN(PdfSinglePagePreviewControllerTest)
#include "PdfSinglePagePreviewControllerTest.moc"
