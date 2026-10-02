// End-to-end contract: Markdown -> pipeline HTML -> QTextDocument.
//
// These tests assert what the reader actually SHOWS, not the intermediate
// HTML string. If a renderer change emits HTML that QTextDocument does not
// understand, these tests fail even when string-level tests still pass.

#include <QTest>
#include <QTextBlock>
#include <QTextCursor>

#include "RenderPipelineTestHelpers.h"

using mdbr::test::renderToDocument;

namespace {

// Character format of the first occurrence of `word` (format of its last character).
QTextCharFormat formatOf(const QTextDocument &doc, const QString &word)
{
    const QTextCursor cursor = doc.find(word);
    return cursor.isNull() ? QTextCharFormat() : cursor.charFormat();
}

QTextBlockFormat blockFormatOf(const QTextDocument &doc, const QString &word)
{
    const QTextCursor cursor = doc.find(word);
    return cursor.isNull() ? QTextBlockFormat() : cursor.blockFormat();
}

} // namespace

class RenderedDocumentContractTest : public QObject
{
    Q_OBJECT

private slots:
    void strikethroughIsStruckOut();
    void bareUrlBecomesLink();
    void wwwAutolinkBecomesLink();
    void boldAndItalicKeepFormatting();
    void arabicParagraphIsRightToLeft();
    void arabicParagraphAfterCodeBlockIsRightToLeft();
    void arabicQuoteIsRightToLeft();
    void englishParagraphIsNotRightToLeft();
    void taskListShowsBallotBoxes();
    void leadingHorizontalRuleKeepsContent();
    void nestedDivKeepsContentOrder();
    void definitionListThenParagraphRendersMarkdown();
    void fencedCodeIsShownVerbatim();
};

void RenderedDocumentContractTest::strikethroughIsStruckOut()
{
    const auto r = renderToDocument("Keep ~~gone~~ text");
    QVERIFY(r.renderResult.isValid());
    QVERIFY(!r.document->toPlainText().contains("~~"));
    QVERIFY(formatOf(*r.document, "gone").fontStrikeOut());
    QVERIFY(!formatOf(*r.document, "Keep").fontStrikeOut());
}

void RenderedDocumentContractTest::bareUrlBecomesLink()
{
    const auto r = renderToDocument("Visit https://example.com/page today.");
    const QTextCharFormat format = formatOf(*r.document, "example.com/page");
    QVERIFY(format.isAnchor());
    QCOMPARE(format.anchorHref(), QString("https://example.com/page"));
}

void RenderedDocumentContractTest::wwwAutolinkBecomesLink()
{
    const auto r = renderToDocument("See www.example.org for details.");
    QVERIFY(formatOf(*r.document, "www.example.org").isAnchor());
}

void RenderedDocumentContractTest::boldAndItalicKeepFormatting()
{
    const auto r = renderToDocument("A **strong** and *soft* word.");
    QVERIFY(formatOf(*r.document, "strong").fontWeight() >= QFont::Bold);
    QVERIFY(formatOf(*r.document, "soft").fontItalic());
}

void RenderedDocumentContractTest::arabicParagraphIsRightToLeft()
{
    // "مرحبا بالعالم"
    const QString arabic = QStringLiteral(u"مرحبا بالعالم");
    const auto r = renderToDocument(arabic);
    QCOMPARE(blockFormatOf(*r.document, arabic).layoutDirection(), Qt::RightToLeft);
}

void RenderedDocumentContractTest::arabicParagraphAfterCodeBlockIsRightToLeft()
{
    // Regression: the RTL stage used to stop detecting after the first code block.
    const QString arabic = QStringLiteral(u"مرحبا بالعالم");
    const auto r = renderToDocument("```\ncode\n```\n\n" + arabic + "\n");
    QCOMPARE(blockFormatOf(*r.document, arabic).layoutDirection(), Qt::RightToLeft);
}

void RenderedDocumentContractTest::arabicQuoteIsRightToLeft()
{
    const QString arabic = QStringLiteral(u"مرحبا بالعالم");
    const auto r = renderToDocument("> " + arabic + "\n");
    QCOMPARE(blockFormatOf(*r.document, arabic).layoutDirection(), Qt::RightToLeft);
}

void RenderedDocumentContractTest::englishParagraphIsNotRightToLeft()
{
    const auto r = renderToDocument("Plain English paragraph.");
    QVERIFY(blockFormatOf(*r.document, "English").layoutDirection() != Qt::RightToLeft);
}

void RenderedDocumentContractTest::taskListShowsBallotBoxes()
{
    const auto r = renderToDocument("- [ ] open\n- [x] done\n");
    const QString text = r.document->toPlainText();
    QVERIFY(text.contains(QStringLiteral(u"☐ open")));
    QVERIFY(text.contains(QStringLiteral(u"☑ done")));
}

void RenderedDocumentContractTest::leadingHorizontalRuleKeepsContent()
{
    const auto r = renderToDocument("---\n\nFirst paragraph.\n\n---\n\nSecond paragraph.\n");
    const QString text = r.document->toPlainText();
    QVERIFY2(text.contains("First paragraph."), "content after a leading rule was dropped");
    QVERIFY(text.contains("Second paragraph."));
}

void RenderedDocumentContractTest::nestedDivKeepsContentOrder()
{
    const auto r = renderToDocument("<div>outer <div>inner</div> after</div>\n\nnext\n");
    const QString text = r.document->toPlainText();
    QVERIFY(text.contains("outer"));
    QVERIFY(text.contains("inner"));
    QVERIFY(text.contains("after"));
    QVERIFY(text.indexOf("after") < text.indexOf("next"));
}

void RenderedDocumentContractTest::definitionListThenParagraphRendersMarkdown()
{
    const auto r = renderToDocument("Term\n: Description\n\nNext **bold** paragraph.\n");
    QVERIFY(!r.document->toPlainText().contains("**"));
    QVERIFY(formatOf(*r.document, "bold").fontWeight() >= QFont::Bold);
}

void RenderedDocumentContractTest::fencedCodeIsShownVerbatim()
{
    const auto r = renderToDocument("~~~\nkey\n: value\n**not bold**\n~~~\n");
    const QString text = r.document->toPlainText();
    QVERIFY(text.contains("key\n: value"));
    QVERIFY(text.contains("**not bold**"));
    QVERIFY(!text.contains("definition"));
}

QTEST_MAIN(RenderedDocumentContractTest)
#include "RenderedDocumentContractTest.moc"
