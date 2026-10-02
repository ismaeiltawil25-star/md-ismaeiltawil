#include "HtmlNormalizer.h"

#include <QRegularExpression>

#include <vector>

namespace {

// Converts <div> blocks to single-cell tables so QTextDocument draws their
// border/background. Opening and closing tags are matched with a stack, so
// nested divs stay balanced (a non-greedy <div>(.*?)</div> regex used to pair
// an outer opening tag with an inner closing tag).
//
// Only the outer kMaxDivTableDepth levels become tables: QTextDocument's
// nested-table layout cost grows exponentially with depth. Deeper divs keep
// their tags, which Qt renders as plain blocks.
QString convertDivsToTables(const QString &html)
{
    static const QRegularExpression divTag(QStringLiteral("<(/?)div(?:\\s[^>]*)?\\s*>"),
                                           QRegularExpression::CaseInsensitiveOption);
    static const QString tableOpen = QStringLiteral(
        "<table class=\"html-block-table\" width=\"100%\" cellspacing=\"0\" cellpadding=\"0\">"
        "<tr><td class=\"html-block-cell\">");
    static const QString tableClose = QStringLiteral("</td></tr></table>");

    QString out;
    out.reserve(html.size());
    std::vector<bool> converted; // one entry per open <div>
    int tableDepth = 0;
    qsizetype pos = 0;

    auto it = divTag.globalMatch(html);
    while (it.hasNext()) {
        const auto match = it.next();
        out += QStringView(html).sliced(pos, match.capturedStart() - pos);
        pos = match.capturedEnd();

        const bool isClose = !match.capturedView(1).isEmpty();
        if (!isClose) {
            const bool asTable = tableDepth < HtmlNormalizer::kMaxDivTableDepth;
            converted.push_back(asTable);
            if (asTable)
                ++tableDepth;
            out += asTable ? QStringView(tableOpen) : match.capturedView();
        } else if (!converted.empty()) {
            const bool wasTable = converted.back();
            converted.pop_back();
            if (wasTable)
                --tableDepth;
            out += wasTable ? QStringView(tableClose) : match.capturedView();
        } else {
            out += match.capturedView(); // stray </div>: leave it for Qt to ignore
        }
    }
    out += QStringView(html).sliced(pos);
    return out;
}

} // namespace

QString HtmlNormalizer::normalizeRawTags(const QString &html)
{
    QString out = html;

    // 1. <kbd ...>content</kbd> → <code>
    out.replace(QRegularExpression(QStringLiteral("<kbd(\\s[^>]*)?>([^<]*)</kbd>")),
                QStringLiteral("<code>\\2</code>"));

    // 2. <mark ...>content</mark> → inline-styled span
    out.replace(QRegularExpression(QStringLiteral("<mark(\\s[^>]*)?>([^<]*)</mark>")),
                QStringLiteral("<span style=\"background-color: #FFF3B0; color: #24292F;\">\\2</span>"));

    // 3. <summary ...>content</summary> → <p class="html-summary">content</p>
    out.replace(QRegularExpression(QStringLiteral("<summary(\\s[^>]*)?>([^<]*)</summary>")),
                QStringLiteral("<p class=\"html-summary\">\\2</p>"));

    // 4. Strip <details> and </details>
    out.replace(QRegularExpression(QStringLiteral("<details[^>]*>")), QString());
    out.replace(QStringLiteral("</details>"), QString());

    // 5. <div ...> / </div> → table wrapper (see convertDivsToTables).
    out = convertDivsToTables(out);

    // 6. <del> (md4c strikethrough) → <s>, which QTextDocument renders as strike-out.
    out.replace(QStringLiteral("<del>"), QStringLiteral("<s>"));
    out.replace(QStringLiteral("</del>"), QStringLiteral("</s>"));

    return out;
}
