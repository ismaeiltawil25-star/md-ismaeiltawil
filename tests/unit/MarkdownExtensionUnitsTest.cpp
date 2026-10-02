// Unit tests for each Markdown extension stage in isolation.
// End-to-end behaviour is covered by MarkdownFeatureCoverageTest; this file
// pins down edge cases so each unit can be changed safely on its own.
//
// Non-ASCII expectations are written as escapes (see SourceHygieneTest).

#include <QTest>

#include "renderers/extensions/AlertRenderer.h"
#include "renderers/extensions/EmojiShortcodes.h"
#include "renderers/extensions/HeadingAnchors.h"
#include "renderers/extensions/HtmlEntities.h"
#include "renderers/extensions/HtmlTextRuns.h"
#include "renderers/extensions/InlineExtensions.h"
#include "renderers/extensions/LatexMathConverter.h"
#include "renderers/extensions/LinkAndMathRenderers.h"
#include "renderers/extensions/SyntaxHighlighter.h"
#include "services/markdown/FootnoteProcessor.h"
#include "services/markdown/InlineCodeSpans.h"

using namespace mdbr::render;
using mdbr::markdown::processFootnotes;

class MarkdownExtensionUnitsTest : public QObject
{
    Q_OBJECT

private slots:
    // HtmlTextRuns
    void textRunsSkipProtectedElements();
    void textRunsHandleMalformedHtml();

    // InlineExtensions
    void highlightRules_data();
    void highlightRules();
    void superscriptRules_data();
    void superscriptRules();
    void emojiKnownAndUnknown();
    void emojiTableIsSane();

    // LaTeX
    void latexConversions_data();
    void latexConversions();
    void latexDeepNestingIsSafe();
    void latexUnknownCommandStaysVisible();

    // Wiki links / math / alerts / anchors
    void wikiLinkTargets_data();
    void wikiLinkTargets();
    void displayMathIsCentred();
    void alertTypes_data();
    void alertTypes();
    void plainQuoteIsNotAnAlert();
    void slugRules_data();
    void slugRules();
    void duplicateHeadingsGetUniqueAnchors();

    // Syntax highlighting
    void highlightsKnownLanguage();
    void unknownLanguageStaysPlain();
    void highlightingPreservesText();
    void hugeBlockIsNotHighlighted();

    // Footnotes and code spans
    void footnoteBasics();
    void footnoteOrderFollowsFirstUse();
    void footnoteMultiLine();
    void footnoteUndefinedReferenceIsKept();
    void footnoteUnreferencedDefinitionIsKept();
    void footnoteInCodeIsNotTouched();
    void codeSpanRanges();
};

// --- HtmlTextRuns -------------------------------------------------------------

void MarkdownExtensionUnitsTest::textRunsSkipProtectedElements()
{
    const QString html = "<p>a <code>b</code> <a href=\"x\">c</a> <pre>d</pre> e</p>";
    const QString out = transformTextRuns(html, [](QStringView t) { return t.toString().toUpper(); });
    QCOMPARE(out, QString("<p>A <code>b</code> <a href=\"x\">c</a> <pre>d</pre> E</p>"));
}

void MarkdownExtensionUnitsTest::textRunsHandleMalformedHtml()
{
    // Unclosed tag at the end and stray closers must not lose or duplicate text.
    const QString out = transformTextRuns("x</code>y<b", [](QStringView t) { return t.toString(); });
    QCOMPARE(out, QString("x</code>y<b"));
}

// --- InlineExtensions ---------------------------------------------------------

void MarkdownExtensionUnitsTest::highlightRules_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<bool>("highlighted");
    QTest::newRow("simple")         << "==hi=="        << true;
    QTest::newRow("two words")      << "==hi there=="  << true;
    QTest::newRow("comparison")     << "a == b"        << false;
    QTest::newRow("space inside")   << "== hi =="      << false;
    QTest::newRow("triple equals")  << "===x==="       << false;
    QTest::newRow("unclosed")       << "==open"        << false;
}

void MarkdownExtensionUnitsTest::highlightRules()
{
    QFETCH(QString, input);
    QFETCH(bool, highlighted);
    QCOMPARE(applyHighlight(input).contains("background-color"), highlighted);
}

