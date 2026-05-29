#include <QtTest/QTest>
#include "PdfBasicPathRenderer.h"

static int countNonBg(const QImage &img, QColor bg = Qt::white) {
    int n = 0; QRgb bgRgb = bg.rgb();
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            if (img.pixel(x, y) != bgRgb) ++n;
    return n;
}

static PdfPageRenderPlan makePlan(const PdfPageBox &visibleBox, int rotation = 0,
                                   QList<PdfPathRecord> paths = {},
                                   QList<PdfTextRun> textRuns = {}) {
    PdfPageRenderPlan p;
    p.pageIndex = 0; p.pageRef = PdfRef{3, 0};
    p.geometry.visibleBox = visibleBox;
    p.geometry.mediaBox = visibleBox;
    p.geometry.cropBox = visibleBox;
    p.geometry.rotation = rotation;
    p.paths = paths; p.textRuns = textRuns;
    p.hasContent = !paths.isEmpty() || !textRuns.isEmpty();
    return p;
}

static PdfPathSegment moveTo(double x, double y, const PdfMatrix &ctm = {}) {
    PdfPathSegment s; s.type = PdfPathSegmentType::MoveTo; s.x = x; s.y = y; s.ctmSnapshot = ctm; return s;
}
static PdfPathSegment lineTo(double x, double y, const PdfMatrix &ctm = {}) {
    PdfPathSegment s; s.type = PdfPathSegmentType::LineTo; s.x = x; s.y = y; s.ctmSnapshot = ctm; return s;
}
static PdfPathSegment cubic(double x1,double y1,double x2,double y2,double x3,double y3, const PdfMatrix &ctm = {}) {
    PdfPathSegment s; s.type = PdfPathSegmentType::CubicBezier;
    s.x1=x1;s.y1=y1;s.x2=x2;s.y2=y2;s.x3=x3;s.y3=y3; s.ctmSnapshot = ctm; return s;
}
static PdfPathSegment rectSeg(double x,double y,double w,double h, const PdfMatrix &ctm = {}) {
    PdfPathSegment s; s.type = PdfPathSegmentType::Rectangle; s.x=x;s.y=y;s.w=w;s.h=h; s.ctmSnapshot = ctm; return s;
}
static PdfPathRecord rec(QList<PdfPathSegment> segs, PdfPaintMode m) {
    PdfPathRecord r; r.segments = segs; r.paintMode = m; return r;
}

class PdfBasicPathRendererTest : public QObject {
    Q_OBJECT

private slots:
    void testMapPointIdentity();
    void testMapPointTranslation();
    void testMapPointScale();
    void testMapPointRotation0();
    void testMapPointRotation90();
    void testMapPointRotation180();
    void testMapPointRotation270();
    void testBuildPageTransformMatchesMapPoint();
    void testRenderNoPaths();
    void testRenderSimpleLine();
    void testRenderRectangleFill();
    void testRenderCubicBezier();
    void testRenderCloseStroke();
    void testRenderFillEvenOdd();
    void testRenderPaintStateReset();
    void testRenderCropBoxAffectsOutput();
    void testRenderRotation90();
    void testRenderDpiScaling();
    void testRenderPerSegmentCTM();
    void testRenderOutputSizeGuard();
    void testRenderImageOwnsPixels();
    void testRenderTextRunsIgnored();
    void testRenderInvalidDpi();
    void testRenderInvalidMaxDimension();
    void testRenderInvalidGeometry();
    void testRenderInvalidRotation();
    void testRenderNegativeLineWidth();
    void testRenderNegativeRectangle();
    void testRenderTransparentBg();
};

static const int W = 612, H = 792;

