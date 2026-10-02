// Markdown feature coverage — what the reader actually supports.
//
// Each row is one syntax feature, rendered through the full pipeline into the
// QTextDocument the reader displays, then checked for what a user would see.
// The test reports EVERY unsupported feature at once (not just the first), so
// this file doubles as the authoritative support matrix: if a row is here and
// passing, the feature works end to end.
//
// To add a feature: add a row to kFeatures, watch it fail, implement, watch it pass.

#include <QTest>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextList>
#include <QTextTable>

#include "RenderPipelineTestHelpers.h"

using mdbr::test::renderToDocument;

namespace {

enum class Expect
{
    HeadingLevel,     // value: level of the block containing `target`
    Bold,             // `target` is bold
    Italic,           // `target` is italic
    StrikeOut,        // `target` is struck out
    Monospace,        // `target` uses a fixed-pitch font
    Superscript,      // `target` is superscript
    Highlighted,      // `target` has a background colour
    Coloured,         // `target` has a foreground colour different from plain text
    LinkTo,           // `target` is a link to value
    AnchorNamed,      // the document contains an anchor named `target`
    InTable,          // `target` is inside a table
    InList,           // `target` is inside a list
    ShowsText,        // the visible text contains `target`
    HidesText,        // the visible text does NOT contain `target`
    HasImage,         // the document contains an image
};

struct Check
{
    Expect expect;
    QString target;
    QString value = {};
};

struct Feature
{
    const char *name;
    QString markdown;
    QList<Check> checks;
};

QTextCharFormat formatOf(const QTextDocument &doc, const QString &text)
{
    const QTextCursor c = doc.find(text);
    return c.isNull() ? QTextCharFormat() : c.charFormat();
}

bool documentHasAnchorNamed(const QTextDocument &doc, const QString &name)
{
    for (QTextBlock b = doc.begin(); b.isValid(); b = b.next())
        for (auto it = b.begin(); !it.atEnd(); ++it)
            if (it.fragment().charFormat().anchorNames().contains(name))
                return true;
    return false;
}

bool documentHasImage(const QTextDocument &doc)
{
    for (QTextBlock b = doc.begin(); b.isValid(); b = b.next())
        for (auto it = b.begin(); !it.atEnd(); ++it)
            if (it.fragment().charFormat().isImageFormat())
                return true;
    return false;
}

// Returns an empty string when the check passes, otherwise why it failed.
QString evaluate(const QTextDocument &doc, const Check &check)
{
    const QString plain = doc.toPlainText();
    const QTextCursor found = doc.find(check.target);
    const bool needsTarget = check.expect != Expect::AnchorNamed && check.expect != Expect::HidesText
                          && check.expect != Expect::HasImage && check.expect != Expect::ShowsText;
    if (needsTarget && found.isNull())
        return QString("\"%1\" not visible").arg(check.target);

    const QTextCharFormat f = formatOf(doc, check.target);
    switch (check.expect) {
    case Expect::HeadingLevel:
        return found.blockFormat().headingLevel() == check.value.toInt() ? QString()
            : QString("heading level %1, expected %2").arg(found.blockFormat().headingLevel()).arg(check.value);
    case Expect::Bold:        return f.fontWeight() >= QFont::Bold ? QString() : "not bold";
    case Expect::Italic:      return f.fontItalic() ? QString() : "not italic";
    case Expect::StrikeOut:   return f.fontStrikeOut() ? QString() : "not struck out";
    case Expect::Monospace: {
        // Qt maps <code>/<pre> to the generic "monospace" family (or the reader
        // stylesheet's Consolas/Courier stack); resolution happens at paint time.
        const QString families = f.fontFamilies().toStringList().join(',').toLower();
        const bool mono = f.fontFixedPitch() || families.contains("monospace")
                       || families.contains("consolas") || families.contains("courier");
        return mono ? QString() : "not monospace";
    }
    case Expect::Superscript: return f.verticalAlignment() == QTextCharFormat::AlignSuperScript ? QString() : "not superscript";
    case Expect::Highlighted: return f.background().style() != Qt::NoBrush ? QString() : "no background";
    case Expect::Coloured:    return f.foreground().style() != Qt::NoBrush ? QString() : "no colour";
    case Expect::LinkTo:      return f.isAnchor() && f.anchorHref() == check.value ? QString()
                                      : QString("link is \"%1\", expected \"%2\"").arg(f.anchorHref(), check.value);
    case Expect::AnchorNamed: return documentHasAnchorNamed(doc, check.target) ? QString() : "anchor missing";
    case Expect::InTable:     return found.currentTable() ? QString() : "not in a table";
    case Expect::InList:      return found.currentList() ? QString() : "not in a list";
    case Expect::ShowsText:   return plain.contains(check.target) ? QString() : QString("\"%1\" not shown").arg(check.target);
    case Expect::HidesText:   return !plain.contains(check.target) ? QString() : QString("raw \"%1\" still shown").arg(check.target);
    case Expect::HasImage:    return documentHasImage(doc) ? QString() : "no image";
    }
    return "unknown check";
}

const QList<Feature> &features()
{
    static const QList<Feature> list = {
        // --- CommonMark core -------------------------------------------------
        {"ATX headings", "# Title one\n\n###### Small six", {{Expect::HeadingLevel, "Title one", "1"}, {Expect::HeadingLevel, "Small six", "6"}, {Expect::HidesText, "#"}}},
        {"Setext headings", "Title\n=====\n\nSub\n---", {{Expect::HeadingLevel, "Title", "1"}, {Expect::HeadingLevel, "Sub", "2"}}},
        {"Bold / italic", "**strong** and *soft* and ***both***", {{Expect::Bold, "strong"}, {Expect::Italic, "soft"}, {Expect::Bold, "both"}, {Expect::Italic, "both"}}},
        {"Underscore emphasis", "__strong__ _soft_", {{Expect::Bold, "strong"}, {Expect::Italic, "soft"}}},
        {"Inline code", "use `printf` here", {{Expect::Monospace, "printf"}, {Expect::HidesText, "`"}}},
        {"Fenced code", "```\nfenced line\n```", {{Expect::Monospace, "fenced line"}}},
        {"Indented code", "para\n\n    indented line", {{Expect::Monospace, "indented line"}}},
        {"Blockquote", "> quoted words", {{Expect::ShowsText, "quoted words"}, {Expect::HidesText, ">"}}},
        {"Bullet list", "- alpha\n- beta", {{Expect::InList, "alpha"}}},
        {"Ordered list", "1. first\n2. second", {{Expect::InList, "first"}}},
        {"Nested list", "- outer\n  - inner", {{Expect::InList, "inner"}}},
        {"Inline link", "[site](https://example.com)", {{Expect::LinkTo, "site", "https://example.com"}}},
        {"Reference link", "[ref][1]\n\n[1]: https://example.org", {{Expect::LinkTo, "ref", "https://example.org"}, {Expect::HidesText, "[1]"}}},
        {"Angle autolink", "<https://example.net>", {{Expect::LinkTo, "example.net", "https://example.net"}}},
        {"Image", "![alt](../../assets/images/sample-cover.png)", {{Expect::HasImage, ""}}},
        // QTextDocument::toPlainText() reports a hard line break as '\n'.
        {"Hard break (spaces)", "line one  \nline two", {{Expect::ShowsText, "line one\nline two"}}},
        {"Hard break (backslash)", "line one\\\nline two", {{Expect::ShowsText, "line one\nline two"}}},
        {"HTML entities", "&copy; &amp; &#169;", {{Expect::ShowsText, QStringLiteral(u"\u00A9 & \u00A9")}}},
        {"Backslash escapes", "\\*not italic\\*", {{Expect::ShowsText, "*not italic*"}}},
        {"Thematic break", "a\n\n---\n\nb", {{Expect::ShowsText, "b"}}},
        {"Raw inline HTML", "H<sub>a</sub>O and x<sup>top</sup>", {{Expect::Superscript, "top"}}},

        // --- GitHub Flavored Markdown ----------------------------------------
        {"Table", "| h1 | h2 |\n|---|---|\n| c1 | c2 |", {{Expect::InTable, "c1"}}},
        {"Strikethrough", "~~gone~~", {{Expect::StrikeOut, "gone"}}},
        {"Task list", "- [ ] todo\n- [x] done", {{Expect::ShowsText, QStringLiteral(u"\u2610 todo")}, {Expect::ShowsText, QStringLiteral(u"\u2611 done")}}},
        {"Bare URL autolink", "see https://example.com/x now", {{Expect::LinkTo, "example.com/x", "https://example.com/x"}}},
        {"Footnotes", "Claim[^1].\n\n[^1]: The source.", {{Expect::HidesText, "[^1]"}, {Expect::ShowsText, "The source."}, {Expect::Superscript, "1"}}},
        {"GitHub alerts", "> [!NOTE]\n> Read this.", {{Expect::HidesText, "[!NOTE]"}, {Expect::Bold, "Note"}, {Expect::ShowsText, "Read this."}}},
        {"Heading anchors", "# My Section\n\n[jump](#my-section)", {{Expect::AnchorNamed, "my-section"}, {Expect::LinkTo, "jump", "#my-section"}}},
        {"Syntax highlighting", "```cpp\nint value = 42; // note\n```", {{Expect::Coloured, "int"}, {Expect::Coloured, "// note"}}},

        // --- Popular extensions (Pandoc, Obsidian, Typora, MkDocs) -----------
        {"Highlight ==mark==", "plain ==marked== text", {{Expect::HidesText, "=="}, {Expect::Highlighted, "marked"}}},
        {"Superscript ^x^", "E = mc^2^", {{Expect::HidesText, "^"}, {Expect::Superscript, "2"}}},
        {"Inline math $..$", "area $\\pi r^2$ here", {{Expect::HidesText, "$"}, {Expect::ShowsText, QStringLiteral(u"\u03C0")}}},
        {"Display math $$..$$", "$$\n\\sum_{i=1}^{n} i\n$$", {{Expect::HidesText, "$$"}, {Expect::ShowsText, QStringLiteral(u"\u2211")}}},
        {"Emoji shortcodes", "Great :smile: :rocket:", {{Expect::HidesText, ":smile:"}, {Expect::ShowsText, QStringLiteral(u"\U0001F604")}}},
        {"Wiki links [[ ]]", "See [[Other Page]]", {{Expect::HidesText, "[["}, {Expect::LinkTo, "Other Page", "Other Page.md"}}},
        {"Definition list", "Term\n: Meaning", {{Expect::InTable, "Meaning"}}},
        {"YAML front matter", "---\ntitle: X\n---\nBody", {{Expect::HidesText, "title:"}, {Expect::ShowsText, "Body"}}},
    };
    return list;
}

} // namespace

class MarkdownFeatureCoverageTest : public QObject
{
    Q_OBJECT

private slots:
    void allFeaturesRender();
};

void MarkdownFeatureCoverageTest::allFeaturesRender()
{
    QStringList unsupported;
    int supported = 0;
    for (const Feature &feature : features()) {
        const auto rendered = renderToDocument(feature.markdown);
        QStringList problems;
        for (const Check &check : feature.checks) {
            const QString problem = evaluate(*rendered.document, check);
            if (!problem.isEmpty())
                problems << problem;
        }
        if (problems.isEmpty())
            ++supported;
        else
            unsupported << QString("  %1: %2").arg(feature.name, problems.join("; "));
    }

    qInfo("Supported %d of %lld features", supported, qlonglong(features().size()));
    QVERIFY2(unsupported.isEmpty(), qPrintable("Unsupported features:\n" + unsupported.join('\n')));
}

QTEST_MAIN(MarkdownFeatureCoverageTest)
#include "MarkdownFeatureCoverageTest.moc"
