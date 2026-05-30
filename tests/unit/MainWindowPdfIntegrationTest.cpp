#include <QtTest/QTest>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QScrollArea>
#include <QTabWidget>
#include <QApplication>
#include "app/MainWindow.h"
#include "widgets/PdfSinglePagePreviewWidget.h"
#include "widgets/PdfZoomToolbar.h"
#include "PdfTestHelpers.h"

static QString createMarkdownFile() {
    static QTemporaryDir dir;
    QString path = dir.filePath("test.md");
    QFile f(path);
    f.open(QIODevice::WriteOnly);
    f.write("# Hello\n\nMarkdown content");
    f.close();
    return path;
}

static QTemporaryDir s_pdfDir;
static QString createValidPdf(const QByteArray &bytes) {
    return PdfTestHelpers::writePdfFile(s_pdfDir, "test.pdf", bytes);
}
static QString createInvalidPdf(const QByteArray &bytes) {
    return PdfTestHelpers::writeBytesFile(s_pdfDir, "not.pdf", bytes);
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
    void testPdfTabContainsZoomToolbarAndZoomWorks();
    void testInvalidPdfTabHasToolbarDisabled();
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

    QString path = createValidPdf(PdfTestHelpers::buildMinimalPdfBytes("0 0 m 100 100 l S"));
    w.openDocumentsFromPaths({path});

    QVERIFY2(tabs->count() >= 1, "A PDF tab should have been created");
    if (tabs->count() == 0) return;
    QWidget *tab = tabs->widget(0);
    QCOMPARE(tab->property("contentType").toString(), QString("pdf"));

    auto *sa = tab->findChild<QScrollArea *>();
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

    QString path = createInvalidPdf("not a pdf");
    w.openDocumentsFromPaths({path});

    QVERIFY2(tabs->count() >= 1, "A diagnostic tab should be created for invalid PDF");
    if (tabs->count() == 0) return;
    QWidget *tab = tabs->widget(0);
    auto *sa = tab->findChild<QScrollArea *>();
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

    QString path = createValidPdf(PdfTestHelpers::buildMinimalPdfBytes("0 0 m 100 100 l S"));
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

    QString path = createValidPdf(PdfTestHelpers::buildMinimalPdfBytes("0 0 m 100 100 l S"));
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

    QString path = createValidPdf(PdfTestHelpers::buildMinimalPdfBytes("BT /F1 12 Tf (Hello) Tj ET"));
    w.openDocumentsFromPaths({path});

    QVERIFY2(tabs->count() >= 1, "A PDF tab should be created for text-only PDF");
    if (tabs->count() == 0) return;
    QWidget *tab = tabs->widget(0);
    QCOMPARE(tab->property("contentType").toString(), QString("pdf"));
    auto *pw = qobject_cast<PdfSinglePagePreviewWidget *>(
        tab->findChild<QScrollArea *>()->widget());
    QVERIFY(pw != nullptr);
    QVERIFY(!pw->image().isNull());
    QVERIFY(PdfTestHelpers::countNonBgPixels(pw->image()) > 0);
    QVERIFY(pw->size() == pw->sizeHint());
}

void MainWindowPdfIntegrationTest::testOpenBlankPdf() {
    MainWindow w;
    w.show();
    QTabWidget *tabs = findTabWidget(w);
    QVERIFY(tabs != nullptr);

    QString path = createValidPdf(PdfTestHelpers::buildMinimalPdfBytes(""));
    w.openDocumentsFromPaths({path});

    QVERIFY2(tabs->count() >= 1, "A PDF tab should be created for blank PDF");
    if (tabs->count() == 0) return;
    QWidget *tab = tabs->widget(0);
    QCOMPARE(tab->property("contentType").toString(), QString("pdf"));
    auto *pw = qobject_cast<PdfSinglePagePreviewWidget *>(
        tab->findChild<QScrollArea *>()->widget());
    QVERIFY(pw != nullptr);
    QVERIFY(!pw->image().isNull());
}

