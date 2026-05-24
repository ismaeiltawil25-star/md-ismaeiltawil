#include <QTest>
#include "renderers/MarkdownHtmlRenderer.h"

class MarkdownHtmlRendererTest : public QObject
{
    Q_OBJECT

private slots:
    void testHeading();
    void testBold();
    void testItalic();
    void testInlineCode();
    void testFencedCodeBlock();
    void testLink();
    void testEmptyInput();
    void testMalformedInput();

    void testBasicTable();
    void testTableHeaderAndBodyCells();
    void testTableSeparatorNotVisible();
    void testAlignmentTable();

    void testFencedCodeCppLanguageClass();
    void testFencedCodePowerShellLanguageClass();
    void testFencedCodeJsonLanguageClass();
    void testFencedCodeYamlLanguageClass();
    void testFencedCodeSqlLanguageClass();
    void testFencedCodeTextNoLanguageClass();

    void testFencedCodeWrappedInCodeBlockTable();
    void testFencedCodePreservesLanguageClass();
    void testFencedCodePreservesContent();
    void testInlineCodeNotWrapped();

    void testBlockquoteWrappedInTable();
    void testBlockquoteContentPreserved();
    void testBlockquoteNestedProducesMultipleWrappers();
    void testBlockquoteContainsInlineElements();
    void testRawHtmlTagsNormalized();
    void testRtlArabicParagraph();
    void testRtlEnglishParagraphUnchanged();
    void testRtlMixedArabicAndEnglish();
    void testRtlArabicListItem();
    void testRtlFencedCodeBlockNotTransformed();
    void testTaskListUnchecked();
    void testTaskListCheckedLowercase();
    void testTaskListCheckedUppercase();
    void testTaskListNormalListItemUnchanged();
};

void MarkdownHtmlRendererTest::testHeading()
{
    auto result = MarkdownHtmlRenderer::render("# Heading 1");
    QVERIFY(result.isValid());
    QVERIFY(result.html.contains("<h1"));
    QVERIFY(result.html.contains("Heading 1"));
}

void MarkdownHtmlRendererTest::testBold()
{
    auto result = MarkdownHtmlRenderer::render("**bold text**");
    QVERIFY(result.isValid());
    QVERIFY(result.html.contains("<strong>"));
    QVERIFY(result.html.contains("bold text"));
}

void MarkdownHtmlRendererTest::testItalic()
{
    auto result = MarkdownHtmlRenderer::render("*italic text*");
    QVERIFY(result.isValid());
    QVERIFY(result.html.contains("<em>"));
    QVERIFY(result.html.contains("italic text"));
}

void MarkdownHtmlRendererTest::testInlineCode()
{
    auto result = MarkdownHtmlRenderer::render("Use `foo()` here.");
    QVERIFY(result.isValid());
    QVERIFY(result.html.contains("<code>"));
    QVERIFY(result.html.contains("foo()"));
}

void MarkdownHtmlRendererTest::testFencedCodeBlock()
{
    auto result = MarkdownHtmlRenderer::render(
        "```\nint x = 42;\n```\n");
    QVERIFY(result.isValid());
    QVERIFY(result.html.contains("<code"));
    QVERIFY(result.html.contains("42"));
}

void MarkdownHtmlRendererTest::testLink()
{
    auto result = MarkdownHtmlRenderer::render(
        "A [link](https://example.com).");
    QVERIFY(result.isValid());
    QVERIFY(result.html.contains("<a "));
    QVERIFY(result.html.contains("https://example.com"));
}

void MarkdownHtmlRendererTest::testEmptyInput()
{
    auto result = MarkdownHtmlRenderer::render(QString());
    QVERIFY(result.isValid());

    result = MarkdownHtmlRenderer::render(QString(""));
    QVERIFY(result.isValid());
}

