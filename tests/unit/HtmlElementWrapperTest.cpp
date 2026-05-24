#include <QTest>
#include "renderers/HtmlElementWrapper.h"

class HtmlElementWrapperTest : public QObject
{
    Q_OBJECT

private slots:
    void testCodeBlockWrapping();
    void testBlockquoteWrapping();
    void testNoCodeBlockNoWrap();
    void testNoBlockquoteNoWrap();
};

void HtmlElementWrapperTest::testCodeBlockWrapping()
{
    QString input = "<pre><code class=\"language-cpp\">int x = 1;\n</code></pre>";
    QString result = HtmlElementWrapper::wrapCodeBlocks(input);
    QVERIFY2(result.contains("code-block-table"), "code block must be wrapped in code-block-table");
    QVERIFY2(result.contains("int x = 1;"), "code content must be preserved");
    QVERIFY2(result.contains("<pre><code"), "original pre/code must still be present");
}

void HtmlElementWrapperTest::testBlockquoteWrapping()
{
    QString input = "<blockquote><p>A simple quote.</p></blockquote>";
    QString result = HtmlElementWrapper::wrapBlockquotes(input);
    QVERIFY2(result.contains("blockquote-table"), "blockquote must be wrapped in blockquote-table");
    QVERIFY2(result.contains("blockquote-bar"), "blockquote must contain blockquote-bar");
    QVERIFY2(result.contains("blockquote-cell"), "blockquote must contain blockquote-cell");
    QVERIFY2(result.contains("A simple quote."), "content must be preserved");
}

void HtmlElementWrapperTest::testNoCodeBlockNoWrap()
{
    QString input = "<p>Just a paragraph with no code.</p>";
    QString result = HtmlElementWrapper::wrapCodeBlocks(input);
    QVERIFY2(!result.contains("code-block-table"), "no code-block-table for plain text");
    QCOMPARE(result, input);
}

void HtmlElementWrapperTest::testNoBlockquoteNoWrap()
{
    QString input = "<p>Just a paragraph.</p>";
    QString result = HtmlElementWrapper::wrapBlockquotes(input);
    QVERIFY2(!result.contains("blockquote-table"), "no blockquote-table for plain text");
    QCOMPARE(result, input);
}

QTEST_MAIN(HtmlElementWrapperTest)
#include "HtmlElementWrapperTest.moc"
