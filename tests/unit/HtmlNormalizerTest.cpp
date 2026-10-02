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
    void testNestedDivsStayBalanced();
    void testDelBecomesStrikeout();
    void testSubSupPreserved();
    void testKbdWithAttribute();
    void testMarkWithAttribute();
    void testSummaryWithAttribute();
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

void HtmlNormalizerTest::testNestedDivsStayBalanced()
{
    const QString input = "<div>outer <div>inner</div> after-inner</div>tail";
    const QString result = HtmlNormalizer::normalizeRawTags(input);

    QCOMPARE(result.count("<table class=\"html-block-table\""), 2);
    QCOMPARE(result.count("</table>"), 2);
    // "after-inner" must still be inside the outer block, i.e. before the last </table>.
    QVERIFY(result.indexOf("after-inner") < result.lastIndexOf("</table>"));
    QVERIFY(result.endsWith("</td></tr></table>tail"));
}

void HtmlNormalizerTest::testDelBecomesStrikeout()
{
    const QString result = HtmlNormalizer::normalizeRawTags("<p><del>gone</del></p>");
    QCOMPARE(result, QString("<p><s>gone</s></p>"));
}

void HtmlNormalizerTest::testSubSupPreserved()
{
    QString input = "H<sub>2</sub>O and E=mc<sup>2</sup>";
    QString result = HtmlNormalizer::normalizeRawTags(input);
    QVERIFY2(result.contains("<sub>"), "<sub> must survive");
    QVERIFY2(result.contains("<sup>"), "<sup> must survive");
}

void HtmlNormalizerTest::testKbdWithAttribute()
{
    QString input = "<kbd class=\"custom\">Ctrl</kbd>";
    QString result = HtmlNormalizer::normalizeRawTags(input);
    QVERIFY2(!result.contains("<kbd"), "kbd with attr must be replaced");
    QVERIFY2(result.contains("<code>Ctrl</code>"), "kbd with attr must become code");
}

void HtmlNormalizerTest::testMarkWithAttribute()
{
    QString input = "<mark style=\"extra:true;\">text</mark>";
    QString result = HtmlNormalizer::normalizeRawTags(input);
    QVERIFY2(!result.contains("<mark"), "mark with attr must be replaced");
    QVERIFY2(result.contains("background-color: #FFF3B0"), "mark with attr must produce amber style");
}

void HtmlNormalizerTest::testSummaryWithAttribute()
{
    QString input = "<summary class=\"custom\">Title</summary>";
    QString result = HtmlNormalizer::normalizeRawTags(input);
    QVERIFY2(!result.contains("<summary"), "summary with attr must be replaced");
    QVERIFY2(result.contains("<p class=\"html-summary\">"), "summary with attr must become p.html-summary");
}

QTEST_MAIN(HtmlNormalizerTest)
#include "HtmlNormalizerTest.moc"