void MarkdownHtmlRendererTest::testMalformedInput()
{
    const QString malformed =
        ">>>>>>>>> INVALID <<<<<<<<<\n"
        "[unclosed link\n"
        "**unclosed bold\n"
        "`unclosed code\n"
        "||||||||||||||||||\n";

    auto result = MarkdownHtmlRenderer::render(malformed);
    QVERIFY(result.isValid());
    QVERIFY(!result.html.isEmpty());
}

void MarkdownHtmlRendererTest::testBasicTable()
{
    auto result = MarkdownHtmlRenderer::render(
        "| Feature | Status |\n"
        "|---|---|\n"
        "| Rendering | Done |\n");
    QVERIFY(result.isValid());
    QVERIFY2(result.html.contains("<table"),
             "basic pipe table must produce <table");
    QVERIFY2(result.html.contains("</table>"),
             "basic pipe table must produce </table>");
}

void MarkdownHtmlRendererTest::testTableHeaderAndBodyCells()
{
    auto result = MarkdownHtmlRenderer::render(
        "| Name | Value |\n"
        "|---|---|\n"
        "| Alpha | 10 |\n");
    QVERIFY(result.isValid());
    QVERIFY2(result.html.contains("<th"),
             "table must produce <th for header cells");
    QVERIFY2(result.html.contains("<td"),
             "table must produce <td for body cells");
}

void MarkdownHtmlRendererTest::testTableSeparatorNotVisible()
{
    auto result = MarkdownHtmlRenderer::render(
        "| A | B |\n"
        "|---|---|\n"
        "| 1 | 2 |\n");
    QVERIFY(result.isValid());
    QVERIFY2(!result.html.contains("|---|---|---|"),
             "raw separator line must not appear in rendered output");
}

void MarkdownHtmlRendererTest::testAlignmentTable()
{
    auto result = MarkdownHtmlRenderer::render(
        "| Left | Center | Right |\n"
        "|:---|:---:|---:|\n"
        "| L | C | R |\n");
    QVERIFY(result.isValid());
    QVERIFY2(result.html.contains("<table"),
             "alignment table must produce <table");
}

void MarkdownHtmlRendererTest::testFencedCodeCppLanguageClass()
{
    auto result = MarkdownHtmlRenderer::render("```cpp\nint x = 1;\n```\n");
    QVERIFY(result.isValid());
    QVERIFY2(result.html.contains("<pre><code class=\"language-cpp\">"),
             "md4c must emit class=\"language-cpp\" on code element");
    QVERIFY2(result.html.contains("int x = 1;"),
             "code content must be present");
}

void MarkdownHtmlRendererTest::testFencedCodePowerShellLanguageClass()
{
    auto result = MarkdownHtmlRenderer::render("```powershell\nWrite-Host \"Hello\"\n```\n");
    QVERIFY(result.isValid());
    QVERIFY2(result.html.contains("class=\"language-powershell\""),
             "md4c must emit class=\"language-powershell\"");
    QVERIFY2(result.html.contains("Write-Host"),
             "code content must be present");
}

void MarkdownHtmlRendererTest::testFencedCodeJsonLanguageClass()
{
    auto result = MarkdownHtmlRenderer::render("```json\n{ \"x\": 1 }\n```\n");
    QVERIFY(result.isValid());
    QVERIFY2(result.html.contains("class=\"language-json\""),
             "md4c must emit class=\"language-json\"");
}

void MarkdownHtmlRendererTest::testFencedCodeYamlLanguageClass()
{
    auto result = MarkdownHtmlRenderer::render("```yaml\nkey: value\n```\n");
    QVERIFY(result.isValid());
    QVERIFY2(result.html.contains("class=\"language-yaml\""),
             "md4c must emit class=\"language-yaml\"");
}

void MarkdownHtmlRendererTest::testFencedCodeSqlLanguageClass()
{
    auto result = MarkdownHtmlRenderer::render("```sql\nSELECT * FROM t;\n```\n");
    QVERIFY(result.isValid());
    QVERIFY2(result.html.contains("class=\"language-sql\""),
             "md4c must emit class=\"language-sql\"");
}

