#include <QTest>
#include "HtmlTestHelpers.h"
#include <QFile>
#include "renderers/MarkdownHtmlRenderer.h"
#include "services/MarkdownPreprocessor.h"
#include "services/DocumentLoader.h"

class ComprehensiveRenderTest : public QObject
{
    Q_OBJECT

private slots:
    void testRenderAllFeatures();
    void testArabicTextPreserved();
    void testCodeBlocksPreserved();
    void testTaskLists();
};

void ComprehensiveRenderTest::testRenderAllFeatures()
{
    QString path = QString(TEST_DATA_DIR) + "/comprehensive-test.md";
    auto loadResult = DocumentLoader::load(path);
    QVERIFY2(loadResult.isValid(), "comprehensive-test.md must load successfully");

    QString cleaned = MarkdownPreprocessor::stripYamlFrontMatter(loadResult.document.rawText);
    QString withDefinitions = MarkdownPreprocessor::convertDefinitionLists(cleaned);
    auto renderResult = MarkdownHtmlRenderer::render(withDefinitions);

    QVERIFY2(renderResult.isValid(), "comprehensive-test.md must render successfully");
    QVERIFY2(!renderResult.html.isEmpty(), "rendered HTML must not be empty");

    // Verify headings rendered
    QVERIFY2(renderResult.html.contains("<h1>") || renderResult.html.contains("<h1 "),
             "document must produce h1 heading");
    QVERIFY2(renderResult.html.contains("<h2>") || renderResult.html.contains("<h2 "),
             "document must produce h2 heading");

    // Verify tables rendered
    QVERIFY2(renderResult.html.contains("<table"),
             "document must produce table elements");

    // Verify code blocks rendered with wrapping
    QVERIFY2(renderResult.html.contains("code-block-table"),
             "fenced code blocks must be wrapped in code-block-table");

    // Verify blockquotes rendered with wrapping
    QVERIFY2(renderResult.html.contains("blockquote-table"),
             "blockquotes must be wrapped in blockquote-table");

    // Verify raw HTML normalization
    QVERIFY2(!renderResult.html.contains("<kbd>"),
             "<kbd> must be normalized to <code>");
    QVERIFY2(renderResult.html.contains("background-color: #FFF3B0"),
             "<mark> must produce amber highlight inline style");

    // Verify RTL support — Arabic content must get dir="rtl"
    QVERIFY2(renderResult.html.contains("dir=\"rtl\""),
             "Arabic text blocks must get dir=\"rtl\" attribute");

    // Verify task list checkboxes replaced
    QVERIFY2(renderResult.html.contains(QStringLiteral("☐")) ||
             renderResult.html.contains(QStringLiteral("☑")),
             "task list checkboxes must be replaced with Unicode markers");

    // Verify code blocks do NOT get RTL direction
    int prePos = renderResult.html.indexOf("<pre");
    if (prePos >= 0) {
        int preClose = renderResult.html.indexOf("</pre>", prePos);
        if (preClose > prePos) {
            QString codeSection = renderResult.html.mid(prePos, preClose - prePos + 6);
            QVERIFY2(!codeSection.contains("dir=\"rtl\""),
                     "code blocks must NOT have dir=\"rtl\" even if they contain Arabic");
        }
    }

    // Verify content from each major section is preserved
    QVERIFY2(renderResult.html.contains("اختبار شامل"),
             "Arabic title must be preserved in rendered output");
    QVERIFY2(renderResult.html.contains("fibonacci"),
             "JavaScript code content must be preserved");
    QVERIFY2(renderResult.html.contains("MarkdownParser"),
             "Python code content must be preserved");
}

void ComprehensiveRenderTest::testArabicTextPreserved()
{
    QString path = QString(TEST_DATA_DIR) + "/comprehensive-test.md";
    auto loadResult = DocumentLoader::load(path);
    QVERIFY(loadResult.isValid());

    QString cleaned = MarkdownPreprocessor::stripYamlFrontMatter(loadResult.document.rawText);
    QString withDefinitions = MarkdownPreprocessor::convertDefinitionLists(cleaned);
    auto renderResult = MarkdownHtmlRenderer::render(withDefinitions);
    QVERIFY(renderResult.isValid());

    QVERIFY2(renderResult.html.contains("مرحباً"),
             "Arabic greeting must be preserved in rendered HTML");
    QVERIFY2(renderResult.html.contains("باللغة العربية"),
             "Arabic paragraph text must be preserved");
}

void ComprehensiveRenderTest::testCodeBlocksPreserved()
{
    QString path = QString(TEST_DATA_DIR) + "/comprehensive-test.md";
    auto loadResult = DocumentLoader::load(path);
    QVERIFY(loadResult.isValid());

    QString cleaned = MarkdownPreprocessor::stripYamlFrontMatter(loadResult.document.rawText);
    QString withDefinitions = MarkdownPreprocessor::convertDefinitionLists(cleaned);
    auto renderResult = MarkdownHtmlRenderer::render(withDefinitions);
    QVERIFY(renderResult.isValid());

    // C++ code
    QVERIFY2(mdbr::test::visibleText(renderResult.html).contains("std::cout"),
             "C++ code content must be preserved");
    // JavaScript code
    QVERIFY2(mdbr::test::visibleText(renderResult.html).contains("fibonacci"),
             "JS code content must be preserved");
    // Python code
    QVERIFY2(mdbr::test::visibleText(renderResult.html).contains("class MarkdownParser"),
             "Python code content must be preserved");
    // PowerShell code
    QVERIFY2(mdbr::test::visibleText(renderResult.html).contains("Get-MarkdownStats"),
             "PowerShell code content must be preserved");
    // SQL code
    QVERIFY2(mdbr::test::visibleText(renderResult.html).contains("doc_stats"),
             "SQL code content must be preserved");
    // JSON code
    QVERIFY2(mdbr::test::visibleText(renderResult.html).contains("maxFileSize"),
             "JSON code content must be preserved");
}

void ComprehensiveRenderTest::testTaskLists()
{
    QString path = QString(TEST_DATA_DIR) + "/comprehensive-test.md";
    auto loadResult = DocumentLoader::load(path);
    QVERIFY(loadResult.isValid());

    QString cleaned = MarkdownPreprocessor::stripYamlFrontMatter(loadResult.document.rawText);
    QString withDefinitions = MarkdownPreprocessor::convertDefinitionLists(cleaned);
    auto renderResult = MarkdownHtmlRenderer::render(withDefinitions);
    QVERIFY(renderResult.isValid());

    // Verify we have both checked and unchecked markers
    QVERIFY2(renderResult.html.contains(QStringLiteral("☑")),
             "checked task list items must produce ☑ marker");
    QVERIFY2(renderResult.html.contains(QStringLiteral("☐")),
             "unchecked task list items must produce ☐ marker");
    QVERIFY2(!renderResult.html.contains("<input type=\"checkbox\""),
             "no raw checkbox input elements should remain");
}

QTEST_MAIN(ComprehensiveRenderTest)
#include "ComprehensiveRenderTest.moc"
