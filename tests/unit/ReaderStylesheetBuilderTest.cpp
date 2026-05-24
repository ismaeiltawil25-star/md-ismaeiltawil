#include <QTest>
#include <QString>
#include "app/appearance/ReaderAppearanceDefaults.h"
#include "app/appearance/ReaderStylesheetBuilder.h"

class ReaderStylesheetBuilderTest : public QObject
{
    Q_OBJECT

private slots:
    void testDefaultDocumentStylesheetHasSelectors();
    void testDefaultDocumentStylesheetHasColors();
    void testWidgetStylesheet();
    void testCustomColorsAppear();

    void testHeadingTextMappedToAllHeadings();
    void testMainTextMappedToBody();
    void testMutedTextMappedToEmI();
    void testParagraphHasBottomMargin();
    void testParagraphHasLineHeight();
    void testStrongUsesFontWeight();
    void testCodeHasMonospaceFontFamily();
    void testListProperties();
    void testLinkTextMappedToA();
    void testInlineCodeTextAndBgMappedToCode();
    void testCodeBlockTextAndBgMappedToPreOnly();
    void testBlockquoteTextAndBorderMappedToBlockquote();
    void testBlockquotePaddingAndNestedSelector();
    void testHorizontalRuleMappedToHrOnly();
    void testPageBackgroundMappedToBody();
    void testNightPresetAllValuesInCss();

    void testTableSelectorsExist();
    void testTableColorRoles();

    void testCodeBlockPreUsesCodeBlockColors();
    void testCodeBlockPreHasPaddingAndLineHeight();
    void testCodeBlockPreCodeUsesSameColors();
    void testCodeBlockPreCodeHasNoPadding();
    void testCodeBlockCellUsesBackgroundAndBorder();
    void testInlineCodeStillUsesInlineColors();
    void testHtmlInlineSelectorsExist();
    void testSummaryUsesHeadingText();
    void testHtmlBlockUsesMainTextAndHorizontalRule();
    void testBodyFontStackIncludesEmojiFallbacks();
    void testDefinitionTableSelectorsExist();
    void testDefinitionTermUsesHeadingText();
    void testDefinitionDescriptionUsesMainText();
    void testTaskListItemCssExists();
};

void ReaderStylesheetBuilderTest::testDefaultDocumentStylesheetHasSelectors()
{
    auto settings = ReaderAppearanceDefaults::calmPalette();
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(settings);

    QVERIFY(css.contains("body {"));
    QVERIFY(css.contains("h1 {"));
    QVERIFY(css.contains("h2 {"));
    QVERIFY(css.contains("h3 {"));
    QVERIFY(css.contains("h4 {"));
    QVERIFY(css.contains("h5 {"));
    QVERIFY(css.contains("h6 {"));
    QVERIFY(css.contains("strong {"));
    QVERIFY(css.contains("em,") || css.contains("em {"));
    QVERIFY(css.contains("p {"));
    QVERIFY(css.contains("ul,") || css.contains("ul {"));
    QVERIFY(css.contains("ol,") || css.contains("ol {"));
    QVERIFY(css.contains("li {"));
    QVERIFY(css.contains("a {"));
    QVERIFY(css.contains("code {"));
    QVERIFY(css.contains("table.code-block-table {"));
    QVERIFY(css.contains("td.code-block-cell {"));
    QVERIFY(css.contains("pre {"));
    QVERIFY(css.contains("pre code {"));
    QVERIFY(css.contains("table.blockquote-table {"));
    QVERIFY(css.contains("td.blockquote-bar {"));
    QVERIFY(css.contains("td.blockquote-cell {"));
    QVERIFY(css.contains("hr {"));
}

