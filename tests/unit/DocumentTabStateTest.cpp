#include <QTest>
#include <QString>
#include "app/document/DocumentTabState.h"

class DocumentTabStateTest : public QObject
{
    Q_OBJECT

private slots:
    void testDefaultConstruction();
    void testFieldAccess();
    void testCopySemantics();
};

void DocumentTabStateTest::testDefaultConstruction()
{
    DocumentTabState state;
    QVERIFY(state.filePath.isEmpty());
    QCOMPARE(state.zoomSteps, 0);
    QCOMPARE(state.contentMode, ContentMode::Plain);
    QVERIFY(state.originalHtml.isEmpty());
    QVERIFY(state.originalPlainText.isEmpty());
}

void DocumentTabStateTest::testFieldAccess()
{
    DocumentTabState state;
    state.filePath = "/path/to/file.md";
    state.zoomSteps = 3;
    state.contentMode = ContentMode::Html;
    state.originalHtml = "<p>hello</p>";
    state.originalPlainText = "hello";

    QCOMPARE(state.filePath, QString("/path/to/file.md"));
    QCOMPARE(state.zoomSteps, 3);
    QCOMPARE(state.contentMode, ContentMode::Html);
    QCOMPARE(state.originalHtml, QString("<p>hello</p>"));
    QCOMPARE(state.originalPlainText, QString("hello"));
}

void DocumentTabStateTest::testCopySemantics()
{
    DocumentTabState a;
    a.filePath = "/copy.md";
    a.zoomSteps = 5;

    DocumentTabState b = a;
    QCOMPARE(b.filePath, QString("/copy.md"));
    QCOMPARE(b.zoomSteps, 5);

    b.filePath = "/modified.md";
    QCOMPARE(a.filePath, QString("/copy.md"));
}

QTEST_MAIN(DocumentTabStateTest)
#include "DocumentTabStateTest.moc"
