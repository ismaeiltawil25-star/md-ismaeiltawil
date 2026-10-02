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

    // Regression tests: front matter must never swallow real content.
    void testLeadingHorizontalRuleIsNotFrontMatter();
    void testProseBetweenRulesIsNotFrontMatter();
    void testFrontMatterWithListsAndComments();
    void testFrontMatterClosedWithDots();
    void testEmptyFrontMatterBlockIsKept();

    // Regression tests: definition lists must not rewrite code or other blocks.
    void testDefinitionListSkipsTildeFence();
    void testDefinitionListSkipsIndentedBacktickFence();
    void testDefinitionListSkipsHeadingTerm();
    void testDefinitionListTableEndsWithBlankLine();
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

void MarkdownPreprocessorTest::testLeadingHorizontalRuleIsNotFrontMatter()
{
    const QString input = "---\n\nFirst paragraph.\n\n---\n\nSecond paragraph.\n";
    QCOMPARE(MarkdownPreprocessor::stripYamlFrontMatter(input), input);
}

void MarkdownPreprocessorTest::testProseBetweenRulesIsNotFrontMatter()
{
    const QString input = "---\nThis sentence has no key\n---\nBody";
    QCOMPARE(MarkdownPreprocessor::stripYamlFrontMatter(input), input);
}

void MarkdownPreprocessorTest::testFrontMatterWithListsAndComments()
{
    const QString input = "---\n# comment\ntitle: Test\ntags:\n  - a\n  - b\ndescription: >\n  long text\n---\n# Body";
    QCOMPARE(MarkdownPreprocessor::stripYamlFrontMatter(input), QString("# Body"));
}

void MarkdownPreprocessorTest::testFrontMatterClosedWithDots()
{
    const QString input = "---\ntitle: Test\n...\nBody";
    QCOMPARE(MarkdownPreprocessor::stripYamlFrontMatter(input), QString("Body"));
}

void MarkdownPreprocessorTest::testEmptyFrontMatterBlockIsKept()
{
    // Two rules with nothing between them are rendered, not silently dropped.
    const QString input = "---\n---\nBody";
    QCOMPARE(MarkdownPreprocessor::stripYamlFrontMatter(input), input);
}

void MarkdownPreprocessorTest::testDefinitionListSkipsTildeFence()
{
    const QString input = "~~~\nkey\n: value\n~~~\n";
    const QString result = MarkdownPreprocessor::convertDefinitionLists(input);
    QVERIFY2(!result.contains("definition-table"), "tilde fenced code must not be converted");
    QVERIFY(result.contains("key\n: value"));
}

void MarkdownPreprocessorTest::testDefinitionListSkipsIndentedBacktickFence()
{
    const QString input = "  ```\nkey\n: value\n  ```\n";
    const QString result = MarkdownPreprocessor::convertDefinitionLists(input);
    QVERIFY2(!result.contains("definition-table"), "indented fenced code must not be converted");
}

void MarkdownPreprocessorTest::testDefinitionListSkipsHeadingTerm()
{
    const QString input = "# Heading\n: not a description\n";
    const QString result = MarkdownPreprocessor::convertDefinitionLists(input);
    QVERIFY(!result.contains("definition-table"));
}

void MarkdownPreprocessorTest::testDefinitionListTableEndsWithBlankLine()
{
    // Without a blank line after </table>, md4c would treat the next paragraph
    // as part of the raw HTML block and show its Markdown literally.
    const QString input = "Term\n: Desc\n\nNext **bold** paragraph.\n";
    const QString result = MarkdownPreprocessor::convertDefinitionLists(input);
    QVERIFY(result.contains("</table>\n\nNext **bold** paragraph."));
}

QTEST_MAIN(MarkdownPreprocessorTest)
#include "MarkdownPreprocessorTest.moc"