void ReaderStylesheetBuilderTest::testDefaultDocumentStylesheetHasColors()
{
    auto settings = ReaderAppearanceDefaults::calmPalette();
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(settings);

    QVERIFY(css.contains(settings.pageBackground));
    QVERIFY(css.contains(settings.mainText));
    QVERIFY(css.contains(settings.headingText));
    QVERIFY(css.contains(settings.mutedText));
    QVERIFY(css.contains(settings.linkText));
    QVERIFY(css.contains(settings.inlineCodeText));
    QVERIFY(css.contains(settings.inlineCodeBackground));
    QVERIFY(css.contains(settings.codeBlockText));
    QVERIFY(css.contains(settings.codeBlockBackground));
    QVERIFY(css.contains(settings.blockquoteText));
    QVERIFY(css.contains(settings.blockquoteBorder));
    QVERIFY(css.contains(settings.horizontalRule));
}

void ReaderStylesheetBuilderTest::testWidgetStylesheet()
{
    auto settings = ReaderAppearanceDefaults::calmPalette();
    QString css = ReaderStylesheetBuilder::buildWidgetStylesheet(settings);

    QVERIFY(css.contains("QTextBrowser"));
    QVERIFY(css.contains("background-color"));
    QVERIFY(css.contains("color"));
    QVERIFY(css.contains(settings.pageBackground));
    QVERIFY(css.contains(settings.mainText));
}

void ReaderStylesheetBuilderTest::testCustomColorsAppear()
{
    ReaderAppearanceSettings custom;
    custom.pageBackground    = QStringLiteral("#111111");
    custom.mainText          = QStringLiteral("#eeeeee");
    custom.codeBlockText     = QStringLiteral("#00ff00");
    custom.blockquoteBorder  = QStringLiteral("#ff0000");

    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(custom);

    QVERIFY(css.contains("#111111"));
    QVERIFY(css.contains("#eeeeee"));
    QVERIFY(css.contains("#00ff00"));
    QVERIFY(css.contains("#ff0000"));
}

// ------------------------------------------------------------------
// Targeted role-to-selector mapping tests with unique test colors
// ------------------------------------------------------------------

void ReaderStylesheetBuilderTest::testHeadingTextMappedToAllHeadings()
{
    ReaderAppearanceSettings s;
    s.headingText = QStringLiteral("#AABB00");
    s.mainText    = QStringLiteral("#000001");
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(s);

    for (const auto& level : {"h1", "h2", "h3", "h4", "h5", "h6"}) {
        QVERIFY2(css.contains(QString("%1 { color: #AABB00;").arg(level)),
                 QString("headingText must appear on %1 selector").arg(level).toLatin1());
    }

    QVERIFY2(!css.contains("body { color: #AABB00;"),
             "headingText must NOT leak into body color");
}

void ReaderStylesheetBuilderTest::testMainTextMappedToBody()
{
    ReaderAppearanceSettings s;
    s.mainText    = QStringLiteral("#BB00BB");
    s.headingText = QStringLiteral("#000002");
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(s);

    QVERIFY2(css.contains("body {") && css.contains("color: #BB00BB;"),
             "mainText must appear as body color");
}

void ReaderStylesheetBuilderTest::testMutedTextMappedToEmI()
{
    ReaderAppearanceSettings s;
    s.mutedText    = QStringLiteral("#CC7700");
    s.mainText     = QStringLiteral("#000003");
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(s);

    QVERIFY2(css.contains("em, i { color: #CC7700;"),
             "mutedText must appear as color value on em, i selector");
    QVERIFY2(!css.contains("body { color: #CC7700;"),
             "mutedText must NOT leak into body color");
}

void ReaderStylesheetBuilderTest::testParagraphHasBottomMargin()
{
    ReaderAppearanceSettings s;
    s.mainText = QStringLiteral("#000001");
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(s);

    QVERIFY2(css.contains("p {") && css.contains("margin: 0 0 12px 0;"),
             "p rule must contain bottom margin");
}

