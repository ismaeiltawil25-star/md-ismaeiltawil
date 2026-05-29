#include <QtTest/QTest>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QScrollArea>
#include <QTabWidget>
#include <QApplication>
#include "app/MainWindow.h"
#include "widgets/PdfSinglePagePreviewWidget.h"

static QByteArray buildPdf(const QByteArray &content = "",
    QByteArray pageExtra = " /MediaBox [0 0 612 792]") {
    QByteArray pdf;
    pdf += "%PDF-1.7\n1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    pdf += "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n";
    pdf += "3 0 obj\n<< /Type /Page /Parent 2 0 R /Contents 4 0 R" + pageExtra + " >>\nendobj\n";
    pdf += "4 0 obj\n<< /Length " + QByteArray::number(content.size())
        + " >>\nstream\n" + content + "\nendstream\nendobj\n";
    qsizetype xo = pdf.size(); pdf += "xref\n0 5\n0000000000 65535 f \n";
    auto c = [&](int n){auto p=pdf.indexOf(QByteArray::number(n)+" 0 obj");
        return QString("%1").arg(p,10,10,QLatin1Char('0')).toUtf8()+" 00000 n\n";};
    pdf+=c(1)+c(2)+c(3)+c(4);
    pdf+="trailer\n<< /Size 5 /Root 1 0 R >>\nstartxref\n"+QByteArray::number(xo)+"\n%%EOF\n";
    return pdf;
}

static QString createPdfFile(const QByteArray &content) {
    static QTemporaryDir dir;
    QString path = dir.filePath("test.pdf");
    QFile f(path);
    f.open(QIODevice::WriteOnly);
    f.write(content);
    f.close();
    return path;
}

static QString createMarkdownFile() {
    static QTemporaryDir dir;
    QString path = dir.filePath("test.md");
    QFile f(path);
    f.open(QIODevice::WriteOnly);
    f.write("# Hello\n\nMarkdown content");
    f.close();
    return path;
}

static int countNonBg(const QImage &img) {
    int n = 0; QRgb bg = Qt::white;
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            if (img.pixel(x, y) != bg) ++n;
    return n;
}

class MainWindowPdfIntegrationTest : public QObject {
    Q_OBJECT

private:
    QTabWidget *findTabWidget(MainWindow &w) const;

private slots:
    void initTestCase();

    void testIsSupportedFileAcceptsPdf();
    void testOpenPdfDocumentCreatesPdfTab();
    void testOpenPdfDocumentInvalidPdfShowsStatus();
    void testOpenDuplicatePdfFocusesExistingTab();
    void testPdfTabCloseRestoresPlaceholder();
    void testMarkdownOpenFlowUnchanged();

    void testOpenTextOnlyPdf();
    void testOpenBlankPdf();
    void testOpenMixedPathTextPdfDisplaysBoth();
    void testOpenPdfDoesNotAffectMarkdownTab();
    void testOpenPdfDoesNotBreakSubsequentMarkdownOpen();
};

QTabWidget *MainWindowPdfIntegrationTest::findTabWidget(MainWindow &w) const {
    return w.findChild<QTabWidget *>();
}

void MainWindowPdfIntegrationTest::initTestCase() {
    QVERIFY(QApplication::instance() != nullptr);
}

void MainWindowPdfIntegrationTest::testIsSupportedFileAcceptsPdf() {
    QVERIFY(MainWindow::isPdfFile("test.pdf"));
    QVERIFY(MainWindow::isPdfFile("document.PDF"));
    QVERIFY(!MainWindow::isPdfFile("test.md"));
    QVERIFY(!MainWindow::isPdfFile("test.txt"));
}

