#include <QtTest/QtTest>
#include <QString>

#include "renderers/MarkdownHtmlRenderer.h"
#include "services/MarkdownPreprocessor.h"

class MarkdownRenderContractTest : public QObject
{
    Q_OBJECT

private:
    static QString renderThroughMainWindowPipeline(const QString &markdown)
    {
        const QString cleaned =
            MarkdownPreprocessor::stripYamlFrontMatter(markdown);

        const QString withDefinitions =
            MarkdownPreprocessor::convertDefinitionLists(cleaned);

        const auto result =
            MarkdownHtmlRenderer::render(withDefinitions);

        return result.html;
    }

private slots:
    void frontMatterIsStrippedAndContentIsNotSubstituted()
    {
        const QString markdown = QStringLiteral(R"MD(---
title: "اختبار يشمل تيرات Markdown"
author: "Markdown Renderer Contract"
date: 2026-05-24
language: ar
---

# MD Backpack Reader — اختبار شامل

## 1. بني داكن / العناوين

# العنوان 1 - العنوان الرئيسي
## العنوان 2 — عنوان فرعي
### Heading 3 — مستوى ثالث
#### العنوان 4 — المستوى الرابع
##### العنوان 5 – جندي الجندي
###### العنوان 6 — مستوى سادس
)MD");

        const QString html = renderThroughMainWindowPipeline(markdown);

        // Front matter must not leak into rendered HTML.
        QVERIFY2(!html.contains(QStringLiteral("author:")),
                 "YAML front matter leaked into rendered HTML.");
        QVERIFY2(!html.contains(QStringLiteral("Markdown Renderer Contract")),
                 "YAML front matter value leaked into rendered HTML.");
        QVERIFY2(!html.contains(QStringLiteral("language: ar")),
                 "YAML front matter language field leaked into rendered HTML.");

        // Exact source text must survive rendering.
        QVERIFY2(html.contains(QStringLiteral("MD Backpack Reader — اختبار شامل")),
                 "Main document title was lost or substituted.");

        QVERIFY2(html.contains(QStringLiteral("1. بني داكن / العناوين")),
                 "Section heading text was changed, translated, or loaded from a stale fixture.");

        QVERIFY2(html.contains(QStringLiteral("العنوان 1 - العنوان الرئيسي")),
                 "H1 text was changed or substituted.");
        QVERIFY2(html.contains(QStringLiteral("العنوان 2 — عنوان فرعي")),
                 "H2 text was changed or substituted.");
        QVERIFY2(html.contains(QStringLiteral("Heading 3 — مستوى ثالث")),
                 "H3 mixed RTL/LTR text was changed or substituted.");
        QVERIFY2(html.contains(QStringLiteral("العنوان 4 — المستوى الرابع")),
                 "H4 text was changed or substituted.");
        QVERIFY2(html.contains(QStringLiteral("العنوان 5 – جندي الجندي")),
                 "H5 text was changed or substituted.");
        QVERIFY2(html.contains(QStringLiteral("العنوان 6 — مستوى سادس")),
                 "H6 text was changed or substituted.");

        // Guard against the stale/translated content seen in the screenshot.
        QVERIFY2(!html.contains(QStringLiteral("1. العناوين / Headings")),
                 "Renderer appears to be using a stale or substituted section title.");
        QVERIFY2(!html.contains(QStringLiteral("Heading 1 — العنوان الرئيسي")),
                 "Renderer appears to be substituting H1 text.");
        QVERIFY2(!html.contains(QStringLiteral("Heading 5 — مستوى خامس")),
                 "Renderer appears to be substituting H5 text.");
    }

