// Clicking links in a document (MainWindow::onAnchorClicked).

#include <QScrollBar>
#include <QStatusBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>

#include "app/MainWindow.h"

namespace {

void writeFile(const QString &path, const QByteArray &content)
{
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(content);
}

} // namespace

class MainWindowLinkNavigationTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void sectionLinkScrollsWithinDocument();
    void relativeMarkdownLinkOpensInNewTab();
    void wikiLinkOpensTargetPage();
    void unsupportedLocalFileIsNotLaunched();
    void missingTargetIsReported();

private:
    DocumentTabPage *openMain(MainWindow &window);

    QTemporaryDir m_dir;
};

void MainWindowLinkNavigationTest::initTestCase()
{
    QVERIFY(m_dir.isValid());
    QByteArray main = "# Top\n\n[go](#target-section)\n\n";
    for (int i = 0; i < 200; ++i)
        main += "Filler paragraph " + QByteArray::number(i) + ".\n\n";
    main += "## Target Section\n\nHere.\n";
    writeFile(m_dir.filePath("main.md"), main);
    writeFile(m_dir.filePath("other.md"), "# Other\n\nSecond file.\n");
    writeFile(m_dir.filePath("Linked Page.md"), "# Linked\n");
    writeFile(m_dir.filePath("tool.bat"), "echo should never run\n");
}

DocumentTabPage *MainWindowLinkNavigationTest::openMain(MainWindow &window)
{
    window.resize(900, 600);
    window.show();
    if (!QTest::qWaitForWindowExposed(&window))
        return nullptr;
    window.openDocumentsFromPaths({m_dir.filePath("main.md")});
    QTest::qWait(50);
    return qobject_cast<DocumentTabPage *>(window.findChild<QTabWidget *>()->currentWidget());
}

void MainWindowLinkNavigationTest::sectionLinkScrollsWithinDocument()
{
    MainWindow window;
    DocumentTabPage *page = openMain(window);
    QVERIFY(page);
    QCOMPARE(page->verticalScrollBar()->value(), 0);

    emit page->anchorClicked(QUrl("#target-section"));
    QVERIFY2(page->verticalScrollBar()->value() > 0, "the view did not move to the heading");
}

void MainWindowLinkNavigationTest::relativeMarkdownLinkOpensInNewTab()
{
    MainWindow window;
    DocumentTabPage *page = openMain(window);
    QVERIFY(page);
    auto *tabs = window.findChild<QTabWidget *>();

    emit page->anchorClicked(QUrl("other.md"));
    QCOMPARE(tabs->count(), 2);
    QCOMPARE(tabs->tabText(tabs->currentIndex()), QString("other.md"));
}

void MainWindowLinkNavigationTest::wikiLinkOpensTargetPage()
{
    MainWindow window;
    DocumentTabPage *page = openMain(window);
    QVERIFY(page);
    auto *tabs = window.findChild<QTabWidget *>();

    emit page->anchorClicked(QUrl("Linked Page.md")); // what [[Linked Page]] renders to
    QCOMPARE(tabs->count(), 2);
    QCOMPARE(tabs->tabText(tabs->currentIndex()), QString("Linked Page.md"));
}

void MainWindowLinkNavigationTest::unsupportedLocalFileIsNotLaunched()
{
    MainWindow window;
    DocumentTabPage *page = openMain(window);
    QVERIFY(page);
    auto *tabs = window.findChild<QTabWidget *>();

    emit page->anchorClicked(QUrl("tool.bat"));
    QCOMPARE(tabs->count(), 1);
    QVERIFY(window.statusBar()->currentMessage().contains("unsupported", Qt::CaseInsensitive));
}

void MainWindowLinkNavigationTest::missingTargetIsReported()
{
    MainWindow window;
    DocumentTabPage *page = openMain(window);
    QVERIFY(page);

    emit page->anchorClicked(QUrl("does-not-exist.md"));
    QVERIFY(window.statusBar()->currentMessage().contains("not found", Qt::CaseInsensitive));
}

QTEST_MAIN(MainWindowLinkNavigationTest)
#include "MainWindowLinkNavigationTest.moc"
