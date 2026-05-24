#include <QTest>
#include "renderers/HtmlNormalizer.h"

class HtmlNormalizerTest : public QObject
{
    Q_OBJECT

private slots:
    void testKbdToCode();
    void testMarkToStyledSpan();
    void testSummaryToParagraph();
    void testDetailsStripped();
    void testDivToTable();
    void testSubSupPreserved();
};

void HtmlNormalizerTest::testKbdToCode()
{
    QString input = "<kbd>Ctrl</kbd> + <kbd>O</kbd>";
    QString result = HtmlNormalizer::normalizeRawTags(input);
    QVERIFY2(!result.contains("<kbd>"), "<kbd> must be replaced");
    QVERIFY2(result.contains("<code>Ctrl</code>"), "kbd must become code");
}

void HtmlNormalizerTest::testMarkToStyledSpan()
{
    QString input = "<mark>highlight</mark>";
    QString result = HtmlNormalizer::normalizeRawTags(input);
    QVERIFY2(!result.contains("<mark>"), "<mark> must be replaced");
    QVERIFY2(result.contains("background-color: #FFF3B0"), "mark must produce amber highlight style");
}

void HtmlNormalizerTest::testSummaryToParagraph()
{
    QString input = "<summary>Title</summary>";
    QString result = HtmlNormalizer::normalizeRawTags(input);
    QVERIFY2(!result.contains("<summary>"), "<summary> must be replaced");
    QVERIFY2(result.contains("<p class=\"html-summary\">"), "summary must become p.html-summary");
}

void HtmlNormalizerTest::testDetailsStripped()
{
    QString input = "<details><summary>Info</summary><p>Content</p></details>";
    QString result = HtmlNormalizer::normalizeRawTags(input);
    QVERIFY2(!result.contains("<details>"), "<details> must be stripped");
    QVERIFY2(!result.contains("</details>"), "</details> must be stripped");
}

void HtmlNormalizerTest::testDivToTable()
{
    QString input = "<div style=\"border:1px solid #888;\"><strong>Content</strong></div>";
    QString result = HtmlNormalizer::normalizeRawTags(input);
    QVERIFY2(result.contains("html-block-table"), "div must become html-block-table");
    QVERIFY2(result.contains("html-block-cell"), "div must become html-block-cell");
    QVERIFY2(result.contains("Content"), "div content must be preserved");
}

void HtmlNormalizerTest::testSubSupPreserved()
{
    QString input = "H<sub>2</sub>O and E=mc<sup>2</sup>";
    QString result = HtmlNormalizer::normalizeRawTags(input);
    QVERIFY2(result.contains("<sub>"), "<sub> must survive");
    QVERIFY2(result.contains("<sup>"), "<sup> must survive");
}

QTEST_MAIN(HtmlNormalizerTest)
#include "HtmlNormalizerTest.moc"
