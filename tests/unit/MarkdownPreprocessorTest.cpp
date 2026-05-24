#include <QTest>
#include "services/MarkdownPreprocessor.h"

class MarkdownPreprocessorTest : public QObject
{
    Q_OBJECT

private slots:
    void testStripsValidYamlFrontMatter();
    void testPreservesContentAfterFrontMatter();
    void testPreservesHorizontalRule();
    void testNoClosingDelimiter();
    void testEmptyInput();
    void testNoFrontMatter();
    void testCrlfFrontMatter();
    void testCrlfNoClosingDelimiter();
    void testDefinitionListConvertsToTable();
    void testDefinitionListMultipleEntries();
    void testDefinitionListInlineCodeInDescription();
    void testDefinitionListSkipsFencedCode();
    void testDefinitionListSkipsNormalParagraph();
};

void MarkdownPreprocessorTest::testStripsValidYamlFrontMatter()
{
    QString input = "---\ntitle: Test\n---\n\n# Hello";
    QString result = MarkdownPreprocessor::stripYamlFrontMatter(input);
    QCOMPARE(result, QString("# Hello"));
}

void MarkdownPreprocessorTest::testPreservesContentAfterFrontMatter()
{
    QString input = "---\nkey: value\n---\nBody text.";
    QString result = MarkdownPreprocessor::stripYamlFrontMatter(input);
    QVERIFY(result.contains("Body text."));
    QVERIFY(!result.contains("key: value"));
}

void MarkdownPreprocessorTest::testPreservesHorizontalRule()
{
    QString input = "# Heading\n\nSome text\n\n---\n\nMore text.";
    QString result = MarkdownPreprocessor::stripYamlFrontMatter(input);
    QCOMPARE(result, input);
}

void MarkdownPreprocessorTest::testNoClosingDelimiter()
{
    QString input = "---\ntitle: Open only\n\n# Content";
    QString result = MarkdownPreprocessor::stripYamlFrontMatter(input);
    QCOMPARE(result, input);
}

void MarkdownPreprocessorTest::testEmptyInput()
{
    QString result = MarkdownPreprocessor::stripYamlFrontMatter(QString());
    QVERIFY(result.isEmpty());

    result = MarkdownPreprocessor::stripYamlFrontMatter(QString(""));
    QVERIFY(result.isEmpty());
}

void MarkdownPreprocessorTest::testCrlfFrontMatter()
{
    QString input = "---\r\ntitle: Test\r\n---\r\n\r\n# Hello";
    QString result = MarkdownPreprocessor::stripYamlFrontMatter(input);
    QCOMPARE(result, QString("# Hello"));
}

void MarkdownPreprocessorTest::testCrlfNoClosingDelimiter()
{
    QString input = "---\r\ntitle: Open only\r\n\r\n# Content";
    QString result = MarkdownPreprocessor::stripYamlFrontMatter(input);
    QCOMPARE(result, input);
}

void MarkdownPreprocessorTest::testNoFrontMatter()
{
    QString input = "# Just a heading\n\nNormal paragraph.";
    QString result = MarkdownPreprocessor::stripYamlFrontMatter(input);
    QCOMPARE(result, input);
}

void MarkdownPreprocessorTest::testDefinitionListConvertsToTable()
{
    QString input = "Renderer\n: Converts Markdown into HTML.\n";
    QString result = MarkdownPreprocessor::convertDefinitionLists(input);
    QVERIFY2(result.contains("definition-table"),
             "definition list must produce definition-table");
    QVERIFY2(result.contains("definition-term"),
             "definition list must produce definition-term");
    QVERIFY2(result.contains("definition-description"),
             "definition list must produce definition-description");
    QVERIFY2(result.contains("Renderer"),
             "term content must be preserved");
    QVERIFY2(result.contains("Converts Markdown into HTML."),
             "description content must be preserved");
}

void MarkdownPreprocessorTest::testDefinitionListMultipleEntries()
{
    QString input = "Term A\n: Desc A.\n\nTerm B\n: Desc B.\n";
    QString result = MarkdownPreprocessor::convertDefinitionLists(input);
    QVERIFY2(result.contains("Term A"), "first term preserved");
    QVERIFY2(result.contains("Term B"), "second term preserved");
    QVERIFY2(result.contains("Desc A."), "first desc preserved");
    QVERIFY2(result.contains("Desc B."), "second desc preserved");
    // Should be a single table with 2 rows
    int tableCount = result.count("definition-table");
    QCOMPARE(tableCount, 1);
}

void MarkdownPreprocessorTest::testDefinitionListInlineCodeInDescription()
{
    QString input = "App\n: Uses `QSettings` for storage.\n";
    QString result = MarkdownPreprocessor::convertDefinitionLists(input);
    QVERIFY2(result.contains("<code>QSettings</code>"),
             "inline code in description must be converted to <code>");
    QVERIFY2(!result.contains("`QSettings`"),
             "raw backtick code must not remain");
}

void MarkdownPreprocessorTest::testDefinitionListSkipsFencedCode()
{
    QString input = "```\nTerm\n: Description\n```\n";
    QString result = MarkdownPreprocessor::convertDefinitionLists(input);
    QVERIFY2(!result.contains("definition-table"),
             "definition pattern inside fenced code must not be converted");
}

void MarkdownPreprocessorTest::testDefinitionListSkipsNormalParagraph()
{
    QString input = "This is a normal paragraph with a: colon in it.\n";
    QString result = MarkdownPreprocessor::convertDefinitionLists(input);
    QVERIFY2(!result.contains("definition-table"),
             "normal paragraph with colon must not be converted");
}

QTEST_MAIN(MarkdownPreprocessorTest)
#include "MarkdownPreprocessorTest.moc"