void ReaderStylesheetBuilderTest::testParagraphHasLineHeight()
{
    ReaderAppearanceSettings s;
    s.mainText = QStringLiteral("#000001");
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(s);

    QVERIFY2(css.contains("p {") && css.contains("line-height: 1.55;"),
             "p rule must contain line-height");
}

void ReaderStylesheetBuilderTest::testStrongUsesFontWeight()
{
    ReaderAppearanceSettings s;
    s.mainText = QStringLiteral("#000001");
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(s);

    QVERIFY2(css.contains("strong {") && css.contains("font-weight: 700;"),
             "strong rule must use font-weight: 700");
}

void ReaderStylesheetBuilderTest::testCodeHasMonospaceFontFamily()
{
    ReaderAppearanceSettings s;
    s.inlineCodeText       = QStringLiteral("#DD00DD");
    s.inlineCodeBackground = QStringLiteral("#EEDDEE");
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(s);

    QVERIFY2(css.contains("font-family: Consolas"),
             "code rule must contain Consolas monospace font-family");
}

void ReaderStylesheetBuilderTest::testListProperties()
{
    auto settings = ReaderAppearanceDefaults::calmPalette();
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(settings);

    QVERIFY2(css.contains("ul,") || css.contains("ul {"),
             "ul selector must exist in stylesheet");
    QVERIFY2(css.contains("ol,") || css.contains("ol {"),
             "ol selector must exist in stylesheet");
    QVERIFY2(css.contains("li {"),
             "li selector must exist in stylesheet");

    QVERIFY2(css.contains("margin-top: 6px;"),
             "ul/ol must contain margin-top: 6px");
    QVERIFY2(css.contains("margin-bottom: 14px;"),
             "ul/ol must contain margin-bottom: 14px");
    QVERIFY2(css.contains("margin-left: 22px;") ||
             css.contains("padding-left: 14px;"),
             "ul/ol must contain margin-left or padding-left for indentation");

    QVERIFY2(css.contains("-qt-list-indent: 1;"),
             "ul/ol must contain -qt-list-indent: 1 for compact nesting");

    QVERIFY2(css.contains("margin-top: 3px;"),
             "li must contain margin-top: 3px");
    QVERIFY2(css.contains("margin-bottom: 5px;"),
             "li must contain margin-bottom: 5px");
    QVERIFY2(css.contains("line-height: 1.45;"),
             "li must contain line-height: 1.45");
}

void ReaderStylesheetBuilderTest::testLinkTextMappedToA()
{
    ReaderAppearanceSettings s;
    s.linkText = QStringLiteral("#00CC00");
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(s);

    QVERIFY2(css.contains("a { color: #00CC00;"),
             "linkText must appear as a color");
}

void ReaderStylesheetBuilderTest::testInlineCodeTextAndBgMappedToCode()
{
    ReaderAppearanceSettings s;
    s.inlineCodeText       = QStringLiteral("#DD00DD");
    s.inlineCodeBackground = QStringLiteral("#EEDDEE");
    s.codeBlockText        = QStringLiteral("#000004");
    s.codeBlockBackground  = QStringLiteral("#000005");
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(s);

    QVERIFY2(css.contains("code {") &&
             css.contains("color: #DD00DD;"),
             "inlineCodeText must appear as code color");
    QVERIFY2(css.contains("background-color: #EEDDEE;"),
             "inlineCodeBackground must appear as code background");
}

