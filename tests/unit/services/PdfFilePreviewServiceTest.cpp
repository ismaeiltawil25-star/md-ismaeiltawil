#include <QtTest/QTest>
#include <QTemporaryDir>
#include "PdfFilePreviewService.h"
#include "PdfTestHelpers.h"

static QTemporaryDir s_pdfDir;
static QString createValidPdf(const QByteArray &content) {
    return PdfTestHelpers::writePdfFile(s_pdfDir, "test.pdf",
        PdfTestHelpers::buildMinimalPdfBytes(content));
}
static QString createBytesFile(const QByteArray &bytes) {
    return PdfTestHelpers::writeBytesFile(s_pdfDir, "test.bin", bytes);
}

class PdfFilePreviewServiceTest : public QObject { Q_OBJECT
private slots:
    void testRenderFileMissingPath();
    void testRenderFileDirectoryPath();
    void testRenderFileEmptyFile();
    void testRenderFileInvalidPdf();
    void testRenderFileBlankPdf();
    void testRenderFileValidPathPdf();
    void testRenderFileTextOnlyPdf();
    void testRenderFileInvalidPageIndex();
    void testRenderFileHighDpiSizeGuard();
    void testRenderFileImageOwnsPixels();
    void testRenderFileDiagnosticsPreserved();
    void testRenderFileEmptyStringPath();
};

void PdfFilePreviewServiceTest::testRenderFileMissingPath() {
    PdfFilePreviewService s;
    auto r = s.renderFilePage("/nonexistent/path/file.pdf");
    QCOMPARE(r.warning, PdfFilePreviewWarning::FileNotFound);
    QVERIFY(r.image.isNull());
}
void PdfFilePreviewServiceTest::testRenderFileDirectoryPath() {
    QTemporaryDir dir;
    PdfFilePreviewService s;
    auto r = s.renderFilePage(dir.path());
    QCOMPARE(r.warning, PdfFilePreviewWarning::FileOpenFailure);
}
void PdfFilePreviewServiceTest::testRenderFileEmptyFile() {
    QString path = createBytesFile(QByteArray{});
    PdfFilePreviewService s;
    auto r = s.renderFilePage(path);
    QCOMPARE(r.warning, PdfFilePreviewWarning::EmptyFile);
}
void PdfFilePreviewServiceTest::testRenderFileInvalidPdf() {
    QString path = createBytesFile("not a pdf");
    PdfFilePreviewService s;
    auto r = s.renderFilePage(path);
    QCOMPARE(r.warning, PdfFilePreviewWarning::DocumentLoadFailure);
    QCOMPARE(r.documentResult.warning, PdfDocumentLoaderWarning::InvalidHeader);
}
void PdfFilePreviewServiceTest::testRenderFileBlankPdf() {
    QString path = createValidPdf("");
    PdfFilePreviewService s;
    auto r = s.renderFilePage(path);
    QCOMPARE(r.warning, PdfFilePreviewWarning::NoRenderablePaths);
    QVERIFY(r.ok()); QVERIFY(!r.image.isNull());
}
void PdfFilePreviewServiceTest::testRenderFileValidPathPdf() {
    QString path = createValidPdf("0 0 m 100 100 l S");
    PdfFilePreviewService s;
    auto r = s.renderFilePage(path);
    QCOMPARE(r.warning, PdfFilePreviewWarning::None);
    QVERIFY(r.ok());
    QVERIFY(!r.image.isNull());
}
void PdfFilePreviewServiceTest::testRenderFileTextOnlyPdf() {
    QString path = createValidPdf("BT /F1 12 Tf (Hello) Tj ET");
    PdfFilePreviewService s;
    auto r = s.renderFilePage(path);
    QCOMPARE(r.warning, PdfFilePreviewWarning::None);
    QVERIFY(r.ok()); QVERIFY(!r.image.isNull());
}
void PdfFilePreviewServiceTest::testRenderFileInvalidPageIndex() {
    QString path = createValidPdf("0 0 m 100 100 l S");
    PdfFilePreviewService s;
    auto r = s.renderFilePage(path, -1);
    QCOMPARE(r.warning, PdfFilePreviewWarning::RenderFailure);
}
void PdfFilePreviewServiceTest::testRenderFileHighDpiSizeGuard() {
    QString path = createValidPdf("0 0 m 100 100 l S");
    PdfPathRenderSettings ss; ss.dpi = 7200;
    PdfFilePreviewService s;
    auto r = s.renderFilePage(path, 0, ss);
    QCOMPARE(r.warning, PdfFilePreviewWarning::RenderFailure);
    QVERIFY(!r.ok());
}
void PdfFilePreviewServiceTest::testRenderFileImageOwnsPixels() {
    QString path = createValidPdf("0 0 m 100 100 l S");
    PdfFilePreviewService s;
    auto r = s.renderFilePage(path);
    QVERIFY(!r.image.isNull());
    QVERIFY(r.image.bytesPerLine() > 0);
}
void PdfFilePreviewServiceTest::testRenderFileDiagnosticsPreserved() {
    QString path = createValidPdf("0 0 m 100 100 l S");
    PdfFilePreviewService s;
    auto r = s.renderFilePage(path);
    QCOMPARE(r.documentResult.warning, PdfDocumentLoaderWarning::None);
    QCOMPARE(r.renderResult.warning, PdfPageImageRenderWarning::None);
}
void PdfFilePreviewServiceTest::testRenderFileEmptyStringPath() {
    PdfFilePreviewService s;
    auto r = s.renderFilePage("");
    QCOMPARE(r.warning, PdfFilePreviewWarning::FileNotFound);
}

QTEST_MAIN(PdfFilePreviewServiceTest)
#include "PdfFilePreviewServiceTest.moc"