void MainWindowPdfIntegrationTest::testOpenMixedPathTextPdfDisplaysBoth() {
    MainWindow w;
    w.show();
    QTabWidget *tabs = findTabWidget(w);
    QVERIFY(tabs != nullptr);

    QString path = createValidPdf(PdfTestHelpers::buildMinimalPdfBytes(
        "0 0 m 100 100 l S BT /F1 12 Tf (Hello) Tj ET"));
    w.openDocumentsFromPaths({path});

    QVERIFY2(tabs->count() >= 1, "A PDF tab should be created for mixed path+text PDF");
    if (tabs->count() == 0) return;
    QWidget *tab = tabs->widget(0);
    QCOMPARE(tab->property("contentType").toString(), QString("pdf"));
    auto *pw = qobject_cast<PdfSinglePagePreviewWidget *>(
        tab->findChild<QScrollArea *>()->widget());
    QVERIFY(pw != nullptr);
    QVERIFY(!pw->image().isNull());
    QVERIFY(PdfTestHelpers::countNonBgPixels(pw->image()) > 0);
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

    QString pdfPath = createValidPdf(PdfTestHelpers::buildMinimalPdfBytes("0 0 m 100 100 l S"));
    w.openDocumentsFromPaths({pdfPath});
    QCOMPARE(tabs->count(), 2);
    QVERIFY(qobject_cast<DocumentTabPage *>(tabs->widget(0)) != nullptr);
}

void MainWindowPdfIntegrationTest::testOpenPdfDoesNotBreakSubsequentMarkdownOpen() {
    MainWindow w;
    w.show();
    QTabWidget *tabs = findTabWidget(w);
    QVERIFY(tabs != nullptr);

    QString pdfPath = createValidPdf(PdfTestHelpers::buildMinimalPdfBytes("0 0 m 100 100 l S"));
    w.openDocumentsFromPaths({pdfPath});
    QCOMPARE(tabs->count(), 1);
    QCOMPARE(tabs->widget(0)->property("contentType").toString(), QString("pdf"));

    QString mdPath = createMarkdownFile();
    w.openDocumentsFromPaths({mdPath});
    QCOMPARE(tabs->count(), 2);
    QVERIFY(qobject_cast<DocumentTabPage *>(tabs->widget(1)) != nullptr);
}

void MainWindowPdfIntegrationTest::testPdfTabContainsZoomToolbarAndZoomWorks() {
    MainWindow w;
    w.show();
    QTabWidget *tabs = findTabWidget(w);
    QVERIFY(tabs != nullptr);

    QString path = createValidPdf(PdfTestHelpers::buildMinimalPdfBytes("0 0 m 100 100 l S"));
    w.openDocumentsFromPaths({path});

    QVERIFY(tabs->count() >= 1);
    QWidget *tab = tabs->widget(0);
    QCOMPARE(tab->property("contentType").toString(), QString("pdf"));

    auto *toolbar = tab->findChild<PdfZoomToolbar *>();
    QVERIFY(toolbar != nullptr);
    QVERIFY(toolbar->isEnabled());

    auto *inBtn = toolbar->findChild<QPushButton *>("pdfZoomInButton");
    auto *outBtn = toolbar->findChild<QPushButton *>("pdfZoomOutButton");
    auto *rstBtn = toolbar->findChild<QPushButton *>("pdfZoomResetButton");
    QVERIFY(inBtn != nullptr);
    QVERIFY(outBtn != nullptr);
    QVERIFY(rstBtn != nullptr);

    auto *pw = tab->findChild<PdfSinglePagePreviewWidget *>();
    QVERIFY(pw != nullptr);
    QVERIFY(!pw->image().isNull());
    QVERIFY(qFuzzyCompare(pw->zoomFactor(), 1.0));
    QVERIFY(pw->size() == pw->sizeHint());

    // Click zoom in
    QTest::mouseClick(inBtn, Qt::LeftButton);
    QVERIFY(pw->zoomFactor() > 1.0);
    QVERIFY(pw->size() == pw->sizeHint());
    QCOMPARE(rstBtn->text(), QString("125 %"));
}

void MainWindowPdfIntegrationTest::testInvalidPdfTabHasToolbarDisabled() {
    MainWindow w;
    w.show();
    QTabWidget *tabs = findTabWidget(w);
    QVERIFY(tabs != nullptr);

    QString path = createInvalidPdf("not a pdf");
    w.openDocumentsFromPaths({path});

    QVERIFY(tabs->count() >= 1);
    QWidget *tab = tabs->widget(0);
    QCOMPARE(tab->property("contentType").toString(), QString("pdf"));

    auto *toolbar = tab->findChild<PdfZoomToolbar *>();
    QVERIFY(toolbar != nullptr);
    QVERIFY(!toolbar->isEnabled());

    auto *pw = tab->findChild<PdfSinglePagePreviewWidget *>();
    QVERIFY(pw != nullptr);
    QVERIFY(!pw->statusText().isEmpty());
}

QTEST_MAIN(MainWindowPdfIntegrationTest)
#include "MainWindowPdfIntegrationTest.moc"