void ReaderStylesheetBuilderTest::testCodeBlockTextAndBgMappedToPreOnly()
{
    ReaderAppearanceSettings s;
    s.codeBlockText       = QStringLiteral("#FF00FF");
    s.codeBlockBackground = QStringLiteral("#334433");
    s.inlineCodeText      = QStringLiteral("#000006");
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(s);

    QVERIFY2(css.contains("pre {") &&
             css.contains("color: #FF00FF;"),
             "codeBlockText must appear as pre color");
    QVERIFY2(css.contains("td.code-block-cell") &&
             css.contains("background-color: #334433;"),
             "codeBlockBackground must appear on td.code-block-cell");

    // codeBlockBackground must NOT leak into blockquote bar or cell
    int barStart = css.indexOf("td.blockquote-bar {");
    int barEnd   = barStart >= 0 ? css.indexOf("}", barStart) : -1;
    if (barStart >= 0 && barEnd > barStart) {
        QString barBlock = css.mid(barStart, barEnd - barStart + 1);
        QVERIFY2(!barBlock.contains("#334433"),
                 "codeBlockBackground must NOT leak into td.blockquote-bar");
    }
    int cellStart = css.indexOf("td.blockquote-cell {");
    int cellEnd   = cellStart >= 0 ? css.indexOf("}", cellStart) : -1;
    if (cellStart >= 0 && cellEnd > cellStart) {
        QString cellBlock = css.mid(cellStart, cellEnd - cellStart + 1);
        QVERIFY2(!cellBlock.contains("#334433"),
                 "codeBlockBackground must NOT leak into td.blockquote-cell");
    }
}

void ReaderStylesheetBuilderTest::testBlockquoteTextAndBorderMappedToBlockquote()
{
    ReaderAppearanceSettings s;
    s.blockquoteText   = QStringLiteral("#885500");
    s.blockquoteBorder = QStringLiteral("#00FFFF");
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(s);

    QVERIFY2(css.contains("table.blockquote-table {") &&
             css.contains("width: 100%;"),
             "table.blockquote-table must have width: 100%");
    QVERIFY2(css.contains("td.blockquote-cell {") &&
             css.contains("color: #885500;"),
             "blockquoteText must appear on td.blockquote-cell");
    QVERIFY2(css.contains("td.blockquote-bar {") &&
             css.contains("background-color: #00FFFF;"),
             "blockquoteBorder must appear on td.blockquote-bar as background-color");
}

void ReaderStylesheetBuilderTest::testBlockquotePaddingAndNestedSelector()
{
    auto settings = ReaderAppearanceDefaults::calmPalette();
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(settings);

    QVERIFY2(css.contains("padding: 6px 0 6px 14px;") ||
             css.contains("td.blockquote-cell") &&
             css.contains("padding:"),
             "td.blockquote-cell must have padding");
}

void ReaderStylesheetBuilderTest::testHorizontalRuleMappedToHrOnly()
{
    ReaderAppearanceSettings s;
    s.horizontalRule = QStringLiteral("#999999");
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(s);

    QVERIFY2(css.contains("hr {") &&
             css.contains("border-top: 1px solid #999999;"),
             "horizontalRule must appear as hr border-top");
    QVERIFY2(css.contains("margin-top: 22px;"),
             "hr must have margin-top: 22px");
    QVERIFY2(css.contains("margin-bottom: 24px;"),
             "hr must have margin-bottom: 24px");

    QVERIFY2(css.contains("td.code-block-cell") &&
             css.contains("border: 1px solid #999999;"),
             "horizontalRule must appear as td.code-block-cell border");
}

void ReaderStylesheetBuilderTest::testPageBackgroundMappedToBody()
{
    ReaderAppearanceSettings s;
    s.pageBackground = QStringLiteral("#F0F0F0");
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(s);

    QVERIFY2(css.contains("body {") &&
             css.contains("background-color: #F0F0F0;"),
             "pageBackground must appear as body background-color");
}