    void headingsRenderAsRealHeadingElements()
    {
        const QString markdown = QStringLiteral(R"MD(
# العنوان 1 - العنوان الرئيسي
## العنوان 2 — عنوان فرعي
### Heading 3 — مستوى ثالث
#### العنوان 4 — المستوى الرابع
##### العنوان 5 – جندي الجندي
###### العنوان 6 — مستوى سادس
)MD");

        const QString html = renderThroughMainWindowPipeline(markdown);

        QVERIFY2(html.contains(QStringLiteral("<h1")),
                 "Markdown # did not render as h1.");
        QVERIFY2(html.contains(QStringLiteral("<h2")),
                 "Markdown ## did not render as h2.");
        QVERIFY2(html.contains(QStringLiteral("<h3")),
                 "Markdown ### did not render as h3.");
        QVERIFY2(html.contains(QStringLiteral("<h4")),
                 "Markdown #### did not render as h4.");
        QVERIFY2(html.contains(QStringLiteral("<h5")),
                 "Markdown ##### did not render as h5.");
        QVERIFY2(html.contains(QStringLiteral("<h6")),
                 "Markdown ###### did not render as h6.");

        QVERIFY2(html.contains(QStringLiteral("العنوان 1 - العنوان الرئيسي")),
                 "h1 text did not remain exact.");
        QVERIFY2(html.contains(QStringLiteral("العنوان 2 — عنوان فرعي")),
                 "h2 text did not remain exact.");
        QVERIFY2(html.contains(QStringLiteral("Heading 3 — مستوى ثالث")),
                 "h3 text did not remain exact.");
        QVERIFY2(html.contains(QStringLiteral("العنوان 4 — المستوى الرابع")),
                 "h4 text did not remain exact.");
        QVERIFY2(html.contains(QStringLiteral("العنوان 5 – جندي الجندي")),
                 "h5 text did not remain exact.");
        QVERIFY2(html.contains(QStringLiteral("العنوان 6 — مستوى سادس")),
                 "h6 text did not remain exact.");
    }

    void rtlBlocksReceiveDirectionAttributes()
    {
        const QString markdown = QStringLiteral(R"MD(
# عنوان عربي

هذه فقرة عربية لاختبار الاتجاه من اليمين إلى اليسار.

- عنصر عربي أول
- عنصر عربي ثاني
)MD");

        const QString html = renderThroughMainWindowPipeline(markdown);

        QVERIFY2(html.contains(QStringLiteral("dir=\"rtl\"")),
                 "RTL direction attribute was not added to Arabic block content.");

        QVERIFY2(html.contains(QStringLiteral("align=\"right\"")),
                 "Arabic block content was not right-aligned.");
    }

    void rtlBlockquotePlacesBarOnRightSide()
    {
        const QString markdown = QStringLiteral(R"MD(
> هذا اقتباس عربي لاختبار اتجاه RTL.
> يجب أن يظهر شريط الاقتباس في جهة اليمين.
)MD");

        const QString html = renderThroughMainWindowPipeline(markdown);

        QVERIFY2(html.contains(QStringLiteral("blockquote-bar")),
                 "Blockquote wrapper did not create a blockquote-bar.");

        QVERIFY2(html.contains(QStringLiteral("blockquote-cell")),
                 "Blockquote wrapper did not create a blockquote-cell.");

        QVERIFY2(html.contains(QStringLiteral("dir=\"rtl\"")),
                 "RTL direction was not applied to Arabic blockquote content.");

        /*
         Contract after the fix:
         For RTL blockquotes, the content cell must appear first and the visual bar
         must appear after it, so QTextBrowser renders the bar on the right side.

         This intentionally fails before the fix if HtmlElementWrapper always emits:
         <td class="blockquote-bar"> ... then <td class="blockquote-cell">
        */
        const qsizetype cellIndex = html.indexOf(QStringLiteral("blockquote-cell"));
        const qsizetype barIndex = html.indexOf(QStringLiteral("blockquote-bar"));

        QVERIFY2(cellIndex >= 0, "blockquote-cell was not found.");
        QVERIFY2(barIndex >= 0, "blockquote-bar was not found.");

        QVERIFY2(cellIndex < barIndex,
                 "RTL blockquote bar is still emitted before the content cell; "
                 "for RTL it must be emitted after the content cell so the bar appears on the right.");
    }

    void codeBlocksDoNotReceiveRtlDirection()
    {
        const QString markdown = QStringLiteral(R"MD(
```cpp
QString text = QStringLiteral("نص عربي داخل كود");
int value = 42;
```
)MD");

        const QString html = renderThroughMainWindowPipeline(markdown);

        // Code block must be wrapped in code-block-table
        QVERIFY2(html.contains(QStringLiteral("code-block-table")),
                 "Fenced code block was not wrapped in code-block-table.");

        // dir="rtl" must NOT appear anywhere inside the code block section
        int prePos = html.indexOf(QStringLiteral("<pre"));
        int preClose = html.indexOf(QStringLiteral("</pre>"), prePos);
        if (prePos >= 0 && preClose > prePos) {
            QString codeSection = html.mid(prePos, preClose - prePos + 6);
            QVERIFY2(!codeSection.contains(QStringLiteral("dir=\"rtl\"")),
                     "Code block must NOT have dir=\"rtl\" even if it contains Arabic text.");
        }

        // Code content must be preserved
        QVERIFY2(html.contains(QStringLiteral("نص عربي داخل كود")),
                 "Arabic text inside code block was lost.");
        QVERIFY2(html.contains(QStringLiteral("int value = 42;")),
                 "C++ code content was lost.");
    }

