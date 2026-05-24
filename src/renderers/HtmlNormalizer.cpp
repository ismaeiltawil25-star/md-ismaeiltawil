#include "HtmlNormalizer.h"

#include <QRegularExpression>

QString HtmlNormalizer::normalizeRawTags(const QString &html)
{
    QString out = html;

    // 1. <kbd>content</kbd> → <code>
    out.replace(QRegularExpression(QStringLiteral("<kbd>([^<]*)</kbd>")),
                QStringLiteral("<code>\\1</code>"));

    // 2. <mark>content</mark> → inline-styled span
    out.replace(QRegularExpression(QStringLiteral("<mark>([^<]*)</mark>")),
                QStringLiteral("<span style=\"background-color: #FFF3B0; color: #24292F;\">\\1</span>"));

    // 3. <summary>content</summary> → <p class="html-summary">content</p>
    out.replace(QRegularExpression(QStringLiteral("<summary>([^<]*)</summary>")),
                QStringLiteral("<p class=\"html-summary\">\\1</p>"));

    // 4. Strip <details> and </details>
    out.replace(QRegularExpression(QStringLiteral("<details[^>]*>")), QString());
    out.replace(QStringLiteral("</details>"), QString());

    // 5. Simple <div ...>content</div> → table wrapper
    out.replace(QRegularExpression(QStringLiteral("<div[^>]*>(.*?)</div>"),
                                   QRegularExpression::DotMatchesEverythingOption),
                QStringLiteral("<table class=\"html-block-table\" width=\"100%\""
                               " cellspacing=\"0\" cellpadding=\"0\">"
                               "<tr><td class=\"html-block-cell\">"
                               "\\1"
                               "</td></tr></table>"));

    return out;
}