void ReaderStylesheetBuilderTest::testNightPresetAllValuesInCss()
{
    auto s = ReaderAppearanceDefaults::nightPalette();
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(s);

    QVERIFY2(css.contains(s.pageBackground),
             "pageBackground hex must appear in Night CSS");
    QVERIFY2(css.contains(s.mainText),
             "mainText hex must appear in Night CSS");
    QVERIFY2(css.contains(s.headingText),
             "headingText hex must appear in Night CSS");
    QVERIFY2(css.contains(s.mutedText),
             "mutedText hex must appear in Night CSS");
    QVERIFY2(css.contains(s.linkText),
             "linkText hex must appear in Night CSS");
    QVERIFY2(css.contains(s.inlineCodeText),
             "inlineCodeText hex must appear in Night CSS");
    QVERIFY2(css.contains(s.inlineCodeBackground),
             "inlineCodeBackground hex must appear in Night CSS");
    QVERIFY2(css.contains(s.codeBlockText),
             "codeBlockText hex must appear in Night CSS");
    QVERIFY2(css.contains(s.codeBlockBackground),
             "codeBlockBackground hex must appear in Night CSS");
    QVERIFY2(css.contains(s.blockquoteText),
             "blockquoteText hex must appear in Night CSS");
    QVERIFY2(css.contains(s.blockquoteBorder),
             "blockquoteBorder hex must appear in Night CSS");
    QVERIFY2(css.contains(s.horizontalRule),
             "horizontalRule hex must appear in Night CSS");

    // Verify none of the calm (default) hex values leak into Night CSS
    auto calm = ReaderAppearanceDefaults::calmPalette();
    if (calm.pageBackground != s.pageBackground)
        QVERIFY2(!css.contains(calm.pageBackground),
                 "calm pageBackground must NOT leak into Night CSS");
    if (calm.headingText != s.headingText)
        QVERIFY2(!css.contains(calm.headingText),
                 "calm headingText must NOT leak into Night CSS");
    if (calm.inlineCodeBackground != s.inlineCodeBackground)
        QVERIFY2(!css.contains(calm.inlineCodeBackground),
                 "calm inlineCodeBackground must NOT leak into Night CSS");
    if (calm.mutedText != s.mutedText)
        QVERIFY2(!css.contains(calm.mutedText),
                 "calm mutedText must NOT leak into Night CSS");
}

void ReaderStylesheetBuilderTest::testTableSelectorsExist()
{
    auto settings = ReaderAppearanceDefaults::calmPalette();
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(settings);

    QVERIFY2(css.contains("table {"),
             "table selector must exist in stylesheet");
    QVERIFY2(css.contains("th {"),
             "th selector must exist in stylesheet");
    QVERIFY2(css.contains("td {"),
             "td selector must exist in stylesheet");
}

void ReaderStylesheetBuilderTest::testTableColorRoles()
{
    ReaderAppearanceSettings s;
    s.mainText              = QStringLiteral("#EEEEEE");
    s.headingText           = QStringLiteral("#AAAA00");
    s.inlineCodeBackground   = QStringLiteral("#335533");
    s.horizontalRule        = QStringLiteral("#777777");
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(s);

    QVERIFY2(css.contains("th {") && css.contains("color: #AAAA00;"),
             "th must use headingText color");
    QVERIFY2(css.contains("background-color: #335533;"),
             "th must use inlineCodeBackground");
    QVERIFY2(css.contains("border: 1px solid #777777;"),
             "th border must use horizontalRule");
    QVERIFY2(css.contains("td {") && css.contains("color: #EEEEEE;"),
             "td must use mainText color");
    QVERIFY2(css.contains("td {") && css.contains("border: 1px solid #777777;"),
             "td border must use horizontalRule");
}

void ReaderStylesheetBuilderTest::testCodeBlockPreUsesCodeBlockColors()
{
    ReaderAppearanceSettings s;
    s.codeBlockText       = QStringLiteral("#00FF00");
    s.codeBlockBackground = QStringLiteral("#112211");
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(s);

    QVERIFY2(css.contains("pre {") && css.contains("color: #00FF00;"),
             "pre must use codeBlockText color");
    QVERIFY2(css.contains("td.code-block-cell {") &&
             css.contains("background-color: #112211;"),
             "td.code-block-cell must use codeBlockBackground");
    QVERIFY2(css.contains("font-family: Consolas"),
             "pre must have monospace font-family");
}

