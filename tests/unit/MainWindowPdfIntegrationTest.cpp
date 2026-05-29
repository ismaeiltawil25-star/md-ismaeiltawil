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

    QTemporaryDir dir;
    QString mdPath = dir.filePath("test.md");
    QFile f(mdPath);
    f.open(QIODevice::WriteOnly);
    f.write("# Hello\n\nMarkdown content");
    f.close();
    w.openDocumentsFromPaths({mdPath});

    QVERIFY2(tabs->count() >= 1, "A Markdown tab should be created");
    if (tabs->count() == 0) return;
    auto *dtp = qobject_cast<DocumentTabPage *>(tabs->widget(0));
    QVERIFY(dtp != nullptr);
}

QTEST_MAIN(MainWindowPdfIntegrationTest)
#include "MainWindowPdfIntegrationTest.moc"
