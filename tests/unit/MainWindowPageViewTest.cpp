// Page view through the real MainWindow open path (settings load, appearance,
// setHtml, tab insertion, showing). Guards against anything in that sequence
// silently resetting the document back to one continuous column.

#include <QAbstractTextDocumentLayout>
#include <QScrollBar>
#include <QStandardPaths>
#include <QTest>
#include <QTextBlock>
#include <QTextLayout>

#include "app/MainWindow.h"
#include "app/layout/PageLayoutRepository.h"

using namespace mdbr::layout;

class MainWindowPageViewTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void openedDocumentIsPaginated_data();
    void openedDocumentIsPaginated();
};

void MainWindowPageViewTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName("md-backpack-reader-tests");
    QCoreApplication::setApplicationName("MainWindowPageViewTest");
}

void MainWindowPageViewTest::cleanupTestCase()
{
    PageLayoutRepository::clear();
}

void MainWindowPageViewTest::openedDocumentIsPaginated_data()
{
    QTest::addColumn<int>("paper");
    QTest::newRow("A4") << int(PaperSize::A4);
    QTest::newRow("A5") << int(PaperSize::A5);
}

void MainWindowPageViewTest::openedDocumentIsPaginated()
{
    QFETCH(int, paper);
    PageLayoutSettings settings;
    settings.pageViewEnabled = true;
    settings.paper = PaperSize(paper);
    PageLayoutRepository::save(settings);

    MainWindow window;
    window.resize(1000, 900);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    window.openDocumentsFromPaths({QStringLiteral(TEST_DATA_DIR "/comprehensive-test.md")});
    QTest::qWait(200);

    auto *page = window.findChild<DocumentTabPage *>();
    QVERIFY(page);
    QVERIFY(page->isPageViewEnabled());
    QCOMPARE(page->lineWrapMode(), QTextEdit::FixedPixelWidth);
    QCOMPARE(page->document()->pageSize(), page->pageGeometry().pageSize);
    QVERIFY2(page->pageCount() > 2, qPrintable(QString("pages: %1").arg(page->pageCount())));

    // The view starts at the top of the first page.
    QCOMPARE(page->verticalScrollBar()->value(), 0);

    // Every line sits inside a page's text area (never in a gap).
    const PageGeometry &g = page->pageGeometry();
    const qreal pageHeight = g.pageSize.height();
    auto *layout = page->document()->documentLayout();
    for (QTextBlock block = page->document()->begin(); block.isValid(); block = block.next()) {
        const QRectF rect = layout->blockBoundingRect(block);
        for (int i = 0; i < block.layout()->lineCount(); ++i) {
            const QTextLine line = block.layout()->lineAt(i);
            if (block.text().mid(line.textStart(), line.textLength()).trimmed().isEmpty())
                continue; // nothing visible on this line
            const qreal top = rect.top() + line.y();
            const qreal pageTop = int(top / pageHeight) * pageHeight;
            QVERIFY2(top >= pageTop + g.margin - 1
                     && top + line.height() <= pageTop + pageHeight - g.margin + 1,
                     qPrintable(QString("line \"%1\" at y=%2 is outside the text area")
                                    .arg(block.text().left(30)).arg(top)));
        }
    }
}

QTEST_MAIN(MainWindowPageViewTest)
#include "MainWindowPageViewTest.moc"
