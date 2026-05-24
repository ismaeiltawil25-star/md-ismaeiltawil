#include "HtmlElementWrapper.h"

// Wrap each fenced code block (<pre><code...>...</code></pre>) in a table+cell
QString HtmlElementWrapper::wrapCodeBlocks(const QString &html)
{
    QString out;
    int pos = 0;

    while (pos < html.size()) {
        int preIdx = html.indexOf("<pre><code", pos);
        if (preIdx < 0) {
            out.append(html.mid(pos));
            break;
        }

        out.append(html.mid(pos, preIdx - pos));

        int codeEnd = html.indexOf('>', preIdx + 10);
        if (codeEnd < 0) {
            out.append(html.mid(preIdx));
            break;
        }

        int closeIdx = html.indexOf("</code></pre>", codeEnd + 1);
        if (closeIdx < 0) {
            out.append(html.mid(preIdx));
            break;
        }

        int closeEnd = closeIdx + 13;
        QString block = html.mid(preIdx, closeEnd - preIdx);

        out.append(QStringLiteral(
            "<table class=\"code-block-table\" cellspacing=\"0\" cellpadding=\"0\">"
            "<tr><td class=\"code-block-cell\">"));
        out.append(block);
        out.append(QStringLiteral("</td></tr></table>"));

        pos = closeEnd;
    }

    return out;
}

// Wrap each <blockquote> tag with a table+cell
QString HtmlElementWrapper::wrapBlockquotes(const QString &html)
{
    QString out;
    int pos = 0;

    while (pos < html.size()) {
        int openIdx = html.indexOf("<blockquote", pos);
        int closeIdx = html.indexOf("</blockquote>", pos);

        if (openIdx < 0 && closeIdx < 0) {
            out.append(html.mid(pos));
            break;
        }

        if (closeIdx >= 0 && (openIdx < 0 || closeIdx < openIdx)) {
            out.append(html.mid(pos, closeIdx - pos));
            out.append(QStringLiteral("</td></tr></table>"));
            pos = closeIdx + 13;
        } else {
            out.append(html.mid(pos, openIdx - pos));
            int tagEnd = html.indexOf('>', openIdx + 11);
            if (tagEnd < 0) {
                out.append(html.mid(openIdx));
                break;
            }
            out.append(QStringLiteral(
                "<table class=\"blockquote-table\" width=\"100%\" cellspacing=\"0\" cellpadding=\"0\">"
                "<tr>"
                "<td class=\"blockquote-bar\" width=\"4\"></td>"
                "<td class=\"blockquote-cell\" width=\"100%\">"));
            pos = tagEnd + 1;
        }
    }

    return out;
}

// Swap bar and cell order inside blockquote-tables that contain RTL content.
// For RTL, the content cell must appear first so the bar renders on the right.
QString HtmlElementWrapper::swapBlockquoteOrderForRtl(const QString &html)
{
    QString result = html;
    int searchPos = 0;

    while (true) {
        int tableStart = result.indexOf(QStringLiteral("<table class=\"blockquote-table\""), searchPos);
        if (tableStart < 0) break;

        int closeTable = result.indexOf(QStringLiteral("</table>"), tableStart);
        if (closeTable < 0) break;
        closeTable += 8;

        int trStart = result.indexOf(QStringLiteral("<tr>"), tableStart);
        int trEnd = result.indexOf(QStringLiteral("</tr>"), trStart);
        if (trStart < 0 || trEnd < 0 || trEnd > closeTable) {
            searchPos = closeTable;
            continue;
        }

        QString tableInner = result.mid(trStart, trEnd - trStart + 5);
        if (!tableInner.contains(QStringLiteral("dir=\"rtl\""))) {
            searchPos = closeTable;
            continue;
        }

        int barStart = result.indexOf(QStringLiteral("<td class=\"blockquote-bar\""), trStart);
        int barEnd = result.indexOf(QStringLiteral("</td>"), barStart);
        if (barStart < 0 || barEnd < 0 || barEnd > trEnd) {
            searchPos = closeTable;
            continue;
        }
        barEnd += 5;

        int cellStart = result.indexOf(QStringLiteral("<td class=\"blockquote-cell\""), trStart);
        int cellEnd = result.indexOf(QStringLiteral("</td>"), cellStart);
        if (cellStart < 0 || cellEnd < 0 || cellEnd > trEnd) {
            searchPos = closeTable;
            continue;
        }
        cellEnd += 5;

        QString barSnippet = result.mid(barStart, barEnd - barStart);
        QString cellSnippet = result.mid(cellStart, cellEnd - cellStart);

        result.replace(cellStart, cellEnd - cellStart, barSnippet);
        result.replace(barStart, barEnd - barStart, cellSnippet);

        searchPos = barStart + cellSnippet.size() + 1;
    }

    return result;
}