void MarkdownHtmlRendererTest::testFencedCodeTextNoLanguageClass()
{
    auto result = MarkdownHtmlRenderer::render("```text\nplain\n```\n");
    QVERIFY(result.isValid());
    QVERIFY2(result.html.contains("class=\"language-text\""),
             "md4c must emit class=\"language-text\" even for 'text'");
}

void MarkdownHtmlRendererTest::testFencedCodeWrappedInCodeBlockTable()
{
    auto result = MarkdownHtmlRenderer::render("```cpp\nint x = 1;\n```\n");
    QVERIFY(result.isValid());
    QVERIFY2(result.html.contains("<table class=\"code-block-table\""),
             "fenced code block must be wrapped in code-block-table");
    QVERIFY2(result.html.contains("</table>"),
             "fenced code block must have closing table tag");
}

void MarkdownHtmlRendererTest::testFencedCodePreservesLanguageClass()
{
    auto result = MarkdownHtmlRenderer::render("```cpp\nint x = 1;\n```\n");
    QVERIFY(result.isValid());
    QVERIFY2(result.html.contains("class=\"language-cpp\""),
             "language class must be preserved after wrapping");
}

void MarkdownHtmlRendererTest::testFencedCodePreservesContent()
{
    auto result = MarkdownHtmlRenderer::render("```cpp\nint x = 1;\n```\n");
    QVERIFY(result.isValid());
    QVERIFY2(result.html.contains("int x = 1;"),
             "code content must be preserved after wrapping");
}

void MarkdownHtmlRendererTest::testInlineCodeNotWrapped()
{
    auto result = MarkdownHtmlRenderer::render("Use `foo()` here.");
    QVERIFY(result.isValid());
    QVERIFY2(!result.html.contains("code-block-table"),
             "inline code must NOT be wrapped in code-block-table");
    QVERIFY2(result.html.contains("<code>foo()</code>") ||
             result.html.contains("<code>foo()"),
             "inline code must remain as plain code element");
}

void MarkdownHtmlRendererTest::testBlockquoteWrappedInTable()
{
    auto result = MarkdownHtmlRenderer::render("> A simple quote.\n");
    QVERIFY(result.isValid());
    QVERIFY2(result.html.contains("blockquote-table"),
             "simple blockquote must be wrapped in blockquote-table");
    QVERIFY2(result.html.contains("blockquote-bar"),
             "simple blockquote must contain blockquote-bar");
    QVERIFY2(result.html.contains("blockquote-cell"),
             "simple blockquote must contain blockquote-cell");
    QVERIFY2(result.html.contains("width=\"100%\""),
             "blockquote wrapper must contain width=\"100%\"");
}

void MarkdownHtmlRendererTest::testBlockquoteContentPreserved()
{
    auto result = MarkdownHtmlRenderer::render("> Quote content here.\n");
    QVERIFY(result.isValid());
    QVERIFY2(result.html.contains("Quote content here"),
             "blockquote content must be preserved");
}

void MarkdownHtmlRendererTest::testBlockquoteNestedProducesMultipleWrappers()
{
    auto result = MarkdownHtmlRenderer::render(
        "> Level 1\n"
        ">\n"
        "> > Level 2\n");
    QVERIFY(result.isValid());
    // Each <blockquote> tag gets its own blockquote-table wrapper, so
    // a nested quote with two levels should produce two blockquote-table entries.
    int count = result.html.count("blockquote-table");
    QVERIFY2(count >= 2,
             "nested blockquote must produce at least 2 blockquote-table wrappers");
    QVERIFY2(result.html.contains("Level 1"),
             "Level 1 content preserved in nested blockquote");
    QVERIFY2(result.html.contains("Level 2"),
             "Level 2 content preserved in nested blockquote");
}