void PdfBasicPathRendererTest::testMapPointIdentity() {
    PdfMatrix id;
    auto pt = PdfBasicPathRenderer::mapPdfPoint(100, 200, id, 1.0, 0, W/2.0, H/2.0, W, H);
    QCOMPARE(pt.x(), 100.0); QCOMPARE(pt.y(), 592.0);
}
void PdfBasicPathRendererTest::testMapPointTranslation() {
    PdfMatrix ctm = PdfMatrix::translation(100, 200);
    auto pt = PdfBasicPathRenderer::mapPdfPoint(0, 0, ctm, 1.0, 0, W/2.0, H/2.0, W, H);
    QCOMPARE(pt.x(), 100.0); QCOMPARE(pt.y(), 592.0);
}
void PdfBasicPathRendererTest::testMapPointScale() {
    PdfMatrix ctm = PdfMatrix::fromCm(2, 0, 0, 2, 0, 0);
    auto pt = PdfBasicPathRenderer::mapPdfPoint(50, 50, ctm, 1.0, 0, W/2.0, H/2.0, W, H);
    QCOMPARE(pt.x(), 100.0); QCOMPARE(pt.y(), 692.0);
}
void PdfBasicPathRendererTest::testMapPointRotation0() {
    PdfMatrix id;
    auto pt0 = PdfBasicPathRenderer::mapPdfPoint(0, 0, id, 1.0, 0, W/2.0, H/2.0, W, H);
    auto pt1 = PdfBasicPathRenderer::mapPdfPoint(W, H, id, 1.0, 0, W/2.0, H/2.0, W, H);
    QVERIFY(pt0.x() >= 0 && pt0.y() <= H);
    QVERIFY(pt1.x() <= W && pt1.y() >= 0);
}
void PdfBasicPathRendererTest::testMapPointRotation90() {
    PdfMatrix id;
    // At rotation 90, image is H x W (swapped)
    auto pt = PdfBasicPathRenderer::mapPdfPoint(0, 0, id, 1.0, 90, W/2.0, H/2.0, H, W);
    QVERIFY(pt.x() >= 0 && pt.y() <= W);
}
void PdfBasicPathRendererTest::testMapPointRotation180() {
    PdfMatrix id;
    auto pt = PdfBasicPathRenderer::mapPdfPoint(0, 0, id, 1.0, 180, W/2.0, H/2.0, W, H);
    QVERIFY(pt.x() >= 0 && pt.y() >= 0);
}
void PdfBasicPathRendererTest::testMapPointRotation270() {
    PdfMatrix id;
    auto pt = PdfBasicPathRenderer::mapPdfPoint(0, 0, id, 1.0, 270, W/2.0, H/2.0, H, W);
    QVERIFY(pt.x() >= 0 && pt.y() >= 0);
}
void PdfBasicPathRendererTest::testBuildPageTransformMatchesMapPoint() {
    double scale = 1.0;
    auto qt = PdfBasicPathRenderer::buildPageTransform(scale, 0, W/2.0, H/2.0, W, H);
    PdfMatrix id;
    for (double x = 0; x <= W; x += W) {
        for (double y = 0; y <= H; y += H) {
            auto mp = PdfBasicPathRenderer::mapPdfPoint(x, y, id, scale, 0, W/2.0, H/2.0, W, H);
            // buildPageTransform maps PDF coords directly: apply translate(-cx,-cy), scale(1,-1), translate(iw/2,ih/2)
            auto qtPt = qt.map(QPointF(x, y));
            QVERIFY(std::abs(mp.x() - qtPt.x()) < 0.01);
            QVERIFY(std::abs(mp.y() - qtPt.y()) < 0.01);
        }
    }
}

