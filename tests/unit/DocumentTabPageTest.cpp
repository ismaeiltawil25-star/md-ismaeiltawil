#include <QTest>
#include <QString>
#include "app/document/DocumentTabPage.h"

class DocumentTabPageTest : public QObject
{
    Q_OBJECT

private slots:
    void testDefaultConstruction();
    void testTypedAccessors();
    void testQTextBrowserBehaviorPreserved();
};

void DocumentTabPageTest::testDefaultConstruction()
{
    DocumentTabPage page;
    QVERIFY(page.filePath().isEmpty());
    QCOMPARE(page.zoomSteps(), 0);
    QCOMPARE(page.contentMode(), ContentMode::Plain);
    QVERIFY(page.originalHtml().isEmpty());
    QVERIFY(page.originalPlainText().isEmpty());
    QVERIFY(page.tabState().filePath.isEmpty());
}

void DocumentTabPageTest::testTypedAccessors()
{
    DocumentTabPage page;
    page.setFilePath("/test.md");
    page.setZoomSteps(2);
    page.setContentMode(ContentMode::Html);
    page.setOriginalHtml("<p>hello</p>");
    page.setOriginalPlainText("hello");

    QCOMPARE(page.filePath(), QString("/test.md"));
    QCOMPARE(page.zoomSteps(), 2);
    QCOMPARE(page.contentMode(), ContentMode::Html);
    QCOMPARE(page.originalHtml(), QString("<p>hello</p>"));
    QCOMPARE(page.originalPlainText(), QString("hello"));

    QCOMPARE(page.tabState().filePath, QString("/test.md"));
    QCOMPARE(page.tabState().zoomSteps, 2);
}

void DocumentTabPageTest::testQTextBrowserBehaviorPreserved()
{
    DocumentTabPage page;
    page.setPlainText("Hello World");
    QCOMPARE(page.toPlainText(), QString("Hello World"));

    page.setHtml("<p><b>Bold</b></p>");
    QVERIFY(page.toPlainText().contains("Bold"));
}

QTEST_MAIN(DocumentTabPageTest)
#include "DocumentTabPageTest.moc"