void MainWindowPdfIntegrationTest::testOpenPdfDocumentCreatesPdfTab() {
    MainWindow w;
    w.show();
    QTabWidget *tabs = findTabWidget(w);
    QVERIFY(tabs != nullptr);

    QString path = createPdfFile(buildPdf("0 0 m 100 100 l S"));
    w.openDocumentsFromPaths({path});

    QVERIFY2(tabs->count() >= 1, "A PDF tab should have been created");
    if (tabs->count() == 0) return;
    QWidget *tab = tabs->widget(0);
    QCOMPARE(tab->property("contentType").toString(), QString("pdf"));

    auto *sa = qobject_cast<QScrollArea *>(tab);
    QVERIFY(sa != nullptr);
    auto *pw = qobject_cast<PdfSinglePagePreviewWidget *>(sa->widget());
    QVERIFY(pw != nullptr);
    QVERIFY(!pw->image().isNull());
    QVERIFY(pw->size() == pw->sizeHint());
}

void MainWindowPdfIntegrationTest::testOpenPdfDocumentInvalidPdfShowsStatus() {
    MainWindow w;
    w.show();
    QTabWidget *tabs = findTabWidget(w);
    QVERIFY(tabs != nullptr);

    QString path = createPdfFile("not a pdf");
    w.openDocumentsFromPaths({path});

    QVERIFY2(tabs->count() >= 1, "A diagnostic tab should be created for invalid PDF");
    if (tabs->count() == 0) return;
    QWidget *tab = tabs->widget(0);
    auto *sa = qobject_cast<QScrollArea *>(tab);
    QVERIFY(sa != nullptr);
    auto *pw = qobject_cast<PdfSinglePagePreviewWidget *>(sa->widget());
    QVERIFY(pw != nullptr);
    QVERIFY(!pw->statusText().isEmpty());
}

void MainWindowPdfIntegrationTest::testOpenDuplicatePdfFocusesExistingTab() {
    MainWindow w;
    w.show();
    QTabWidget *tabs = findTabWidget(w);
    QVERIFY(tabs != nullptr);

    QString path = createPdfFile(buildPdf("0 0 m 100 100 l S"));
    w.openDocumentsFromPaths({path});
    QCOMPARE(tabs->count(), 1);
    w.openDocumentsFromPaths({path});
    QCOMPARE(tabs->count(), 1);
}

void MainWindowPdfIntegrationTest::testPdfTabCloseRestoresPlaceholder() {
    MainWindow w;
    w.show();
    QTabWidget *tabs = findTabWidget(w);
    QVERIFY(tabs != nullptr);

    QString path = createPdfFile(buildPdf("0 0 m 100 100 l S"));
    w.openDocumentsFromPaths({path});
    QCOMPARE(tabs->count(), 1);
    tabs->removeTab(0);
    QCOMPARE(tabs->count(), 0);
}

void MainWindowPdfIntegrationTest::testMarkdownOpenFlowUnchanged() {
    MainWindow w;
    w.show();
    QTabWidget *tabs = findTabWidget(w);
    QVERIFY(tabs != nullptr);

    QString mdPath = createMarkdownFile();
    w.openDocumentsFromPaths({mdPath});

    QVERIFY2(tabs->count() >= 1, "A Markdown tab should be created");
    if (tabs->count() == 0) return;
    auto *dtp = qobject_cast<DocumentTabPage *>(tabs->widget(0));
    QVERIFY(dtp != nullptr);
}

void MainWindowPdfIntegrationTest::testOpenTextOnlyPdf() {
    MainWindow w;
    w.show();
    QTabWidget *tabs = findTabWidget(w);
    QVERIFY(tabs != nullptr);

    QString path = createPdfFile(buildPdf("BT /F1 12 Tf (Hello) Tj ET"));
    w.openDocumentsFromPaths({path});

    QVERIFY2(tabs->count() >= 1, "A PDF tab should be created for text-only PDF");
    if (tabs->count() == 0) return;
    QWidget *tab = tabs->widget(0);
    QCOMPARE(tab->property("contentType").toString(), QString("pdf"));
    auto *pw = qobject_cast<PdfSinglePagePreviewWidget *>(
        qobject_cast<QScrollArea *>(tab)->widget());
    QVERIFY(pw != nullptr);
    QVERIFY(!pw->image().isNull());
    QVERIFY(countNonBg(pw->image()) > 0);
    QVERIFY(pw->size() == pw->sizeHint());
}

