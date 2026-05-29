#include <QtTest/QTest>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QDir>
#include "PdfFilePreviewService.h"

static QByteArray buildPdf(const QByteArray &content, bool flate = false,
    QByteArray pageExtra = "", QByteArray parentExtra = "") {
    QByteArray body = content;
    QByteArray len = QByteArray::number(body.size());
    QByteArray filter;
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
    return pdf;
}
static QTemporaryFile *makeTempFile(const QByteArray &content) {
    auto *f = new QTemporaryFile;
    f->open(); f->write(content); f->flush(); f->close();
    return f;
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
    auto *f = makeTempFile(QByteArray{});
    PdfFilePreviewService s;
    auto r = s.renderFilePage(f->fileName());
    QCOMPARE(r.warning, PdfFilePreviewWarning::EmptyFile);
    delete f;
}
void PdfFilePreviewServiceTest::testRenderFileInvalidPdf() {
    auto *f = makeTempFile(QByteArray("not a pdf"));
    PdfFilePreviewService s;
    auto r = s.renderFilePage(f->fileName());
    QCOMPARE(r.warning, PdfFilePreviewWarning::DocumentLoadFailure);
    QCOMPARE(r.documentResult.warning, PdfDocumentLoaderWarning::InvalidHeader);
    delete f;
}
void PdfFilePreviewServiceTest::testRenderFileBlankPdf() {
    auto *f = makeTempFile(buildPdf("", false, " /MediaBox [0 0 612 792]"));
    PdfFilePreviewService s;
    auto r = s.renderFilePage(f->fileName());
    QCOMPARE(r.warning, PdfFilePreviewWarning::NoRenderablePaths);
    QVERIFY(r.ok()); QVERIFY(!r.image.isNull());
    delete f;
}
void PdfFilePreviewServiceTest::testRenderFileValidPathPdf() {
    auto *f = makeTempFile(buildPdf("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]"));
    PdfFilePreviewService s;
    auto r = s.renderFilePage(f->fileName());
    QCOMPARE(r.warning, PdfFilePreviewWarning::None);
    QVERIFY(r.ok());
    QVERIFY(!r.image.isNull());
    delete f;
}
void PdfFilePreviewServiceTest::testRenderFileTextOnlyPdf() {
    auto *f = makeTempFile(buildPdf("BT /F1 12 Tf (Hello) Tj ET", false, " /MediaBox [0 0 612 792]"));
    PdfFilePreviewService s;
    auto r = s.renderFilePage(f->fileName());
    QCOMPARE(r.warning, PdfFilePreviewWarning::None);
    QVERIFY(r.ok()); QVERIFY(!r.image.isNull());
    delete f;
}
void PdfFilePreviewServiceTest::testRenderFileInvalidPageIndex() {
    auto *f = makeTempFile(buildPdf("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]"));
    PdfFilePreviewService s;
    auto r = s.renderFilePage(f->fileName(), -1);
    QCOMPARE(r.warning, PdfFilePreviewWarning::RenderFailure);
    delete f;
}
void PdfFilePreviewServiceTest::testRenderFileHighDpiSizeGuard() {
    auto *f = makeTempFile(buildPdf("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]"));
    PdfPathRenderSettings ss; ss.dpi = 7200;
    PdfFilePreviewService s;
    auto r = s.renderFilePage(f->fileName(), 0, ss);
    QCOMPARE(r.warning, PdfFilePreviewWarning::RenderFailure);
    QVERIFY(!r.ok());
    delete f;
}
void PdfFilePreviewServiceTest::testRenderFileImageOwnsPixels() {
    auto *f = makeTempFile(buildPdf("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]"));
    PdfFilePreviewService s;
    auto r = s.renderFilePage(f->fileName());
    QVERIFY(!r.image.isNull());
    QVERIFY(r.image.bytesPerLine() > 0);
    delete f;
}
void PdfFilePreviewServiceTest::testRenderFileDiagnosticsPreserved() {
    auto *f = makeTempFile(buildPdf("0 0 m 100 100 l S", false, " /MediaBox [0 0 612 792]"));
    PdfFilePreviewService s;
    auto r = s.renderFilePage(f->fileName());
    QCOMPARE(r.documentResult.warning, PdfDocumentLoaderWarning::None);
    QCOMPARE(r.renderResult.warning, PdfPageImageRenderWarning::None);
    delete f;
}
void PdfFilePreviewServiceTest::testRenderFileEmptyStringPath() {
    PdfFilePreviewService s;
    auto r = s.renderFilePage("");
    QCOMPARE(r.warning, PdfFilePreviewWarning::FileNotFound);
}

QTEST_MAIN(PdfFilePreviewServiceTest)
#include "PdfFilePreviewServiceTest.moc"
