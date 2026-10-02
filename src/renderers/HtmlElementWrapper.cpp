#include "HtmlElementWrapper.h"

#include <QRegularExpression>

#include <utility>
#include <vector>

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

// Wrap each <blockquote> in a table so QTextDocument can draw the quote bar.
//
// Only the outer kMaxBlockquoteTableDepth levels become tables. QTextDocument's
// nested-table layout cost roughly doubles per level (depth 14 ≈ 0.8 s, depth 20
// froze the app), so deeper levels keep their plain <blockquote> tags, which Qt
// renders as a cheap indent. Content is never dropped.
QString HtmlElementWrapper::wrapBlockquotes(const QString &html)
{
    static const QString openTag = QStringLiteral("<blockquote");
    static const QString closeTag = QStringLiteral("</blockquote>");
    static const QString tableOpen = QStringLiteral(
        "<table class=\"blockquote-table\" width=\"100%\" cellspacing=\"0\" cellpadding=\"0\">"
        "<tr>"
        "<td class=\"blockquote-bar\" width=\"4\"%1></td>"
        "<td class=\"blockquote-cell\" width=\"100%\">");
    // AlertRenderer marks alerts with data-alert-color; the bar takes that colour.
    static const QRegularExpression alertColor(QStringLiteral("data-alert-color=\"(#[0-9A-Fa-f]{6})\""));
    static const QString tableClose = QStringLiteral("</td></tr></table>");

    const QStringView view(html);
    QString out;
    out.reserve(html.size() + html.size() / 4);

    int depth = 0;
    qsizetype pos = 0;
    qsizetype nextOpen = view.indexOf(openTag);
    qsizetype nextClose = view.indexOf(closeTag);

    while (nextOpen >= 0 || nextClose >= 0) {
        const bool isClose = nextClose >= 0 && (nextOpen < 0 || nextClose < nextOpen);

        if (isClose) {
            out += view.sliced(pos, nextClose - pos);
            out += depth <= kMaxBlockquoteTableDepth ? QStringView(tableClose) : QStringView(closeTag);
            if (depth > 0)
                --depth;
            pos = nextClose + closeTag.size();
            nextClose = view.indexOf(closeTag, pos);
        } else {
            const qsizetype tagEnd = view.indexOf(u'>', nextOpen + openTag.size());
            if (tagEnd < 0)
                break;
            out += view.sliced(pos, nextOpen - pos);
            ++depth;
            const QStringView openTagText = view.sliced(nextOpen, tagEnd + 1 - nextOpen);
            if (depth <= kMaxBlockquoteTableDepth) {
                const auto color = alertColor.matchView(openTagText);
                out += tableOpen.arg(color.hasMatch()
                    ? QStringLiteral(" style=\"background-color: %1;\"").arg(color.captured(1))
                    : QString());
            } else {
                out += openTagText;
            }
            pos = tagEnd + 1;
            nextOpen = view.indexOf(openTag, pos);
        }
    }

    out += view.sliced(pos);
    return out;
}

// Swap bar and cell order inside blockquote-tables that contain RTL content.
// For RTL, the content cell must appear first so the bar renders on the right.
//
// Single linear pass. When a bar cell is immediately followed by a blockquote
// cell that the RTL stage marked dir="rtl", the bar is dropped there and
// re-emitted right after that cell's matching </td> (tracked with a stack, so
// nested tables inside the quote — code blocks, nested quotes — are handled).
QString HtmlElementWrapper::swapBlockquoteOrderForRtl(const QString &html)
{
    // Bars may carry an inline colour (alerts), so match by prefix up to </td>.
    static const QString barPrefix = QStringLiteral("<td class=\"blockquote-bar\"");
    static const QString cellEnd = QStringLiteral("</td>");
    static const QString contentCellPrefix = QStringLiteral("<td class=\"blockquote-cell\"");
    static const QString rtlMarker = QStringLiteral("dir=\"rtl\"");

    const QStringView view(html);
    QString out;
    out.reserve(html.size());

    // One entry per open <td>: the bar to emit after its </td> (empty = none).
    std::vector<QString> openCells;
    QString barPending;
    qsizetype pos = 0;

    while (pos < view.size()) {
        const qsizetype lt = view.indexOf(u'<', pos);
        if (lt < 0) {
            out += view.sliced(pos);
            break;
        }
        out += view.sliced(pos, lt - pos);

        const QStringView rest = view.sliced(lt);
        if (rest.startsWith(barPrefix)) {
            const qsizetype barEnd = rest.indexOf(cellEnd);
            if (barEnd >= 0) {
                const QStringView bar = rest.first(barEnd + cellEnd.size());
                const QStringView next = rest.sliced(bar.size());
                const qsizetype nextEnd = next.indexOf(u'>');
                if (next.startsWith(contentCellPrefix) && nextEnd >= 0
                    && next.first(nextEnd).contains(rtlMarker)) {
                    barPending = bar.toString(); // drop the bar here, re-emit after the cell
                    pos = lt + bar.size();
                    continue;
                }
            }
        }

        const qsizetype gt = view.indexOf(u'>', lt);
        if (gt < 0) {
            out += rest;
            break;
        }
        const QStringView tag = view.sliced(lt, gt - lt + 1);
        out += tag;
        pos = gt + 1;

        if (tag.startsWith(u"<td") && (tag.size() == 4 || tag[3] == u' ')) {
            openCells.push_back(std::move(barPending));
            barPending.clear();
        } else if (tag == u"</td>" && !openCells.empty()) {
            out += openCells.back();
            openCells.pop_back();
        }
    }

    return out;
}
