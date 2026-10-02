#include "InlineExtensions.h"

#include "EmojiShortcodes.h"
#include "HtmlTextRuns.h"

#include <QRegularExpression>

namespace mdbr::render {

namespace {

// Same colours as HtmlNormalizer's <mark> conversion: readable on light and dark pages.
constexpr auto kHighlightOpen = "<span style=\"background-color: #FFF3B0; color: #24292F;\">";

[[nodiscard]] QString replaceAll(QStringView text, const QRegularExpression &pattern,
                                 const std::function<QString(const QRegularExpressionMatch &)> &replace)
{
    QString out;
    qsizetype last = 0;
    auto it = pattern.globalMatchView(text);
    while (it.hasNext()) {
        const auto match = it.next();
        out += text.sliced(last, match.capturedStart() - last);
        out += replace(match);
        last = match.capturedEnd();
    }
    if (last == 0)
        return text.toString();
    out += text.sliced(last);
    return out;
}

} // namespace

QString applyHighlight(QStringView text)
{
    // ==x== with no space just inside the delimiters, not part of "===".
    static const QRegularExpression pattern(QStringLiteral(R"((?<!=)==(?=[^\s=])(.+?)(?<=[^\s=])==(?!=))"));
    if (!text.contains(u"=="))
        return text.toString();
    return replaceAll(text, pattern, [](const QRegularExpressionMatch &m) {
        return QString::fromLatin1(kHighlightOpen) + m.captured(1) + QStringLiteral("</span>");
    });
}

QString applySuperscript(QStringView text)
{
    // ^x^ with no whitespace inside (Pandoc rule), so "x^2 + y^3" stays literal.
    static const QRegularExpression pattern(QStringLiteral(R"(\^([^\s^]+)\^)"));
    if (!text.contains(u'^'))
        return text.toString();
    return replaceAll(text, pattern, [](const QRegularExpressionMatch &m) {
        return QStringLiteral("<sup>") + m.captured(1) + QStringLiteral("</sup>");
    });
}

QString applyEmojiShortcodes(QStringView text)
{
    static const QRegularExpression pattern(QStringLiteral(R"(:([a-z0-9_+\-]+):)"));
    if (!text.contains(u':'))
        return text.toString();
    return replaceAll(text, pattern, [](const QRegularExpressionMatch &m) {
        const auto emoji = emojiForShortcode(m.capturedView(1));
        return emoji ? *emoji : m.captured(0); // unknown names stay literal
    });
}

QString applyInlineExtensions(const QString &html, const InlineExtensionOptions &options)
{
    if (!options.highlight && !options.superscript && !options.emoji)
        return html;

    return transformTextRuns(html, [&options](QStringView text) {
        QString result = text.toString();
        if (options.emoji)
            result = applyEmojiShortcodes(result);
        if (options.highlight)
            result = applyHighlight(result);
        if (options.superscript)
            result = applySuperscript(result);
        return result;
    });
}

} // namespace mdbr::render
