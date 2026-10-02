#include "HtmlElementWrapper.h"

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
//
// Single linear pass. When a bar cell is immediately followed by a blockquote
// cell that the RTL stage marked dir="rtl", the bar is dropped there and
// re-emitted right after that cell's matching </td> (tracked with a stack, so
// nested tables inside the quote — code blocks, nested quotes — are handled).
QString HtmlElementWrapper::swapBlockquoteOrderForRtl(const QString &html)
{
    static const QString barCell = QStringLiteral(
        "<td class=\"blockquote-bar\" width=\"4\"></td>");
    static const QString contentCellPrefix = QStringLiteral("<td class=\"blockquote-cell\"");
    static const QString rtlMarker = QStringLiteral("dir=\"rtl\"");

    const QStringView view(html);
    QString out;
    out.reserve(html.size());

    // One entry per open <td>: true when the bar must be emitted after its </td>.
    std::vector<bool> openCells;
    bool barPending = false;
    qsizetype pos = 0;

    while (pos < view.size()) {
        const qsizetype lt = view.indexOf(u'<', pos);
        if (lt < 0) {
            out += view.sliced(pos);
            break;
        }
        out += view.sliced(pos, lt - pos);

        const QStringView rest = view.sliced(lt);
        if (rest.startsWith(barCell)) {
            const QStringView next = rest.sliced(barCell.size());
            const qsizetype nextEnd = next.indexOf(u'>');
            if (next.startsWith(contentCellPrefix) && nextEnd >= 0
                && next.first(nextEnd).contains(rtlMarker)) {
                barPending = true; // drop the bar here, re-emit after the cell
                pos = lt + barCell.size();
                continue;
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
            openCells.push_back(barPending);
            barPending = false;
        } else if (tag == u"</td>" && !openCells.empty()) {
            const bool emitBar = openCells.back();
            openCells.pop_back();
            if (emitBar)
                out += barCell;
        }
    }

    return out;
}