void MarkdownExtensionUnitsTest::superscriptRules_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");
    QTest::newRow("power")          << "mc^2^"       << "mc<sup>2</sup>";
    QTest::newRow("two")            << "2^10^ and x^n^" << "2<sup>10</sup> and x<sup>n</sup>";
    QTest::newRow("caret math")     << "x^2 + y^3"   << "x^2 + y^3";
    QTest::newRow("space inside")   << "a ^b c^"     << "a ^b c^";
}

void MarkdownExtensionUnitsTest::superscriptRules()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);
    QCOMPARE(applySuperscript(input), expected);
}

void MarkdownExtensionUnitsTest::emojiKnownAndUnknown()
{
    QCOMPARE(applyEmojiShortcodes(QStringLiteral(":rocket: go")), QStringLiteral(u"\U0001F680 go"));
    QCOMPARE(applyEmojiShortcodes(QStringLiteral(":+1:")), QStringLiteral(u"\U0001F44D"));
    QCOMPARE(applyEmojiShortcodes(QStringLiteral(":not_an_emoji:")), QStringLiteral(":not_an_emoji:"));
    QCOMPARE(applyEmojiShortcodes(QStringLiteral("at 10:30:45")), QStringLiteral("at 10:30:45"));
}

void MarkdownExtensionUnitsTest::emojiTableIsSane()
{
    QVERIFY(emojiShortcodeCount() > 150);
    QVERIFY(emojiForShortcode(u"smile").has_value());
    QVERIFY(!emojiForShortcode(u"").has_value());
}

// --- LaTeX --------------------------------------------------------------------

void MarkdownExtensionUnitsTest::latexConversions_data()
{
    QTest::addColumn<QString>("latex");
    QTest::addColumn<QString>("contains");
    QTest::newRow("greek")        << "\\alpha + \\beta"   << QStringLiteral(u"\u03B1");
    QTest::newRow("power")        << "x^2"                << "x<sup>2</sup>";
    QTest::newRow("group power")  << "e^{i\\pi}"          << QStringLiteral(u"e<sup>i\u03C0</sup>");
    QTest::newRow("subscript")    << "a_{n+1}"            << "a<sub>n";
    QTest::newRow("fraction")     << "\\frac{1}{2}"       << QStringLiteral(u"<sup>1</sup>\u2044<sub>2</sub>");
    QTest::newRow("sqrt")         << "\\sqrt{x}"          << QStringLiteral(u"\u221A");
    QTest::newRow("sum")          << "\\sum_{i=1}^{n}"    << QStringLiteral(u"\u2211<sub>i");
    QTest::newRow("reals")        << "\\mathbb{R}"        << QStringLiteral(u"\u211D");
    QTest::newRow("text")         << "\\text{if } x"      << "if ";
    QTest::newRow("relation")     << "a \\le b"           << QStringLiteral(u"\u2264");
    QTest::newRow("escape html")  << "a < b"              << "&lt;";
}

void MarkdownExtensionUnitsTest::latexConversions()
{
    QFETCH(QString, latex);
    QFETCH(QString, contains);
    const QString html = latexToHtml(latex);
    QVERIFY2(html.contains(contains), qPrintable(html));
}

