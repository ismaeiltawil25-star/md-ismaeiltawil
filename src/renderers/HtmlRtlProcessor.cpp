#include "HtmlRtlProcessor.h"

#include <QHash>

#include <algorithm>
#include <vector>

// Algorithm — one linear pass over the HTML:
//   * Candidate blocks (p, li, h1-h6, td.blockquote-cell) are pushed on a stack
//     when opened and popped when closed.
//   * A text run containing RTL characters marks the innermost open element;
//     when an element closes, its mark propagates to its parent, so a block is
//     RTL when ANY text inside it (including nested blocks) is RTL.
//   * Closed, marked candidates get dir="rtl" align="right" inserted into their
//     opening tag. All insertions are applied in a final O(n) copy.
//   * Inside <pre>/<code> and td.code-block-cell no new candidates are created
//     (code is never re-aligned), but code text still counts for its container.
//
// Complexity is O(n) in the HTML length; the previous implementation re-scanned
// and copied the remainder of the document per tag and froze on deep nesting.

namespace {

constexpr QStringView kRtlAttributes = u" dir=\"rtl\" align=\"right\"";

[[nodiscard]] constexpr bool isRtlCodePoint(char16_t cp)
{
    return (cp >= 0x0590 && cp <= 0x05FF)   // Hebrew
        || (cp >= 0x0600 && cp <= 0x06FF)   // Arabic
        || (cp >= 0x0700 && cp <= 0x074F)   // Syriac
        || (cp >= 0x0750 && cp <= 0x077F)   // Arabic Supplement
        || (cp >= 0x0780 && cp <= 0x07BF)   // Thaana
        || (cp >= 0x07C0 && cp <= 0x07FF)   // N'Ko
        || (cp >= 0x0800 && cp <= 0x083F)   // Samaritan
        || (cp >= 0x0840 && cp <= 0x085F)   // Mandaic
        || (cp >= 0x0870 && cp <= 0x089F)   // Arabic Extended-B
        || (cp >= 0x08A0 && cp <= 0x08FF)   // Arabic Extended-A
        || (cp >= 0xFB50 && cp <= 0xFDFF)   // Arabic Presentation Forms-A
        || (cp >= 0xFE70 && cp <= 0xFEFF);  // Arabic Presentation Forms-B
}

[[nodiscard]] bool containsRtlText(QStringView text)
{
    return std::any_of(text.begin(), text.end(),
                       [](QChar c) { return isRtlCodePoint(c.unicode()); });
}

[[nodiscard]] bool isBlockCandidate(QStringView name)
{
    return name == u"p" || name == u"li"
        || (name.size() == 2 && name[0] == u'h' && name[1] >= u'1' && name[1] <= u'6');
}

[[nodiscard]] bool isTrackedElement(QStringView name)
{
    return isBlockCandidate(name) || name == u"td";
}

struct OpenElement
{
    QStringView name;
    qsizetype attributeInsertPos = 0; // index of the opening tag's '>'
    bool isCandidate = false;         // may receive dir="rtl"
    bool startsCodeRegion = false;    // td.code-block-cell
    bool hasRtlText = false;
};

class RtlScanner
{
public:
    explicit RtlScanner(QStringView html) : m_html(html) {}

    std::vector<qsizetype> run()
    {
        qsizetype pos = 0;
        while (pos < m_html.size()) {
            const qsizetype lt = m_html.indexOf(u'<', pos);
            if (lt < 0) {
                onText(m_html.sliced(pos));
                break;
            }
            onText(m_html.sliced(pos, lt - pos));

            const qsizetype gt = m_html.indexOf(u'>', lt);
            if (gt < 0)
                break;
            onTag(m_html.sliced(lt, gt - lt + 1), gt);
            pos = gt + 1;
        }
        std::sort(m_insertPositions.begin(), m_insertPositions.end());
        return std::move(m_insertPositions);
    }

private:
    void onText(QStringView text)
    {
        if (!m_stack.empty() && !m_stack.back().hasRtlText && containsRtlText(text))
            m_stack.back().hasRtlText = true;
    }

    void onTag(QStringView tag, qsizetype gtPos)
    {
        // tag is "<...>"; the name ends at the first space or the closing '>'.
        QStringView inner = tag.sliced(1, tag.size() - 2);
        const qsizetype space = inner.indexOf(u' ');
        const QStringView name = space < 0 ? inner : inner.first(space);

        if (name.startsWith(u'/'))
            onCloseTag(name.sliced(1));
        else
            onOpenTag(name, tag, gtPos);
    }

    void onOpenTag(QStringView name, QStringView tag, qsizetype gtPos)
    {
        if (name == u"pre" || name == u"code") {
            ++m_codeDepth;
            return;
        }
        if (m_codeDepth > 0)
            return;

        if (name == u"td") {
            const bool codeCell = tag.contains(u"code-block-cell");
            push({name, gtPos, tag.contains(u"blockquote-cell"), codeCell});
            if (codeCell)
                ++m_codeDepth;
        } else if (isBlockCandidate(name)) {
            push({name, gtPos, true, false});
        }
    }

    void onCloseTag(QStringView name)
    {
        if (name == u"pre" || name == u"code") {
            if (m_codeDepth > 0)
                --m_codeDepth;
            return;
        }
        if (!isTrackedElement(name))
            return;
        if (m_codeDepth > 0 && name != u"td")
            return;
        if (m_openCount.value(name) == 0)
            return; // stray closing tag

        // Elements left open above the match were never closed: discard them
        // (their RTL mark still propagates to the parent).
        while (m_stack.back().name != name)
            pop(/*closed=*/false);
        pop(/*closed=*/true);
    }

    void push(OpenElement element)
    {
        ++m_openCount[element.name];
        m_stack.push_back(element);
    }

    void pop(bool closed)
    {
        const OpenElement element = m_stack.back();
        m_stack.pop_back();
        --m_openCount[element.name];

        if (element.startsCodeRegion && m_codeDepth > 0)
            --m_codeDepth;
        if (!element.hasRtlText)
            return;
        if (closed && element.isCandidate)
            m_insertPositions.push_back(element.attributeInsertPos);
        if (!m_stack.empty())
            m_stack.back().hasRtlText = true;
    }

    QStringView m_html;
    std::vector<OpenElement> m_stack;
    QHash<QStringView, int> m_openCount;
    std::vector<qsizetype> m_insertPositions;
    int m_codeDepth = 0;
};

} // namespace

QString HtmlRtlProcessor::applyRtl(const QString &html)
{
    const std::vector<qsizetype> positions = RtlScanner(html).run();
    if (positions.empty())
        return html;

    QString result;
    result.reserve(html.size() + qsizetype(positions.size()) * kRtlAttributes.size());
    qsizetype copied = 0;
    for (const qsizetype pos : positions) {
        result += QStringView(html).sliced(copied, pos - copied);
        result += kRtlAttributes;
        copied = pos;
    }
    result += QStringView(html).sliced(copied);
    return result;
}
