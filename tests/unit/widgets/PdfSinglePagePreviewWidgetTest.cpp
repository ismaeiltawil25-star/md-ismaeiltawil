#include <QtTest/QTest>
#include "PdfSinglePagePreviewWidget.h"

class PdfSinglePagePreviewWidgetTest : public QObject {
    Q_OBJECT

private slots:
    void testSetImage();
    void testClear();
    void testClearKeepsZoom();
    void testSetStatusText();
    void testZoomFactor();
    void testZoomFactorClampedLow();
    void testZoomFactorClampedHigh();
    void testZoomFactorNaN();
    void testZoomFactorInf();
    void testSizeHintWithImage();
    void testSizeHintEmpty();
    void testSizeHintChangesWithImageReplacement();
    void testSizeHintChangesWithZoom();
    void testHighDpiLogicalSize();
    void testSizeHintTinyImageDoesNotBecomeZero();
    void testPaintDoesNotCrashWithImage();
    void testPaintDoesNotCrashWithoutImage();
    void testPaintDoesNotCrashWithStatusText();
};

void PdfSinglePagePreviewWidgetTest::testSetImage() {
    PdfSinglePagePreviewWidget w;
    QImage img(100, 100, QImage::Format_ARGB32);
    w.setImage(img);
    QCOMPARE(w.image().size(), QSize(100, 100));
}
void PdfSinglePagePreviewWidgetTest::testClear() {
    PdfSinglePagePreviewWidget w;
    w.setImage(QImage(100, 100, QImage::Format_ARGB32));
    w.setStatusText("test");
    w.clear();
    QVERIFY(w.image().isNull());
    QVERIFY(w.statusText().isEmpty());
}
void PdfSinglePagePreviewWidgetTest::testClearKeepsZoom() {
    PdfSinglePagePreviewWidget w;
    w.setZoomFactor(2.5);
    w.clear();
    QCOMPARE(w.zoomFactor(), 2.5);
}
void PdfSinglePagePreviewWidgetTest::testSetStatusText() {
    PdfSinglePagePreviewWidget w;
    w.setStatusText("hello");
    QCOMPARE(w.statusText(), QString("hello"));
}
void PdfSinglePagePreviewWidgetTest::testZoomFactor() {
    PdfSinglePagePreviewWidget w;
    w.setZoomFactor(2.0);
    QCOMPARE(w.zoomFactor(), 2.0);
}
void PdfSinglePagePreviewWidgetTest::testZoomFactorClampedLow() {
    PdfSinglePagePreviewWidget w;
    w.setZoomFactor(-1.0);
    QCOMPARE(w.zoomFactor(), PdfSinglePagePreviewWidget::MinZoom);
}
void PdfSinglePagePreviewWidgetTest::testZoomFactorClampedHigh() {
    PdfSinglePagePreviewWidget w;
    w.setZoomFactor(100.0);
    QCOMPARE(w.zoomFactor(), PdfSinglePagePreviewWidget::MaxZoom);
}
void PdfSinglePagePreviewWidgetTest::testZoomFactorNaN() {
    PdfSinglePagePreviewWidget w;
    w.setZoomFactor(std::numeric_limits<double>::quiet_NaN());
    QCOMPARE(w.zoomFactor(), PdfSinglePagePreviewWidget::MinZoom);
}
void PdfSinglePagePreviewWidgetTest::testZoomFactorInf() {
    PdfSinglePagePreviewWidget w;
    w.setZoomFactor(std::numeric_limits<double>::infinity());
    QCOMPARE(w.zoomFactor(), PdfSinglePagePreviewWidget::MinZoom);
}
void PdfSinglePagePreviewWidgetTest::testSizeHintWithImage() {
    PdfSinglePagePreviewWidget w;
    QImage img(200, 100, QImage::Format_ARGB32);
    w.setImage(img);
    QSize sh = w.sizeHint();
    QCOMPARE(sh.width(), 200);
    QCOMPARE(sh.height(), 100);
}
void PdfSinglePagePreviewWidgetTest::testSizeHintEmpty() {
    PdfSinglePagePreviewWidget w;
    QCOMPARE(w.sizeHint(), PdfSinglePagePreviewWidget::minimumPreviewSize());
}
void PdfSinglePagePreviewWidgetTest::testSizeHintChangesWithImageReplacement() {
    PdfSinglePagePreviewWidget w;
    w.setImage(QImage(200, 100, QImage::Format_ARGB32));
    QSize s1 = w.sizeHint();
    w.setImage(QImage(400, 300, QImage::Format_ARGB32));
    QSize s2 = w.sizeHint();
    QVERIFY(s1 != s2);
}
void PdfSinglePagePreviewWidgetTest::testSizeHintChangesWithZoom() {
    PdfSinglePagePreviewWidget w;
    w.setImage(QImage(100, 100, QImage::Format_ARGB32));
    QSize s1 = w.sizeHint();
    w.setZoomFactor(2.0);
    QSize s2 = w.sizeHint();
    QVERIFY(s1 != s2);
}
void PdfSinglePagePreviewWidgetTest::testHighDpiLogicalSize() {
    PdfSinglePagePreviewWidget w;
    QImage img(200, 100, QImage::Format_ARGB32);
    img.setDevicePixelRatio(2.0);
    w.setImage(img);
    QSize sh = w.sizeHint();
    QCOMPARE(sh.width(), 100);
    QCOMPARE(sh.height(), 50);
}
void PdfSinglePagePreviewWidgetTest::testSizeHintTinyImageDoesNotBecomeZero() {
    PdfSinglePagePreviewWidget w;
    QImage img(1, 1, QImage::Format_ARGB32);
    w.setImage(img);
    w.setZoomFactor(PdfSinglePagePreviewWidget::MinZoom);
    QSize sh = w.sizeHint();
    QVERIFY(sh.width() >= 1);
    QVERIFY(sh.height() >= 1);
}
void PdfSinglePagePreviewWidgetTest::testPaintDoesNotCrashWithImage() {
    PdfSinglePagePreviewWidget w;
    w.setImage(QImage(100, 100, QImage::Format_ARGB32));
    w.resize(300, 300);
    w.show();
    QTest::qWait(10);
}
void PdfSinglePagePreviewWidgetTest::testPaintDoesNotCrashWithoutImage() {
    PdfSinglePagePreviewWidget w;
    w.resize(300, 300);
    w.show();
    QTest::qWait(10);
}
void PdfSinglePagePreviewWidgetTest::testPaintDoesNotCrashWithStatusText() {
    PdfSinglePagePreviewWidget w;
    w.setStatusText("No image loaded");
    w.resize(300, 300);
    w.show();
    QTest::qWait(10);
}

QTEST_MAIN(PdfSinglePagePreviewWidgetTest)
#include "PdfSinglePagePreviewWidgetTest.moc"
