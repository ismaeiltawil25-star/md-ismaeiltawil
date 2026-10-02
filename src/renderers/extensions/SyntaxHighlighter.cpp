#include "SyntaxHighlighter.h"

#include "HtmlEntities.h"
#include "SyntaxLanguages.h"

#include <QRegularExpression>

namespace mdbr::render {

namespace {

class Tokenizer
{
public:
    Tokenizer(QStringView source, const LanguageRules &rules) : m_src(source), m_rules(rules)
    {
        m_out.reserve(source.size() + source.size() / 3);
    }

    QString run()
    {
        while (m_pos < m_src.size()) {
            if (blockComment() || lineComment() || preprocessor() || markupTag() || string()
                || number() || word())
                continue;
            plain(m_src[m_pos]);
            ++m_pos;
        }
        return m_out;
    }

private:
    [[nodiscard]] bool startsWithAt(QStringView token) const
    {
        return !token.isEmpty() && m_src.sliced(m_pos).startsWith(token);
    }

    [[nodiscard]] bool isWordChar(QChar c) const
    {
        return c.isLetterOrNumber() || c == u'_' || c == u'$' || (m_rules.dashInWords && c == u'-');
    }

    void emitToken(const char *cls, qsizetype end)
    {
        m_out += QStringLiteral("<span class=\"tok-%1\">").arg(QLatin1StringView(cls));
        m_out += escapeHtmlAttribute(m_src.sliced(m_pos, end - m_pos));
        m_out += QStringLiteral("</span>");
        m_lineStart = m_src.sliced(m_pos, end - m_pos).endsWith(u'\n');
        m_pos = end;
    }

    void plain(QChar c)
    {
        m_out += escapeHtmlAttribute(QStringView(&c, 1));
        if (c == u'\n')
            m_lineStart = true;
        else if (!c.isSpace())
            m_lineStart = false;
    }

    [[nodiscard]] qsizetype endOfLine(qsizetype from) const
    {
        const qsizetype nl = m_src.indexOf(u'\n', from);
        return nl < 0 ? m_src.size() : nl;
    }

    bool blockComment()
    {
        if (!startsWithAt(m_rules.blockCommentOpen))
            return false;
        const qsizetype close = m_src.indexOf(m_rules.blockCommentClose, m_pos + m_rules.blockCommentOpen.size());
        emitToken("comment", close < 0 ? m_src.size() : close + m_rules.blockCommentClose.size());
        return true;
    }

    bool lineComment()
    {
        for (const QString &marker : m_rules.lineComments) {
            if (!startsWithAt(marker))
                continue;
            // "#" inside a shell word ("$#", "a#b") is not a comment.
            if (marker == u"#" && m_pos > 0 && isWordChar(m_src[m_pos - 1]))
                return false;
            emitToken("comment", endOfLine(m_pos));
            return true;
        }
        return false;
    }

    bool preprocessor()
    {
        if (!m_rules.hashPreprocessor || !m_lineStart || m_src[m_pos] != u'#')
            return false;
        emitToken("preproc", endOfLine(m_pos));
        return true;
    }

    bool markupTag()
    {
        if (!m_rules.markup || m_src[m_pos] != u'<')
            return false;
        const qsizetype close = m_src.indexOf(u'>', m_pos);
        emitToken("keyword", close < 0 ? m_src.size() : close + 1);
        return true;
    }

    bool string()
    {
        const QChar quote = m_src[m_pos];
        if (!m_rules.stringQuotes.contains(quote))
            return false;

        // Triple-quoted strings (Python docstrings).
        const QString triple(3, quote);
        if (quote != u'`' && startsWithAt(triple)) {
            const qsizetype close = m_src.indexOf(triple, m_pos + 3);
            emitToken("string", close < 0 ? m_src.size() : close + 3);
            return true;
        }

        qsizetype j = m_pos + 1;
        while (j < m_src.size()) {
            const QChar c = m_src[j];
            if (c == u'\\') { j += 2; continue; }
            if (c == quote) { ++j; break; }
            if (c == u'\n' && quote != u'`') break; // unterminated: stop at end of line
            ++j;
        }
        emitToken("string", qMin(j, m_src.size()));
        return true;
    }

    bool number()
    {
        const QChar c = m_src[m_pos];
        const bool startsNumber = c.isDigit()
            || (c == u'.' && m_pos + 1 < m_src.size() && m_src[m_pos + 1].isDigit());
        if (!startsNumber || (m_pos > 0 && isWordChar(m_src[m_pos - 1])))
            return false;
        qsizetype j = m_pos + 1;
        while (j < m_src.size() && (m_src[j].isLetterOrNumber() || m_src[j] == u'.' || m_src[j] == u'_'
                                    || m_src[j] == u'\''))
            ++j;
        emitToken("number", j);
        return true;
    }

    bool word()
    {
        const QChar c = m_src[m_pos];
        if (!(c.isLetter() || c == u'_' || c == u'$' || c == u'@'))
            return false;
        qsizetype j = m_pos + 1;
        while (j < m_src.size() && isWordChar(m_src[j]))
            ++j;
        const QString text = m_src.sliced(m_pos, j - m_pos).toString();
        const QString key = m_rules.caseInsensitive ? text.toLower() : text;
        if (m_rules.keywords.contains(key))
            emitToken("keyword", j);
        else if (m_rules.types.contains(key))
            emitToken("type", j);
        else {
            m_out += escapeHtmlAttribute(text);
            m_lineStart = false;
            m_pos = j;
        }
        return true;
    }

    QStringView m_src;
    const LanguageRules &m_rules;
    QString m_out;
    qsizetype m_pos = 0;
    bool m_lineStart = true;
};

} // namespace

bool isKnownLanguage(const QString &language)
{
    return findLanguage(language) != nullptr;
}

QString highlightSource(const QString &source, const QString &language)
{
    const LanguageRules *rules = findLanguage(language);
    if (!rules)
        return escapeHtmlAttribute(source);
    return Tokenizer(source, *rules).run();
}

QString highlightCodeBlocks(const QString &html)
{
    static const QString closeTag = QStringLiteral("</code></pre>");
    static const QRegularExpression openTag(QStringLiteral(R"re(<pre><code class="language-([^"\s]+)">)re"));

    if (!html.contains(QStringLiteral("<code class=\"language-")))
        return html;

    QString out;
    out.reserve(html.size() + html.size() / 4);
    qsizetype pos = 0;
    auto it = openTag.globalMatch(html);
    while (it.hasNext()) {
        const auto m = it.next();
        if (m.capturedStart() < pos)
            continue; // inside a block we already copied
        const qsizetype contentStart = m.capturedEnd();
        const qsizetype contentEnd = html.indexOf(closeTag, contentStart);
        if (contentEnd < 0)
            break;

        out += QStringView(html).sliced(pos, contentStart - pos);
        const QStringView content = QStringView(html).sliced(contentStart, contentEnd - contentStart);
        const QString language = m.captured(1);
        if (content.size() <= kMaxHighlightChars && isKnownLanguage(language))
            out += highlightSource(decodeHtmlEntities(content), language);
        else
            out += content;
        pos = contentEnd;
    }
    out += QStringView(html).sliced(pos);
    return out;
}

} // namespace mdbr::render