void MarkdownExtensionUnitsTest::latexDeepNestingIsSafe()
{
    // Must not overflow the stack; content stays visible.
    const QString deep = QString(100'000, u'{') + "x" + QString(100'000, u'}');
    const QString html = latexToHtml(deep);
    QVERIFY(html.contains(u'x'));
}

void MarkdownExtensionUnitsTest::latexUnknownCommandStaysVisible()
{
    QVERIFY(latexToHtml(QStringLiteral("\\weirdcommand{x}")).contains("\\weirdcommand"));
}

// --- Wiki links / math / alerts / anchors --------------------------------------

void MarkdownExtensionUnitsTest::wikiLinkTargets_data()
{
    QTest::addColumn<QString>("target");
    QTest::addColumn<QString>("href");
    QTest::newRow("page")          << "Other Page"      << "Other Page.md";
    QTest::newRow("with ext")      << "notes.txt"       << "notes.txt";
    QTest::newRow("page section")  << "Guide#Install It" << "Guide.md#install-it";
    QTest::newRow("same page")     << "#Usage"          << "#usage";
}

void MarkdownExtensionUnitsTest::wikiLinkTargets()
{
    QFETCH(QString, target);
    QFETCH(QString, href);
    const QString html = renderWikiLinks(QString("<x-wikilink data-target=\"%1\">L</x-wikilink>").arg(target));
    QCOMPARE(html, QString("<a href=\"%1\">L</a>").arg(href));
}

void MarkdownExtensionUnitsTest::displayMathIsCentred()
{
    const QString html = renderMath("<p><x-equation type=\"display\">x^2</x-equation></p>");
    QVERIFY(html.startsWith("<p align=\"center\""));
    QVERIFY(html.contains("x<sup>2</sup>"));
    QVERIFY(!html.contains("x-equation"));
}

void MarkdownExtensionUnitsTest::alertTypes_data()
{
    QTest::addColumn<QString>("marker");
    QTest::addColumn<QString>("title");
    QTest::newRow("note")      << "NOTE"      << "Note";
    QTest::newRow("tip")       << "TIP"       << "Tip";
    QTest::newRow("important") << "IMPORTANT" << "Important";
    QTest::newRow("warning")   << "WARNING"   << "Warning";
    QTest::newRow("caution")   << "caution"   << "Caution";
}

void MarkdownExtensionUnitsTest::alertTypes()
{
    QFETCH(QString, marker);
    QFETCH(QString, title);
    const QString html = renderAlerts(QString("<blockquote>\n<p>[!%1]\nBody</p>\n</blockquote>").arg(marker));
    QVERIFY(html.contains("data-alert-color=\"#"));
    QVERIFY(html.contains(title));
    QVERIFY(html.contains("Body"));
    QVERIFY(!html.contains("[!"));
}

void MarkdownExtensionUnitsTest::plainQuoteIsNotAnAlert()
{
    const QString html = "<blockquote>\n<p>[!UNKNOWN] text</p>\n</blockquote>";
    QCOMPARE(renderAlerts(html), html);
}

void MarkdownExtensionUnitsTest::slugRules_data()
{
    QTest::addColumn<QString>("heading");
    QTest::addColumn<QString>("slug");
    QTest::newRow("words")       << "Getting Started"        << "getting-started";
    QTest::newRow("punctuation") << "What's new? (v2.0)"     << "whats-new-v20";
    QTest::newRow("keeps dash")  << "pre-release_notes"      << "pre-release_notes";
    QTest::newRow("arabic")      << QStringLiteral(u"\u0645\u0642\u062F\u0645\u0629 \u0639\u0627\u0645\u0629")
                                 << QStringLiteral(u"\u0645\u0642\u062F\u0645\u0629-\u0639\u0627\u0645\u0629");
}

void MarkdownExtensionUnitsTest::slugRules()
{
    QFETCH(QString, heading);
    QFETCH(QString, slug);
    QCOMPARE(slugify(heading), slug);
}

void MarkdownExtensionUnitsTest::duplicateHeadingsGetUniqueAnchors()
{
    const QString html = addHeadingAnchors("<h2>Intro</h2><h2>Intro</h2><h3>Intro</h3>");
    QVERIFY(html.contains("name=\"intro\""));
    QVERIFY(html.contains("name=\"intro-1\""));
    QVERIFY(html.contains("name=\"intro-2\""));
}

// --- Syntax highlighting -------------------------------------------------------

void MarkdownExtensionUnitsTest::highlightsKnownLanguage()
{
    const QString html = highlightCodeBlocks(
        "<pre><code class=\"language-python\">def f(x):  # note\n    return &quot;hi&quot;\n</code></pre>");
    QVERIFY(html.contains("<span class=\"tok-keyword\">def</span>"));
    QVERIFY(html.contains("<span class=\"tok-comment\"># note</span>"));
    QVERIFY(html.contains("<span class=\"tok-string\">&quot;hi&quot;</span>"));
}

void MarkdownExtensionUnitsTest::unknownLanguageStaysPlain()
{
    const QString html = "<pre><code class=\"language-brainfuck\">+++.</code></pre>";
    QCOMPARE(highlightCodeBlocks(html), html);
}

void MarkdownExtensionUnitsTest::highlightingPreservesText()
{
    // Removing the spans must give back exactly the original (escaped) code.
    const QString code = "#include &lt;vector&gt;\nint main() { return 0x1F; } // ok\n";
    const QString html = highlightCodeBlocks("<pre><code class=\"language-cpp\">" + code + "</code></pre>");
    QString text = html;
    text.remove(QRegularExpression("<span class=\"tok-[a-z]+\">"));
    text.remove("</span>");
    QCOMPARE(text, "<pre><code class=\"language-cpp\">" + code + "</code></pre>");
}

void MarkdownExtensionUnitsTest::hugeBlockIsNotHighlighted()
{
    const QString code = QString("int x;\n").repeated(int(kMaxHighlightChars / 7) + 10);
    const QString html = "<pre><code class=\"language-cpp\">" + code + "</code></pre>";
    QCOMPARE(highlightCodeBlocks(html), html);
}

// --- Footnotes ------------------------------------------------------------------

void MarkdownExtensionUnitsTest::footnoteBasics()
{
    const QString out = processFootnotes("Claim[^a].\n\n[^a]: Source.\n");
    QVERIFY(out.contains("<sup><a href=\"#fn-1\" name=\"fnref-1\">1</a></sup>"));
    QVERIFY(out.contains("1. <a name=\"fn-1\"></a>Source."));
    QVERIFY(out.contains("href=\"#fnref-1\""));
    QVERIFY(!out.contains("[^a]"));
}

void MarkdownExtensionUnitsTest::footnoteOrderFollowsFirstUse()
{
    const QString out = processFootnotes("B[^b] A[^a] B again[^b]\n\n[^a]: Alpha\n[^b]: Beta\n");
    QVERIFY(out.indexOf("1. <a name=\"fn-1\"></a>Beta") >= 0);
    QVERIFY(out.indexOf("2. <a name=\"fn-2\"></a>Alpha") >= 0);
    QCOMPARE(out.count("name=\"fnref-1\""), 1); // only the first use gets the back-link target
}

void MarkdownExtensionUnitsTest::footnoteMultiLine()
{
    const QString out = processFootnotes("X[^n]\n\n[^n]: First line\n    second line\n\n    second paragraph\n\nAfter.\n");
    QVERIFY(out.contains("First line"));
    QVERIFY(out.contains("    second line"));
    QVERIFY(out.contains("    second paragraph"));
    QVERIFY(out.indexOf("After.") < out.indexOf("***")); // body text stays before the notes
}

void MarkdownExtensionUnitsTest::footnoteUndefinedReferenceIsKept()
{
    const QString input = "Missing[^none] here.\n\n[^x]: unused\n";
    QCOMPARE(processFootnotes(input), input);
}

void MarkdownExtensionUnitsTest::footnoteUnreferencedDefinitionIsKept()
{
    const QString out = processFootnotes("Used[^u].\n\n[^u]: yes\n[^z]: never used\n");
    QVERIFY(out.contains("[^z]: never used")); // no silent content loss
}

void MarkdownExtensionUnitsTest::footnoteInCodeIsNotTouched()
{
    const QString out = processFootnotes("Real[^r] and `code[^r]`\n\n```\nfence[^r]\n```\n\n[^r]: Note\n");
    QVERIFY(out.contains("`code[^r]`"));
    QVERIFY(out.contains("fence[^r]"));
    QVERIFY(out.contains("<sup>"));
}

void MarkdownExtensionUnitsTest::codeSpanRanges()
{
    using mdbr::markdown::codeSpanRanges;
    QCOMPARE(codeSpanRanges(u"a `b` c").size(), 1);
    QCOMPARE(codeSpanRanges(u"``a ` b`` c").size(), 1);   // longer fence contains a backtick
    QCOMPARE(codeSpanRanges(u"unclosed `here").size(), 0);
    QCOMPARE(codeSpanRanges(u"\\`not code`").size(), 0);  // escaped opener
}

QTEST_APPLESS_MAIN(MarkdownExtensionUnitsTest)
#include "MarkdownExtensionUnitsTest.moc"