void MainWindowPdfIntegrationTest::testOpenBlankPdf() {
    MainWindow w;
    w.show();
    QTabWidget *tabs = findTabWidget(w);
    QVERIFY(tabs != nullptr);

    QString path = createPdfFile(buildPdf(""));
    w.openDocumentsFromPaths({path});

    QVERIFY2(tabs->count() >= 1, "A PDF tab should be created for blank PDF");
    if (tabs->count() == 0) return;
    QWidget *tab = tabs->widget(0);
    QCOMPARE(tab->property("contentType").toString(), QString("pdf"));
    auto *pw = qobject_cast<PdfSinglePagePreviewWidget *>(
        qobject_cast<QScrollArea *>(tab)->widget());
    QVERIFY(pw != nullptr);
    QVERIFY(!pw->image().isNull());
}

void MainWindowPdfIntegrationTest::testOpenMixedPathTextPdfDisplaysBoth() {
    MainWindow w;
    w.show();
    QTabWidget *tabs = findTabWidget(w);
    QVERIFY(tabs != nullptr);

    QString path = createPdfFile(buildPdf(
        "0 0 m 100 100 l S BT /F1 12 Tf (Hello) Tj ET"));
    w.openDocumentsFromPaths({path});

    QVERIFY2(tabs->count() >= 1, "A PDF tab should be created for mixed path+text PDF");
    if (tabs->count() == 0) return;
    QWidget *tab = tabs->widget(0);
    QCOMPARE(tab->property("contentType").toString(), QString("pdf"));
    auto *pw = qobject_cast<PdfSinglePagePreviewWidget *>(
        qobject_cast<QScrollArea *>(tab)->widget());
    QVERIFY(pw != nullptr);
    QVERIFY(!pw->image().isNull());
    QVERIFY(countNonBg(pw->image()) > 0);
}

void MainWindowPdfIntegrationTest::testOpenPdfDoesNotAffectMarkdownTab() {
    MainWindow w;
    w.show();
    QTabWidget *tabs = findTabWidget(w);
    QVERIFY(tabs != nullptr);

    QString mdPath = createMarkdownFile();
    w.openDocumentsFromPaths({mdPath});
    QCOMPARE(tabs->count(), 1);
    QVERIFY(qobject_cast<DocumentTabPage *>(tabs->widget(0)) != nullptr);

    QString pdfPath = createPdfFile(buildPdf("0 0 m 100 100 l S"));
    w.openDocumentsFromPaths({pdfPath});
    QCOMPARE(tabs->count(), 2);
    QVERIFY(qobject_cast<DocumentTabPage *>(tabs->widget(0)) != nullptr);
}

void MainWindowPdfIntegrationTest::testOpenPdfDoesNotBreakSubsequentMarkdownOpen() {
    MainWindow w;
    w.show();
    QTabWidget *tabs = findTabWidget(w);
    QVERIFY(tabs != nullptr);

    QString pdfPath = createPdfFile(buildPdf("0 0 m 100 100 l S"));
    w.openDocumentsFromPaths({pdfPath});
    QCOMPARE(tabs->count(), 1);
    QCOMPARE(tabs->widget(0)->property("contentType").toString(), QString("pdf"));

    QString mdPath = createMarkdownFile();
    w.openDocumentsFromPaths({mdPath});
    QCOMPARE(tabs->count(), 2);
    QVERIFY(qobject_cast<DocumentTabPage *>(tabs->widget(1)) != nullptr);
}

QTEST_MAIN(MainWindowPdfIntegrationTest)
#include "MainWindowPdfIntegrationTest.moc"
