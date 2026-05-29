#include <QtTest/QTest>
#include <QPainter>
#include <QImage>
#include <limits>
#include "PdfBasicTextRenderer.h"

static PdfPageRenderPlan makePlan(const PdfPageBox &vb = PdfPageBox{0,0,612,792},
                                   int rotation = 0,
                                   QList<PdfTextRun> runs = {}) {
    PdfPageRenderPlan p; p.geometry.visibleBox = vb; p.geometry.rotation = rotation; p.textRuns = runs; return p;
}
static PdfTextRun makeTj(const QByteArray &bytes, QByteArray font = "F1", double fs = 12,
                          double tx = 72, double ty = 720, const PdfMatrix &ctm = {}) {
    PdfTextRun r; r.runType = PdfTextRunType::Tj; r.textBytes = bytes;
    r.fontName = font; r.fontSize = fs;
    r.textMatrixSnapshot = PdfMatrix::translation(tx, ty);
    r.ctmSnapshot = ctm; return r;
}
static PdfTextRun makeTJ(QList<PdfTJElement> elems, QByteArray font = "F1", double fs = 12,
                          double tx = 72, double ty = 720, const PdfMatrix &ctm = {}) {
    PdfTextRun r; r.runType = PdfTextRunType::TJ; r.tjElements = elems;
    r.fontName = font; r.fontSize = fs;
    r.textMatrixSnapshot = PdfMatrix::translation(tx, ty);
    r.ctmSnapshot = ctm; return r;
}
static PdfTJElement strElem(const QByteArray &s) {
    PdfTJElement e; e.isString = true; e.stringBytes = s; return e;
}
static PdfTJElement numElem(double v) {
    PdfTJElement e; e.isString = false; e.numericOffset = v; return e;
}

class PdfBasicTextRendererTest : public QObject {
    Q_OBJECT
private slots:
    void testRenderNoTextRuns();
    void testRenderSimpleTj();
    void testDecodeAscii();
    void testDecodeLatin1();
    void testDecodeControlBytes();
    void testDecodeUtf16BeBom();
    void testRenderTJWithStrings();
    void testRenderTJNumericOffsetSign();
    void testRenderTJPartialUnsupportedElement();
    void testRenderInvalidFontSize();
    void testRenderInvalidFontSizeNaN();
    void testRenderTextMatrixTranslation();
    void testRenderDpiScalesFontSize();
    void testRenderPainterRestored();
    void testRenderMultipleRuns();
    void testRenderSomeRenderedSomeSkipped();
    void testRenderAllSkipped();
    void testRenderInvalidDpiZero();
    void testRenderInvalidDpiNaN();
    void testRenderInactivePainter();
    void testRenderPerRunDiagCarriesExactWarning();
};