void ReaderStylesheetBuilderTest::testCodeBlockPreHasPaddingAndLineHeight()
{
    auto settings = ReaderAppearanceDefaults::calmPalette();
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(settings);

    QVERIFY2(css.contains("td.code-block-cell") && css.contains("padding: 10px 12px;"),
             "td.code-block-cell must own the code block padding");
    QVERIFY2(css.contains("line-height: 1.45;"),
             "pre must have line-height");
}

void ReaderStylesheetBuilderTest::testCodeBlockPreCodeUsesSameColors()
{
    ReaderAppearanceSettings s;
    s.codeBlockText       = QStringLiteral("#AAFFAA");
    s.codeBlockBackground = QStringLiteral("#334433");
    s.inlineCodeText      = QStringLiteral("#000007");
    s.inlineCodeBackground = QStringLiteral("#EEEEEE");
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(s);

    QVERIFY2(css.contains("pre code {") && css.contains("color: #AAFFAA;"),
             "pre code must use codeBlockText, not inlineCodeText");
    QVERIFY2(css.contains("background-color: transparent;"),
             "pre code must use transparent to avoid per-line striping");
}

void ReaderStylesheetBuilderTest::testCodeBlockPreCodeHasNoPadding()
{
    auto settings = ReaderAppearanceDefaults::calmPalette();
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(settings);

    QVERIFY2(css.contains("pre code {") &&
             (css.contains("padding: 0;") || css.contains("padding: 0px;")),
             "pre code must have padding: 0");
}

void ReaderStylesheetBuilderTest::testCodeBlockCellUsesBackgroundAndBorder()
{
    ReaderAppearanceSettings s;
    s.codeBlockBackground = QStringLiteral("#112233");
    s.horizontalRule      = QStringLiteral("#445566");
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(s);

    QVERIFY2(css.contains("td.code-block-cell") &&
             css.contains("background-color: #112233;"),
             "td.code-block-cell must use codeBlockBackground");
    QVERIFY2(css.contains("border: 1px solid #445566;"),
             "td.code-block-cell border must use horizontalRule");
}

void ReaderStylesheetBuilderTest::testInlineCodeStillUsesInlineColors()
{
    ReaderAppearanceSettings s;
    s.inlineCodeText       = QStringLiteral("#DD00DD");
    s.inlineCodeBackground = QStringLiteral("#EEDDEE");
    s.codeBlockText        = QStringLiteral("#000004");
    s.codeBlockBackground  = QStringLiteral("#000005");
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(s);

    // Inline code (code not inside pre) must use #DD00DD / #EEDDEE.
    // These colors are set on the standalone "code {" rule only.
    int codePos = css.indexOf("code {");
    int preCodePos = css.indexOf("pre code {");

    QVERIFY2(codePos >= 0, "code selector must exist");
    QVERIFY2(preCodePos > codePos,
             "pre code selector must appear after plain code selector");

    QVERIFY2(css.count("color: #DD00DD;") >= 1,
             "inline code must use inlineCodeText");
    QVERIFY2(css.contains("background-color: #EEDDEE;"),
             "inline code must use inlineCodeBackground");
}

void ReaderStylesheetBuilderTest::testHtmlInlineSelectorsExist()
{
    auto settings = ReaderAppearanceDefaults::calmPalette();
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(settings);

    QVERIFY2(css.contains("p.html-summary {"),
             "p.html-summary selector must exist in stylesheet");
    QVERIFY2(css.contains("table.html-block-table {"),
             "table.html-block-table selector must exist in stylesheet");
    QVERIFY2(css.contains("td.html-block-cell {"),
             "td.html-block-cell selector must exist in stylesheet");
    QVERIFY2(css.contains("sub {"),
             "sub selector must exist in stylesheet");
    QVERIFY2(css.contains("sup {"),
             "sup selector must exist in stylesheet");
}

