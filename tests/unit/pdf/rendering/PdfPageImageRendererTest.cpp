#include <QtTest/QTest>
#include <QtZlib/zlib.h>
#include "PdfPageImageRenderer.h"
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
    pdf += "%PDF-1.7\n1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    QByteArray pD = "<< /Type /Pages /Kids [3 0 R] /Count 1" + parentExtra + " >>";
    pdf += "2 0 obj\n" + pD + "\nendobj\n";
    QByteArray pgD = "<< /Type /Page /Parent 2 0 R /Contents 4 0 R" + pageExtra + " >>";
    pdf += "3 0 obj\n" + pgD + "\nendobj\n";
    pdf += "4 0 obj\n<< /Length " + lengthStr + filterLine + " >>\nstream\n" + streamBody + "\nendstream\nendobj\n";

    qsizetype xo = pdf.size();
    pdf += "xref\n0 5\n0000000000 65535 f \n";
    auto c = [&](int n) { auto p = pdf.indexOf(QByteArray::number(n) + " 0 obj");
        return QString("%1").arg(p, 10, 10, QLatin1Char('0')).toUtf8() + " 00000 n\n"; };
    pdf += c(1) + c(2) + c(3) + c(4);
    pdf += "trailer\n<< /Size 5 /Root 1 0 R >>\n";
    pdf += "startxref\n" + QByteArray::number(xo) + "\n%%EOF\n";
    return PdfDocumentLoader().load(pdf).document;
}

int countBg(const QImage &img) { int n=0; QRgb bg=Qt::white; for(int y=0;y<img.height();++y)for(int x=0;x<img.width();++x)if(img.pixel(x,y)!=bg)++n;return n;}

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
};

void PdfPageImageRendererTest::testRenderPageBlankPage() {
    auto doc = buildDoc("", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::NoRenderablePaths);
    QVERIFY(res.ok()); QVERIFY(!res.image.isNull());
}
void PdfPageImageRendererTest::testRenderPageSimplePath() {
    auto doc = buildDoc("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::None);
    QCOMPARE(countBg(res.image) > 0, true);
}
void PdfPageImageRendererTest::testRenderPageTextOnly() {
    auto doc = buildDoc("BT /F1 12 Tf (Hello) Tj ET", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::NoRenderablePaths);
    QVERIFY(res.ok());
}
void PdfPageImageRendererTest::testRenderPageMixedTextAndPath() {
    auto doc = buildDoc("BT /F1 12 Tf (X) Tj ET 0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::None);
    QCOMPARE(countBg(res.image) > 0, true);
}
void PdfPageImageRendererTest::testRenderPageInvalidPageIndex() {
    auto doc = buildDoc("", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, -1);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::PlanBuildFailure);
    QVERIFY(!res.ok()); QVERIFY(res.image.isNull());
}
void PdfPageImageRendererTest::testRenderPageMissingMediaBoxFallback() {
    auto doc = buildDoc("0 0 m 100 100 l S");
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::PlanBuildFailure);
    QVERIFY(!res.ok());
    QVERIFY(!res.image.isNull());
}
void PdfPageImageRendererTest::testRenderPageContentPipelineFailure() {
    auto doc = buildDoc("BT ET", false, " /MediaBox [0 0 612 792]");
    auto *rs = const_cast<PdfRawStreamObject*>(doc.streamStore.get(4, 0));
    rs->filters = {{"ASCIIHexDecode"}};
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::PlanBuildFailure);
    QVERIFY(!res.ok());
}
void PdfPageImageRendererTest::testRenderPageContentFailureDoesNotBecomeNoRenderablePaths() {
    auto doc = buildDoc("BT ET", false, " /MediaBox [0 0 612 792]");
    auto *rs = const_cast<PdfRawStreamObject*>(doc.streamStore.get(4, 0));
    rs->filters = {{"ASCIIHexDecode"}};
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::PlanBuildFailure);
    QCOMPARE(res.planResult.ok(), false);
}
void PdfPageImageRendererTest::testRenderPageMissingMediaBoxNotNoRenderablePaths() {
    auto doc = buildDoc("", false, "");
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::PlanBuildFailure);
    QVERIFY(!res.ok());
}
void PdfPageImageRendererTest::testRenderPageHighDpiSizeGuard() {
    auto doc = buildDoc("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]");
    PdfPathRenderSettings s; s.dpi = 7200;
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0, s);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::PathRenderFailure);
    QVERIFY(!res.ok());
}
void PdfPageImageRendererTest::testRenderPageInvalidSettings() {
    auto doc = buildDoc("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]");
    PdfPathRenderSettings s; s.dpi = 0;
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0, s);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::PathRenderFailure);
    QVERIFY(!res.ok());
}
void PdfPageImageRendererTest::testRenderPageImageOwnsPixels() {
    auto doc = buildDoc("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QVERIFY(!res.image.isNull());
    QVERIFY(res.image.bytesPerLine() > 0);
}
void PdfPageImageRendererTest::testRenderPageEndToEndFromDocumentLoader() {
    auto doc = buildDoc("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]");
    PdfStreamDecoder d; PdfPageImageRenderer r(d);
    auto res = r.renderPage(doc, 0);
    QCOMPARE(res.warning, PdfPageImageRenderWarning::None);
    QCOMPARE(countBg(res.image) > 0, true);
}

QTEST_MAIN(PdfPageImageRendererTest)
#include "PdfPageImageRendererTest.moc"