void MarkdownHtmlRendererTest::testBlockquoteContainsInlineElements()
{
    auto result = MarkdownHtmlRenderer::render(
        "> Quote with **bold**, *italic*, `code`, and a [link](https://example.com).\n");
    QVERIFY(result.isValid());
    QVERIFY2(result.html.contains("width=\"100%\""),
             "blockquote wrapper must include width=\"100%\" attribute");
    QVERIFY2(result.html.contains("bold"),
             "bold content inside blockquote must be preserved");
    QVERIFY2(result.html.contains("<strong>") || result.html.contains("<strong"),
             "bold markdown inside blockquote must produce <strong>");
    QVERIFY2(result.html.contains("italic"),
             "italic content inside blockquote must be preserved");
    QVERIFY2(result.html.contains("<em>") || result.html.contains("<em"),
             "italic markdown inside blockquote must produce <em>");
    QVERIFY2(result.html.contains("<code>") || result.html.contains("<code"),
             "inline code inside blockquote must produce <code>");
    QVERIFY2(result.html.contains("<a ") || result.html.contains("href="),
             "link inside blockquote must produce <a>");
    QVERIFY2(result.html.contains("https://example.com"),
             "link URL inside blockquote must be present");
}

void MarkdownHtmlRendererTest::testRawHtmlTagsNormalized()
{
    auto result = MarkdownHtmlRenderer::render(
        "Some <kbd>Ctrl</kbd> + <kbd>O</kbd>, <mark>highlight</mark>, "
        "H<sub>2</sub>O, E=mc<sup>2</sup>.\n\n"
        "<details>\n<summary>Title</summary>\n\nContent\n\n</details>\n\n"
        "<div style=\"border:1px solid #888; padding:8px;\">\n"
        "  <strong>HTML block:</strong> raw div.\n"
        "</div>\n");
    QVERIFY(result.isValid());

    // kbd → <code>
    QVERIFY2(!result.html.contains("<kbd>"), "<kbd> must be replaced");
    QVERIFY2(result.html.contains("<code>Ctrl</code>"),
             "kbd must normalize to <code>Ctrl</code>");

    // mark → inline-styled span with amber highlight
    QVERIFY2(!result.html.contains("<mark>"), "<mark> must be replaced");
    QVERIFY2(result.html.contains("background-color: #FFF3B0"),
             "mark must produce inline style with amber highlight");

    // summary → p.html-summary
    QVERIFY2(!result.html.contains("<summary>"), "<summary> must be replaced");
    QVERIFY2(result.html.contains("<p class=\"html-summary\">"),
             "summary must become p.html-summary");

    // details stripped
    QVERIFY2(!result.html.contains("<details>"), "<details> must be stripped");
    QVERIFY2(!result.html.contains("</details>"), "</details> must be stripped");

    // div → table wrapper
    QVERIFY2(!result.html.contains("<div "), "<div must be replaced");
    QVERIFY2(result.html.contains("html-block-table"),
             "div must become html-block-table");
    QVERIFY2(result.html.contains("html-block-cell"),
             "div must become html-block-cell");

    // sub/sup unchanged
    QVERIFY2(result.html.contains("<sub>"), "<sub> must survive");
    QVERIFY2(result.html.contains("<sup>"), "<sup> must survive");

    // Content preserved
    QVERIFY2(result.html.contains("Ctrl"), "kbd content preserved");
    QVERIFY2(result.html.contains("O"), "kbd content preserved");
    QVERIFY2(result.html.contains("highlight"), "mark content preserved");
    QVERIFY2(result.html.contains("Title"), "summary content preserved");
    QVERIFY2(result.html.contains("Content"), "details content preserved");
    QVERIFY2(result.html.contains("HTML block"), "div content preserved");
}

void MarkdownHtmlRendererTest::testRtlArabicParagraph()
{
    auto result = MarkdownHtmlRenderer::render("هذه فقرة عربية.\n");
    QVERIFY(result.isValid());
    QVERIFY2(result.html.contains("dir=\"rtl\""),
             "Arabic paragraph must get dir=\"rtl\"");
    QVERIFY2(result.html.contains("align=\"right\""),
             "Arabic paragraph must get align=\"right\"");
}

