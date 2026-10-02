#include "HtmlNormalizer.h"

#include <QRegularExpression>

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

    // 5. <div ...> / </div> → table wrapper. Opening and closing tags are
    //    replaced independently, so nested divs stay balanced and in order.
    //    (A single non-greedy <div>(.*?)</div> regex paired an outer opening tag
    //    with the inner closing tag and pushed trailing content outside.)
    static const QRegularExpression divOpen(QStringLiteral("<div(?:\\s[^>]*)?>"),
                                            QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression divClose(QStringLiteral("</div\\s*>"),
                                             QRegularExpression::CaseInsensitiveOption);
    out.replace(divOpen, QStringLiteral("<table class=\"html-block-table\" width=\"100%\""
                                        " cellspacing=\"0\" cellpadding=\"0\">"
                                        "<tr><td class=\"html-block-cell\">"));
    out.replace(divClose, QStringLiteral("</td></tr></table>"));

    // 6. <del> (md4c strikethrough) → <s>, which QTextDocument renders as strike-out.
    out.replace(QStringLiteral("<del>"), QStringLiteral("<s>"));
    out.replace(QStringLiteral("</del>"), QStringLiteral("</s>"));

    return out;
}
