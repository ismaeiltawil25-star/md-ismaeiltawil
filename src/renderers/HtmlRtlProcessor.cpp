#include "HtmlRtlProcessor.h"

#include <QRegularExpression>

struct BlockRange { int openPos; int closePos; QString tag; };

static int findMatchingClose(const QString &html, int openPos, const QString &tag)
{
    QString openTag = "<" + tag;
    QString closeTag = "</" + tag + ">";
    int depth = 0;
    int j = openPos + openTag.size();

    while (j < html.size()) {
        j = html.indexOf('<', j);
        if (j < 0) return -1;

        if (html.mid(j).startsWith(openTag + ">") ||
            html.mid(j).startsWith(openTag + " ") ||
            html.mid(j).startsWith(openTag + "\t") ||
            html.mid(j).startsWith(openTag + "/"))
        {
            ++depth;
            j = html.indexOf('>', j) + 1;
            continue;
        }

        if (html.mid(j).startsWith(closeTag)) {
            if (depth == 0) return j;
            --depth;
            j += closeTag.size();
            continue;
        }

        if (tag == "li" || tag == "li ") {
            if (html.mid(j).startsWith("<ul") || html.mid(j).startsWith("<ol")) {
                ++depth;
                j = html.indexOf('>', j) + 1;
                continue;
            }
            if (html.mid(j).startsWith("</ul>") || html.mid(j).startsWith("</ol>")) {
                --depth;
                j += 5;
                continue;
            }
        }

        ++j;
    }
    return -1;
}

static bool containsRtlText(const QStringView &text)
{
    for (const QChar &c : text) {
        char16_t cp = c.unicode();
        // Arabic
        if ((cp >= 0x0600 && cp <= 0x06FF)) return true;
        // Arabic Supplement
        if ((cp >= 0x0750 && cp <= 0x077F)) return true;
        // Arabic Extended-A
        if ((cp >= 0x08A0 && cp <= 0x08FF)) return true;
        // Arabic Extended-B
        if ((cp >= 0x0870 && cp <= 0x089F)) return true;
        // Arabic Presentation Forms-A
        if ((cp >= 0xFB50 && cp <= 0xFDFF)) return true;
        // Arabic Presentation Forms-B
        if ((cp >= 0xFE70 && cp <= 0xFEFF)) return true;
        // Persian (Farsi) specific: extends into Arabic blocks covered above
        // Urdu specific: extends into Arabic blocks covered above
        // Syriac
        if ((cp >= 0x0700 && cp <= 0x074F)) return true;
        // Thaana
        if ((cp >= 0x0780 && cp <= 0x07BF)) return true;
        // N'Ko
        if ((cp >= 0x07C0 && cp <= 0x07FF)) return true;
        // Samaritan
        if ((cp >= 0x0800 && cp <= 0x083F)) return true;
        // Mandaic
        if ((cp >= 0x0840 && cp <= 0x085F)) return true;
        // Hebrew
        if ((cp >= 0x0590 && cp <= 0x05FF)) return true;
    }
    return false;
}

QString HtmlRtlProcessor::applyRtl(const QString &html)
{
    static const QStringList blockTags = {
        "p", "li", "h1", "h2", "h3", "h4", "h5", "h6"
    };

    QList<BlockRange> ranges;

    int skipDepth = 0;
    int i = 0;
    while (i < html.size()) {
        int lt = html.indexOf('<', i);
        if (lt < 0) break;

        int gt = html.indexOf('>', lt);
        if (gt < 0) break;

        int nameEnd = html.indexOf(QChar(' '), lt + 1);
        if (nameEnd < 0 || nameEnd > gt) nameEnd = gt;
        QString tagName = html.mid(lt + 1, nameEnd - lt - 1);

        if (tagName == "pre" || tagName == "code") {
            ++skipDepth;
            i = gt + 1;
            continue;
        }
        if (tagName.startsWith('/') && (tagName.mid(1) == "pre" || tagName.mid(1) == "code")) {
            if (skipDepth > 0) --skipDepth;
            i = gt + 1;
            continue;
        }

        if (skipDepth > 0) {
            i = gt + 1;
            continue;
        }

        if (tagName == "td" && html.mid(lt, gt - lt + 1).contains("code-block-cell")) {
            ++skipDepth;
            i = gt + 1;
            continue;
        }
        if (tagName == "/td" && html.mid(lt, gt - lt + 1).contains("/td")) {
            if (skipDepth > 0) --skipDepth;
            i = gt + 1;
            continue;
        }

        for (const QString &btag : blockTags) {
            if (tagName == btag) {
                int closePos = findMatchingClose(html, lt, btag);
                if (closePos > lt) {
                    ranges.append({lt, closePos, btag});
                }
                break;
            }
        }

        if (tagName == "td" && html.mid(lt, gt - lt + 1).contains("blockquote-cell")) {
            int closePos = findMatchingClose(html, lt, "td");
            if (closePos > lt) {
                ranges.append({lt, closePos, "td"});
            }
        }

        i = gt + 1;
    }

    std::sort(ranges.begin(), ranges.end(),
              [](const BlockRange &a, const BlockRange &b) {
                  return a.openPos > b.openPos;
              });

    QString result = html;
    for (const BlockRange &br : ranges) {
        int contentStart = result.indexOf('>', br.openPos) + 1;
        if (contentStart <= 0 || contentStart >= br.closePos) continue;

        QString innerHtml = result.mid(contentStart, br.closePos - contentStart);
        QString textOnly = innerHtml;
        textOnly.remove(QRegularExpression("<[^>]*>"));
        if (textOnly.trimmed().isEmpty()) continue;

        if (containsRtlText(QStringView(textOnly))) {
            int tagEnd = result.indexOf('>', br.openPos);
            if (tagEnd < 0 || tagEnd >= result.size()) continue;

            QString toInsert = QStringLiteral(" dir=\"rtl\" align=\"right\"");
            result.insert(tagEnd, toInsert);
        }
    }

    return result;
}
