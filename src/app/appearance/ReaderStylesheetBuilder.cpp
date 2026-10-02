#include "ReaderStylesheetBuilder.h"

namespace {

// Relative luminance of "#RRGGBB" (0 = black, 1 = white); invalid input counts as light.
double luminance(const QString &hex)
{
    if (hex.size() != 7 || !hex.startsWith(u'#'))
        return 1.0;
    bool ok = false;
    const uint rgb = hex.mid(1).toUInt(&ok, 16);
    if (!ok)
        return 1.0;
    const double r = ((rgb >> 16) & 0xFF) / 255.0;
    const double g = ((rgb >> 8) & 0xFF) / 255.0;
    const double b = (rgb & 0xFF) / 255.0;
    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

// Syntax colours (GitHub light / dark palettes), picked by page brightness so
// code stays readable in day, night and custom themes.
QString syntaxStylesheet(const QString &pageBackground)
{
    const bool dark = luminance(pageBackground) < 0.5;
    const char *keyword = dark ? "#ff7b72" : "#cf222e";
    const char *type    = dark ? "#d2a8ff" : "#8250df";
    const char *string  = dark ? "#a5d6ff" : "#0a3069";
    const char *comment = dark ? "#8b949e" : "#6e7781";
    const char *number  = dark ? "#79c0ff" : "#0550ae";
    const char *preproc = dark ? "#ffa657" : "#953800";
    return QStringLiteral(
               "span.tok-keyword { color: %1; font-weight: 600; }"
               "span.tok-type { color: %2; }"
               "span.tok-string { color: %3; }"
               "span.tok-comment { color: %4; font-style: italic; }"
               "span.tok-number { color: %5; }"
               "span.tok-preproc { color: %6; }")
        .arg(QLatin1StringView(keyword), QLatin1StringView(type), QLatin1StringView(string),
             QLatin1StringView(comment), QLatin1StringView(number), QLatin1StringView(preproc));
}

} // namespace

QString ReaderStylesheetBuilder::buildDocumentStylesheet(const ReaderAppearanceSettings &s)
{
    return QStringLiteral(
        "body {"
        "  font-family: 'Segoe UI', 'Segoe UI Emoji', 'Segoe UI Symbol', 'Noto Color Emoji', 'Apple Color Emoji', 'Helvetica Neue', Arial, sans-serif;"
        "  font-size: 14px;"
        "  line-height: 1.6;"
        "  color: %1;"
        "  background-color: %11;"
        "  margin: 16px;"
        "}"
        "strong { font-weight: 700; }"
        "em, i { color: %12; font-style: italic; }"
        "h1 { color: %2; font-size: 28px; margin: 28px 0 14px 0; font-weight: 700; }"
        "h2 { color: %2; font-size: 23px; margin: 26px 0 12px 0; font-weight: 700; }"
        "h3 { color: %2; font-size: 19px; margin: 22px 0 10px 0; font-weight: 650; }"
        "h4 { color: %2; font-size: 16px; margin: 18px 0 8px 0; font-weight: 650; }"
        "h5 { color: %2; font-size: 14px; margin: 16px 0 6px 0; font-weight: 650; }"
        "h6 { color: %2; font-size: 13px; margin: 14px 0 6px 0; font-weight: 600; }"
        "p { margin: 0 0 12px 0; line-height: 1.55; }"
        "ul, ol { margin-top: 6px; margin-bottom: 14px; margin-left: 22px; padding-left: 14px; -qt-list-indent: 1; }"
        "li { margin-top: 3px; margin-bottom: 5px; line-height: 1.45; }"
        "li.task-list-item { list-style-type: none; }"
        "code {"
        "  font-family: Consolas, 'Cascadia Mono', 'Courier New', monospace;"
        "  font-size: 0.95em;"
        "  color: %3;"
        "  background-color: %4;"
        "  padding: 1px 4px;"
        "}"
        "table.code-block-table { width: 100%; border-collapse: collapse; margin-top: 8px; margin-bottom: 18px; }"
        "td.code-block-cell { background-color: %6; border: 1px solid %10; padding: 10px 12px; }"
        "pre {"
        "  color: %5;"
        "  background-color: transparent;"
        "  font-family: Consolas, 'Cascadia Mono', 'Courier New', monospace;"
        "  font-size: 13px;"
        "  line-height: 1.45;"
        "  margin: 0;"
        "  padding: 0;"
        "  white-space: pre-wrap;"
        "  border: 0;"
        "}"
        "pre code {"
        "  color: %5;"
        "  background-color: transparent;"
        "  padding: 0;"
        "  font-size: 1em;"
        "}"
        "table.blockquote-table {"
        "  width: 100%;"
        "  margin-top: 12px;"
        "  margin-bottom: 18px;"
        "  border-collapse: collapse;"
        "}"
        "td.blockquote-bar {"
        "  background-color: %7;"
        "  width: 4px;"
        "}"
        "td.blockquote-cell {"
        "  color: %8;"
        "  padding: 6px 0 6px 14px;"
        "}"
        "a { color: %9; text-decoration: none; }"
        "a:hover { text-decoration: underline; }"
        "table { border-collapse: collapse; margin-top: 10px; margin-bottom: 18px; }"
        "th { font-weight: 700; padding: 6px 8px; border: 1px solid %10; background-color: %4; color: %2; }"
        "td { padding: 6px 8px; border: 1px solid %10; color: %1; }"
        "hr { border-top: 1px solid %10; margin-top: 22px; margin-bottom: 24px; }"
        "sub { font-size: 0.8em; }"
        "sup { font-size: 0.8em; }"
        "p.html-summary { color: %2; font-weight: 700; margin-top: 10px; margin-bottom: 6px; }"
        "table.html-block-table {"
        "  width: 100%;"
        "  margin-top: 8px;"
        "  margin-bottom: 12px;"
        "  border-collapse: collapse;"
        "}"
        "td.html-block-cell {"
        "  color: %1;"
        "  border: 1px solid %10;"
        "  padding: 8px 10px;"
        "}"
        "table.definition-table {"
        "  width: 100%;"
        "  margin-top: 8px;"
        "  margin-bottom: 18px;"
        "  border-collapse: collapse;"
        "}"
        "td.definition-term {"
        "  color: %2;"
        "  font-weight: 700;"
        "  padding: 6px 10px 6px 0;"
        "}"
        "td.definition-description {"
        "  color: %1;"
        "  padding: 6px 0;"
        "}"
    )
        .arg(s.mainText)           // %1  body color
        .arg(s.headingText)        // %2  h1-h6 color
        .arg(s.inlineCodeText)     // %3  code color
        .arg(s.inlineCodeBackground) // %4  code background
        .arg(s.codeBlockText)      // %5  pre color
        .arg(s.codeBlockBackground) // %6  pre background
        .arg(s.blockquoteBorder)   // %7  blockquote border-left
        .arg(s.blockquoteText)     // %8  blockquote text
        .arg(s.linkText)           // %9  link color
        .arg(s.horizontalRule)     // %10 hr border
        .arg(s.pageBackground)     // %11 body background
        .arg(s.mutedText)          // %12 em, i color
        + syntaxStylesheet(s.pageBackground);
}

QString ReaderStylesheetBuilder::buildWidgetStylesheet(const ReaderAppearanceSettings &s,
                                                       const QString &backgroundOverride)
{
    const QString background = backgroundOverride.isEmpty() ? s.pageBackground : backgroundOverride;
    return QStringLiteral("QTextBrowser { background-color: %1; color: %2; }")
        .arg(background, s.mainText);
}