void PdfBasicPathRendererTest::testRenderNoPaths() {
    PdfPageBox box{0,0,W,H};
    auto plan = makePlan(box);
    PdfBasicPathRenderer r; PdfPathRenderSettings s;
    auto res = r.render(plan, s);
    QCOMPARE(res.warning, PdfPathRenderWarning::NoPaths);
    QVERIFY(res.ok());
    QCOMPARE(res.imageWidth, W); QCOMPARE(res.imageHeight, H);
    QCOMPARE(countNonBg(res.image), 0);
}
void PdfBasicPathRendererTest::testRenderSimpleLine() {
    PdfPageBox box{0,0,W,H}; PdfPathRenderSettings s;
    auto plan = makePlan(box, 0, {rec({moveTo(100,100), lineTo(200,200)}, PdfPaintMode::Stroke)});
    PdfBasicPathRenderer r;
    auto res = r.render(plan, s);
    QCOMPARE(res.warning, PdfPathRenderWarning::None);
    QVERIFY(countNonBg(res.image) > 0);
}
void PdfBasicPathRendererTest::testRenderRectangleFill() {
    PdfPageBox box{0,0,W,H}; PdfPathRenderSettings s;
    auto plan = makePlan(box, 0, {rec({rectSeg(100,100,200,200)}, PdfPaintMode::Fill)});
    auto res = PdfBasicPathRenderer().render(plan, s);
    QCOMPARE(res.warning, PdfPathRenderWarning::None);
    QVERIFY(countNonBg(res.image) > 0);
}
void PdfBasicPathRendererTest::testRenderCubicBezier() {
    PdfPageBox box{0,0,W,H};
    auto plan = makePlan(box, 0, {rec({moveTo(100,100),
        cubic(150,50,250,50,300,100)}, PdfPaintMode::Stroke)});
    auto res = PdfBasicPathRenderer().render(plan, {});
    QCOMPARE(res.warning, PdfPathRenderWarning::None);
    QVERIFY(countNonBg(res.image) > 0);
}
void PdfBasicPathRendererTest::testRenderCloseStroke() {
    PdfPageBox box{0,0,W,H};
    auto plan = makePlan(box, 0, {rec({moveTo(100,100), lineTo(200,100)},
        PdfPaintMode::CloseStroke)});
    auto res = PdfBasicPathRenderer().render(plan, {});
    QCOMPARE(res.warning, PdfPathRenderWarning::None);
    QVERIFY(countNonBg(res.image) > 0);
}
void PdfBasicPathRendererTest::testRenderFillEvenOdd() {
    PdfPageBox box{0,0,W,H};
    auto plan = makePlan(box, 0, {rec({rectSeg(50,50,200,200), rectSeg(100,100,100,100)},
        PdfPaintMode::FillEvenOdd)});
    auto res = PdfBasicPathRenderer().render(plan, {});
    QCOMPARE(res.warning, PdfPathRenderWarning::None);
}