void PdfBasicTextRendererTest::testRenderNoTextRuns() {
    auto plan = makePlan(); QImage img(300,300,QImage::Format_ARGB32); QPainter p(&img);
    PdfBasicTextRenderer r; PdfPathRenderSettings s;
    auto res = r.render(p, plan, s);
    QCOMPARE(res.warning, PdfBasicTextRenderWarning::NoTextRuns);
    QVERIFY(res.ok());
}
void PdfBasicTextRendererTest::testRenderSimpleTj() {
    auto plan = makePlan(PdfPageBox{0,0,612,792}, 0, {makeTj("Hello")});
    QImage img(612,792,QImage::Format_ARGB32); QPainter p(&img);
    PdfBasicTextRenderer r; PdfPathRenderSettings s;
    auto res = r.render(p, plan, s);
    QCOMPARE(res.warning, PdfBasicTextRenderWarning::None);
    QCOMPARE(res.runsRendered, 1);
}
void PdfBasicTextRendererTest::testDecodeAscii() {
    bool ok; QString diag;
    auto s = PdfBasicTextRenderer::decodeTextBytes("Hello", ok, diag);
    QVERIFY(ok); QCOMPARE(s, QString("Hello"));
}
void PdfBasicTextRendererTest::testDecodeLatin1() {
    bool ok; QString diag;
    auto s = PdfBasicTextRenderer::decodeTextBytes("\xE9", ok, diag);
    QVERIFY(ok); QCOMPARE(s, QString::fromLatin1("\xE9"));
}
void PdfBasicTextRendererTest::testDecodeControlBytes() {
    bool ok; QString diag;
    auto s = PdfBasicTextRenderer::decodeTextBytes("\x01Hello\x7F", ok, diag);
    QVERIFY(ok); QCOMPARE(s, QString("?Hello?"));
}
void PdfBasicTextRendererTest::testDecodeUtf16BeBom() {
    bool ok; QString diag;
    auto s = PdfBasicTextRenderer::decodeTextBytes("\xFE\xFF\x00H", ok, diag);
    QVERIFY(!ok); QVERIFY(s.isEmpty()); QVERIFY(!diag.isEmpty());
}
void PdfBasicTextRendererTest::testRenderTJWithStrings() {
    auto plan = makePlan(PdfPageBox{0,0,612,792}, 0,
        {makeTJ({strElem("AB"), strElem("CD")})});
    QImage img(612,792,QImage::Format_ARGB32); QPainter p(&img);
    PdfBasicTextRenderer r; PdfPathRenderSettings s;
    auto res = r.render(p, plan, s);
    QCOMPARE(res.warning, PdfBasicTextRenderWarning::None);
    QCOMPARE(res.runsRendered, 1);
}
void PdfBasicTextRendererTest::testRenderTJNumericOffsetSign() {
    QImage img(612,792,QImage::Format_ARGB32); QPainter p(&img);
    PdfBasicTextRenderer r; PdfPathRenderSettings s;
    // Not testing exact pixels — just that render succeeds with numeric offsets
    auto plan = makePlan(PdfPageBox{0,0,612,792}, 0,
        {makeTJ({strElem("A"), numElem(500), strElem("B")})});
    auto res = r.render(p, plan, s);
    QCOMPARE(res.runsRendered, 1);
}
void PdfBasicTextRendererTest::testRenderTJPartialUnsupportedElement() {
    QByteArray bom; bom += char(0xFE); bom += char(0xFF); bom += "A";
    auto plan = makePlan(PdfPageBox{0,0,612,792}, 0,
        {makeTJ({strElem("Hello"), strElem(bom)})});
    QImage img(612,792,QImage::Format_ARGB32); QPainter p(&img);
    PdfBasicTextRenderer r; PdfPathRenderSettings s;
    auto res = r.render(p, plan, s);
    QCOMPARE(res.warning, PdfBasicTextRenderWarning::SkippedRun);
    QVERIFY(res.ok());
    QCOMPARE(res.runsRendered, 1);
    QCOMPARE(res.runsSkipped, 0);
    QVERIFY(!res.diagnostics.isEmpty());
}
void PdfBasicTextRendererTest::testRenderInvalidFontSize() {
    auto plan = makePlan(PdfPageBox{0,0,612,792}, 0,
        {makeTj("X", "F1", 0.0)});
    QImage img(612,792,QImage::Format_ARGB32); QPainter p(&img);
    PdfBasicTextRenderer r; PdfPathRenderSettings s;
    auto res = r.render(p, plan, s);
    QCOMPARE(res.runsSkipped, 1);
    QCOMPARE(res.diagnostics[0].warning, PdfBasicTextRenderWarning::InvalidFontSize);
}
void PdfBasicTextRendererTest::testRenderInvalidFontSizeNaN() {
    auto run = makeTj("X", "F1", 12.0);
    run.fontSize = std::numeric_limits<double>::quiet_NaN();
    auto plan = makePlan(PdfPageBox{0,0,612,792}, 0, {run});
    QImage img(612,792,QImage::Format_ARGB32); QPainter p(&img);
    PdfBasicTextRenderer r; PdfPathRenderSettings s;
    auto res = r.render(p, plan, s);
    QCOMPARE(res.runsSkipped, 1);
    QCOMPARE(res.diagnostics[0].warning, PdfBasicTextRenderWarning::InvalidFontSize);
}
void PdfBasicTextRendererTest::testRenderTextMatrixTranslation() {
    auto r1 = makeTj("A", "F1", 12, 100, 100);
    auto r2 = makeTj("B", "F1", 12, 300, 500);
    auto plan = makePlan(PdfPageBox{0,0,612,792}, 0, {r1, r2});
    QImage img(612,792,QImage::Format_ARGB32); QPainter p(&img);
    PdfBasicTextRenderer r; PdfPathRenderSettings s;
    auto res = r.render(p, plan, s);
    QCOMPARE(res.runsRendered, 2);
}
void PdfBasicTextRendererTest::testRenderDpiScalesFontSize() {
    auto plan = makePlan(PdfPageBox{0,0,612,792}, 0, {makeTj("A")});
    QImage img(612,792,QImage::Format_ARGB32); QPainter p(&img);
    PdfBasicTextRenderer r;
    PdfPathRenderSettings s; s.dpi = 144;
    auto res = r.render(p, plan, s);
    QCOMPARE(res.runsRendered, 1);
}
void PdfBasicTextRendererTest::testRenderPainterRestored() {
    auto plan = makePlan(PdfPageBox{0,0,612,792}, 0, {makeTj("A")});
    QImage img(612,792,QImage::Format_ARGB32); QPainter p(&img);
    QPen origPen(Qt::red); p.setPen(origPen);
    PdfBasicTextRenderer r; PdfPathRenderSettings s;
    r.render(p, plan, s);
    QCOMPARE(p.pen().color(), Qt::red);
}
void PdfBasicTextRendererTest::testRenderMultipleRuns() {
    auto plan = makePlan(PdfPageBox{0,0,612,792}, 0,
        {makeTj("A"), makeTj("B"), makeTj("C")});
    QImage img(612,792,QImage::Format_ARGB32); QPainter p(&img);
    PdfBasicTextRenderer r; PdfPathRenderSettings s;
    auto res = r.render(p, plan, s);
    QCOMPARE(res.runsRendered, 3);
}
void PdfBasicTextRendererTest::testRenderSomeRenderedSomeSkipped() {
    auto plan = makePlan(PdfPageBox{0,0,612,792}, 0,
        {makeTj("A"), makeTj("X", "F1", 0.0)});
    QImage img(612,792,QImage::Format_ARGB32); QPainter p(&img);
    PdfBasicTextRenderer r; PdfPathRenderSettings s;
    auto res = r.render(p, plan, s);
    QCOMPARE(res.warning, PdfBasicTextRenderWarning::SkippedRun);
    QVERIFY(res.ok());
    QCOMPARE(res.runsRendered, 1);
    QCOMPARE(res.runsSkipped, 1);
}
void PdfBasicTextRendererTest::testRenderAllSkipped() {
    auto plan = makePlan(PdfPageBox{0,0,612,792}, 0,
        {makeTj("X", "F1", 0.0)});
    QImage img(612,792,QImage::Format_ARGB32); QPainter p(&img);
    PdfBasicTextRenderer r; PdfPathRenderSettings s;
    auto res = r.render(p, plan, s);
    QCOMPARE(res.warning, PdfBasicTextRenderWarning::SkippedRun);
    QVERIFY(!res.ok());
    QCOMPARE(res.runsRendered, 0);
}
void PdfBasicTextRendererTest::testRenderInvalidDpiZero() {
    auto plan = makePlan(); QImage img(300,300,QImage::Format_ARGB32); QPainter p(&img);
    PdfBasicTextRenderer r; PdfPathRenderSettings s; s.dpi = 0;
    auto res = r.render(p, plan, s);
    QCOMPARE(res.warning, PdfBasicTextRenderWarning::InvalidSettings);
    QVERIFY(!res.ok());
}
void PdfBasicTextRendererTest::testRenderInvalidDpiNaN() {
    auto plan = makePlan(); QImage img(300,300,QImage::Format_ARGB32); QPainter p(&img);
    PdfBasicTextRenderer r; PdfPathRenderSettings s;
    s.dpi = std::numeric_limits<double>::quiet_NaN();
    auto res = r.render(p, plan, s);
    QCOMPARE(res.warning, PdfBasicTextRenderWarning::InvalidSettings);
}
void PdfBasicTextRendererTest::testRenderInactivePainter() {
    auto plan = makePlan(); QPainter p;
    PdfBasicTextRenderer r; PdfPathRenderSettings s;
    auto res = r.render(p, plan, s);
    QCOMPARE(res.warning, PdfBasicTextRenderWarning::InvalidSettings);
}
void PdfBasicTextRendererTest::testRenderPerRunDiagCarriesExactWarning() {
    QByteArray bom; bom += char(0xFE); bom += char(0xFF); bom += "A";
    auto plan = makePlan(PdfPageBox{0,0,612,792}, 0, {makeTj(bom)});
    QImage img(612,792,QImage::Format_ARGB32); QPainter p(&img);
    PdfBasicTextRenderer r; PdfPathRenderSettings s;
    auto res = r.render(p, plan, s);
    QCOMPARE(res.warning, PdfBasicTextRenderWarning::SkippedRun);
    QCOMPARE(res.diagnostics[0].warning, PdfBasicTextRenderWarning::UnsupportedEncoding);
}

QTEST_MAIN(PdfBasicTextRendererTest)
#include "PdfBasicTextRendererTest.moc"