void MarkdownHtmlRendererTest::testRtlEnglishParagraphUnchanged()
{
    auto result = MarkdownHtmlRenderer::render("This is an English paragraph.\n");
    QVERIFY(result.isValid());
    QVERIFY2(!result.html.contains("dir=\"rtl\""),
             "English paragraph must NOT have dir=\"rtl\"");
}

void MarkdownHtmlRendererTest::testRtlMixedArabicAndEnglish()
{
    auto result = MarkdownHtmlRenderer::render(
        "هذا نص عربي مع English و `inline code`.\n");
    QVERIFY(result.isValid());
    QVERIFY2(result.html.contains("dir=\"rtl\""),
             "Mixed Arabic/English paragraph must get dir=\"rtl\"");
    QVERIFY2(result.html.contains("<code>"),
             "Inline code inside Arabic paragraph must be preserved");
}

void MarkdownHtmlRendererTest::testRtlArabicListItem()
{
    auto result = MarkdownHtmlRenderer::render("- عنصر أول\n- عنصر ثانٍ\n");
    QVERIFY(result.isValid());
    QVERIFY2(result.html.contains("dir=\"rtl\""),
             "Arabic list item must get dir=\"rtl\"");
}

void MarkdownHtmlRendererTest::testRtlFencedCodeBlockNotTransformed()
{
    auto result = MarkdownHtmlRenderer::render(
        "```text\nمثال كود نصي عربي\n```\n");
    QVERIFY(result.isValid());
    // Code block content should NOT have dir="rtl"
    // Check that dir="rtl" does NOT appear in the code block section
    int prePos = result.html.indexOf("<pre");
    int preClose = result.html.indexOf("</pre>", prePos);
    if (prePos >= 0 && preClose > prePos) {
        QString codeSection = result.html.mid(prePos, preClose - prePos + 6);
        QVERIFY2(!codeSection.contains("dir=\"rtl\""),
                 "Fenced code block must NOT have dir=\"rtl\"");
    }
}

void MarkdownHtmlRendererTest::testTaskListUnchecked()
{
    auto result = MarkdownHtmlRenderer::render("- [ ] Unchecked item\n");
    QVERIFY(result.isValid());
    QVERIFY2(!result.html.contains("[ ]"), "unchecked marker must not contain raw '[ ]'");
    QVERIFY2(!result.html.contains("<input"), "unchecked must replace <input>");
    QVERIFY2(result.html.contains("☐"), "unchecked must contain ☐ marker");
    QVERIFY2(result.html.contains("Unchecked item"), "unchecked item content preserved");
}

void MarkdownHtmlRendererTest::testTaskListCheckedLowercase()
{
    auto result = MarkdownHtmlRenderer::render("- [x] Checked item\n");
    QVERIFY(result.isValid());
    QVERIFY2(!result.html.contains("[x]"), "checked [x] must not remain raw");
    QVERIFY2(!result.html.contains("<input"), "checked must replace <input>");
    QVERIFY2(result.html.contains("☑"), "checked must contain ☑ marker");
    QVERIFY2(result.html.contains("Checked item"), "checked item content preserved");
}

void MarkdownHtmlRendererTest::testTaskListCheckedUppercase()
{
    auto result = MarkdownHtmlRenderer::render("- [X] Checked uppercase\n");
    QVERIFY(result.isValid());
    QVERIFY2(!result.html.contains("[X]"), "checked [X] must not remain raw");
    QVERIFY2(result.html.contains("☑"), "checked uppercase must contain ☑ marker");
}

void MarkdownHtmlRendererTest::testTaskListNormalListItemUnchanged()
{
    auto result = MarkdownHtmlRenderer::render("- Normal list item\n");
    QVERIFY(result.isValid());
    QVERIFY2(!result.html.contains("task-list-item"), "normal item must not get task class");
    QVERIFY2(result.html.contains("Normal list item"), "normal item content preserved");
}

QTEST_MAIN(MarkdownHtmlRendererTest)
#include "MarkdownHtmlRendererTest.moc"