    void taskListsProduceUnicodeMarkers()
    {
        const QString markdown = QStringLiteral(R"MD(
- [x] مهمة مكتملة
- [ ] مهمة غير مكتملة
)MD");

        const QString html = renderThroughMainWindowPipeline(markdown);

        QVERIFY2(!html.contains(QStringLiteral("<input")),
                 "Task list checkbox input elements must be replaced with Unicode markers.");
        QVERIFY2(html.contains(QStringLiteral("☑")),
                 "Checked task list item must show ☑ marker.");
        QVERIFY2(html.contains(QStringLiteral("☐")),
                 "Unchecked task list item must show ☐ marker.");
        QVERIFY2(html.contains(QStringLiteral("مهمة مكتملة")),
                 "Checked task content was lost.");
        QVERIFY2(html.contains(QStringLiteral("مهمة غير مكتملة")),
                 "Unchecked task content was lost.");
    }

    void rawHtmlTagsAreNormalized()
    {
        const QString markdown = QStringLiteral(R"MD(
Press <kbd>Ctrl</kbd> + <kbd>O</kbd> to open a file.

This is <mark>highlighted text</mark>.

<details>
<summary>Click to reveal</summary>
Hidden content.
</details>

<div style="border:1px solid #ccc;">
Custom block content.
</div>

H<sub>2</sub>O and E=mc<sup>2</sup>.
)MD");

        const QString html = renderThroughMainWindowPipeline(markdown);

        // kbd must be replaced with code
        QVERIFY2(!html.contains(QStringLiteral("<kbd>")),
                 "<kbd> must be normalized to <code>.");
        QVERIFY2(html.contains(QStringLiteral("<code>Ctrl</code>")),
                 "<kbd> content must be wrapped in <code>.");

        // mark must produce amber highlight span
        QVERIFY2(!html.contains(QStringLiteral("<mark>")),
                 "<mark> must be replaced with styled span.");
        QVERIFY2(html.contains(QStringLiteral("background-color: #FFF3B0")),
                 "<mark> must produce amber background style.");

        // details must be stripped
        QVERIFY2(!html.contains(QStringLiteral("<details>")),
                 "<details> must be stripped from rendered HTML.");
        QVERIFY2(!html.contains(QStringLiteral("</details>")),
                 "</details> must be stripped from rendered HTML.");

        // summary must become p.html-summary
        QVERIFY2(!html.contains(QStringLiteral("<summary>")),
                 "<summary> must be replaced.");
        QVERIFY2(html.contains(QStringLiteral("html-summary")),
                 "<summary> must become a p.html-summary element.");

        // div must become table wrapper
        QVERIFY2(html.contains(QStringLiteral("html-block-table")),
                 "<div> must be wrapped in html-block-table.");
        QVERIFY2(html.contains(QStringLiteral("html-block-cell")),
                 "<div> must be wrapped in html-block-cell.");

        // sub/sup must survive
        QVERIFY2(html.contains(QStringLiteral("<sub>")),
                 "<sub> must survive normalization.");
        QVERIFY2(html.contains(QStringLiteral("<sup>")),
                 "<sup> must survive normalization.");
    }

    void tableRendersCorrectly()
    {
        const QString markdown = QStringLiteral(R"MD(
| الميزة | الحالة |
|--------|--------|
| العناوين | ✅ |
| الجداول | ✅ |
)MD");

        const QString html = renderThroughMainWindowPipeline(markdown);

        QVERIFY2(html.contains(QStringLiteral("<table")),
                 "GFM table must render as <table> element.");
        QVERIFY2(html.contains(QStringLiteral("العناوين")),
                 "Table content must be preserved.");
        QVERIFY2(html.contains(QStringLiteral("✅")),
                 "Table cell with emoji must be preserved.");
    }
};

QTEST_MAIN(MarkdownRenderContractTest)
#include "MarkdownRenderContractTest.moc"