void ReaderStylesheetBuilderTest::testSummaryUsesHeadingText()
{
    ReaderAppearanceSettings s;
    s.headingText = QStringLiteral("#FFAA00");
    s.mainText    = QStringLiteral("#000022");
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(s);

    QVERIFY2(css.contains("p.html-summary {") && css.contains("color: #FFAA00;"),
             "p.html-summary must use headingText for color");
    QVERIFY2(css.contains("font-weight: 700;"),
             "p.html-summary must have font-weight: 700");
}

void ReaderStylesheetBuilderTest::testHtmlBlockUsesMainTextAndHorizontalRule()
{
    ReaderAppearanceSettings s;
    s.mainText        = QStringLiteral("#EEEE00");
    s.horizontalRule  = QStringLiteral("#888888");
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(s);

    QVERIFY2(css.contains("td.html-block-cell {") && css.contains("color: #EEEE00;"),
             "td.html-block-cell must use mainText for color");
    QVERIFY2(css.contains("border: 1px solid #888888;"),
             "td.html-block-cell border must use horizontalRule");
}

void ReaderStylesheetBuilderTest::testBodyFontStackIncludesEmojiFallbacks()
{
    auto settings = ReaderAppearanceDefaults::calmPalette();
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(settings);

    int bodyStart = css.indexOf("body {");
    QVERIFY2(bodyStart >= 0, "body selector must exist");
    int bodyEnd = css.indexOf('}', bodyStart);
    QString bodyBlock = css.mid(bodyStart, bodyEnd - bodyStart + 1);

    QVERIFY2(bodyBlock.contains("Segoe UI"),
             "body font-family must include Segoe UI");
    QVERIFY2(bodyBlock.contains("Segoe UI Emoji"),
             "body font-family must include Segoe UI Emoji");
    QVERIFY2(bodyBlock.contains("Segoe UI Symbol"),
             "body font-family must include Segoe UI Symbol");
    QVERIFY2(bodyBlock.contains("Noto Color Emoji") ||
             bodyBlock.contains("Apple Color Emoji"),
             "body font-family must include a cross-platform emoji fallback");
}

void ReaderStylesheetBuilderTest::testDefinitionTableSelectorsExist()
{
    auto settings = ReaderAppearanceDefaults::calmPalette();
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(settings);

    QVERIFY2(css.contains("table.definition-table {"),
             "table.definition-table selector must exist");
    QVERIFY2(css.contains("td.definition-term {"),
             "td.definition-term selector must exist");
    QVERIFY2(css.contains("td.definition-description {"),
             "td.definition-description selector must exist");
}

void ReaderStylesheetBuilderTest::testDefinitionTermUsesHeadingText()
{
    ReaderAppearanceSettings s;
    s.headingText = QStringLiteral("#AAFF00");
    s.mainText    = QStringLiteral("#000011");
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(s);

    QVERIFY2(css.contains("td.definition-term {") && css.contains("color: #AAFF00;"),
             "definition-term must use headingText color");
    QVERIFY2(css.contains("font-weight: 700;"),
             "definition-term must have font-weight: 700");
}

void ReaderStylesheetBuilderTest::testDefinitionDescriptionUsesMainText()
{
    ReaderAppearanceSettings s;
    s.mainText = QStringLiteral("#00EEEE");
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(s);

    QVERIFY2(css.contains("td.definition-description {") && css.contains("color: #00EEEE;"),
             "definition-description must use mainText color");
}

void ReaderStylesheetBuilderTest::testTaskListItemCssExists()
{
    auto settings = ReaderAppearanceDefaults::calmPalette();
    QString css = ReaderStylesheetBuilder::buildDocumentStylesheet(settings);

    QVERIFY2(css.contains("li.task-list-item {") && css.contains("list-style-type: none;"),
             "li.task-list-item must remove default list bullet");
}

QTEST_MAIN(ReaderStylesheetBuilderTest)
#include "ReaderStylesheetBuilderTest.moc"