void PdfBasicPathRendererTest::testRenderPaintStateReset() {
    PdfPageBox box{0,0,W,H};
    auto plan = makePlan(box, 0, {
        rec({rectSeg(50,50,100,100)}, PdfPaintMode::Fill),
        rec({moveTo(200,200), lineTo(300,300)}, PdfPaintMode::Stroke)});
    PdfPathRenderSettings s; s.defaultStrokeColor = Qt::red; s.defaultFillColor = Qt::blue;
    auto res = PdfBasicPathRenderer().render(plan, s);
    QCOMPARE(res.warning, PdfPathRenderWarning::None);
    QVERIFY(countNonBg(res.image, Qt::white) > 0);
}
void PdfBasicPathRendererTest::testRenderCropBoxAffectsOutput() {
    PdfPageBox media{0,0,612,792}, crop{50,50,562,742};
    auto plan = makePlan(crop);
    plan.geometry.mediaBox = media; plan.geometry.cropBox = crop; plan.geometry.visibleBox = crop;
    auto res = PdfBasicPathRenderer().render(plan, {});
    QCOMPARE(res.sourceGeometry.llx, 50.0);
    QCOMPARE(res.imageWidth, 512);
    QCOMPARE(res.imageHeight, 692);
}
void PdfBasicPathRendererTest::testRenderRotation90() {
    PdfPageBox box{0,0,W,H};
    auto plan = makePlan(box, 90, {rec({moveTo(100,100), lineTo(200,200)}, PdfPaintMode::Stroke)});
    auto res = PdfBasicPathRenderer().render(plan, {});
    QCOMPARE(res.warning, PdfPathRenderWarning::None);
    QCOMPARE(res.imageWidth, H); QCOMPARE(res.imageHeight, W);
    QCOMPARE(res.rotation, 90);
}
void PdfBasicPathRendererTest::testRenderDpiScaling() {
    PdfPageBox box{0,0,W,H};
    auto plan = makePlan(box, 0, {rec({moveTo(100,100), lineTo(200,200)}, PdfPaintMode::Stroke)});
    PdfPathRenderSettings s; s.dpi = 144;
    auto res = PdfBasicPathRenderer().render(plan, s);
    QCOMPARE(res.imageWidth, W*2); QCOMPARE(res.imageHeight, H*2);
}
void PdfBasicPathRendererTest::testRenderPerSegmentCTM() {
    PdfPageBox box{0,0,W,H};
    PdfPathRecord r; r.paintMode = PdfPaintMode::Stroke;
    r.segments.append(moveTo(0,0)); r.segments.append(lineTo(100,100));
    r.segments.append(moveTo(0,0, PdfMatrix::translation(300, 0)));
    r.segments.append(lineTo(100,100, PdfMatrix::translation(300, 0)));
    auto plan = makePlan(box, 0, {r});
    auto res = PdfBasicPathRenderer().render(plan, {});
    QCOMPARE(res.warning, PdfPathRenderWarning::None);
    QVERIFY(countNonBg(res.image) > 0);
}
void PdfBasicPathRendererTest::testRenderOutputSizeGuard() {
    PdfPageBox box{0,0,W,H};
    PdfPathRenderSettings s; s.dpi = 7200;
    auto plan = makePlan(box);
    auto res = PdfBasicPathRenderer().render(plan, s);
    QCOMPARE(res.warning, PdfPathRenderWarning::ImageTooLarge);
    QVERIFY(!res.ok());
}
void PdfBasicPathRendererTest::testRenderImageOwnsPixels() {
    PdfPageBox box{0,0,W,H};
    auto plan = makePlan(box, 0, {rec({rectSeg(10,10,50,50)}, PdfPaintMode::Fill)});
    auto res = PdfBasicPathRenderer().render(plan, {});
    QVERIFY(!res.image.isNull());
    QVERIFY(res.image.bytesPerLine() > 0);
}
void PdfBasicPathRendererTest::testRenderTextRunsIgnored() {
    PdfPageBox box{0,0,W,H};
    PdfTextRun tr; tr.textBytes = "Hello";
    auto plan = makePlan(box, 0, {}, {tr});
    auto res = PdfBasicPathRenderer().render(plan, {});
    QCOMPARE(res.warning, PdfPathRenderWarning::NoPaths);
}
void PdfBasicPathRendererTest::testRenderInvalidDpi() {
    PdfPageBox box{0,0,W,H}; PdfPathRenderSettings s; s.dpi = 0;
    auto res = PdfBasicPathRenderer().render(makePlan(box), s);
    QCOMPARE(res.warning, PdfPathRenderWarning::InvalidSettings);
}
void PdfBasicPathRendererTest::testRenderInvalidMaxDimension() {
    PdfPageBox box{0,0,W,H}; PdfPathRenderSettings s; s.maxDimension = 0;
    auto res = PdfBasicPathRenderer().render(makePlan(box), s);
    QCOMPARE(res.warning, PdfPathRenderWarning::InvalidSettings);
}
void PdfBasicPathRendererTest::testRenderInvalidGeometry() {
    PdfPageBox box{0,0,0,0};
    auto res = PdfBasicPathRenderer().render(makePlan(box), {});
    QCOMPARE(res.warning, PdfPathRenderWarning::InvalidGeometry);
}
void PdfBasicPathRendererTest::testRenderInvalidRotation() {
    PdfPageBox box{0,0,W,H};
    auto plan = makePlan(box, 45);
    auto res = PdfBasicPathRenderer().render(plan, {});
    QCOMPARE(res.warning, PdfPathRenderWarning::InvalidGeometry);
}
void PdfBasicPathRendererTest::testRenderNegativeLineWidth() {
    PdfPageBox box{0,0,W,H}; PdfPathRenderSettings s; s.defaultLineWidthPx = -1;
    auto res = PdfBasicPathRenderer().render(makePlan(box), s);
    QCOMPARE(res.warning, PdfPathRenderWarning::InvalidSettings);
}
void PdfBasicPathRendererTest::testRenderNegativeRectangle() {
    PdfPageBox box{0,0,W,H};
    auto plan = makePlan(box, 0, {rec({rectSeg(200,200,-100,-100)}, PdfPaintMode::Fill)});
    auto res = PdfBasicPathRenderer().render(plan, {});
    QCOMPARE(res.warning, PdfPathRenderWarning::None);
    QVERIFY(countNonBg(res.image) > 0);
}
void PdfBasicPathRendererTest::testRenderTransparentBg() {
    PdfPageBox box{0,0,W,H}; PdfPathRenderSettings s; s.backgroundColor = Qt::transparent;
    auto res = PdfBasicPathRenderer().render(makePlan(box), s);
    QVERIFY(res.ok());
    QCOMPARE(qAlpha(res.image.pixel(0, 0)), 0);
}

QTEST_MAIN(PdfBasicPathRendererTest)
#include "PdfBasicPathRendererTest.moc"
