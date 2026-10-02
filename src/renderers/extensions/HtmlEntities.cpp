#include "HtmlEntities.h"

namespace mdbr::render {

QString decodeHtmlEntities(QStringView text)
{
    if (!text.contains(u'&'))
        return text.toString();

    QString out;
    out.reserve(text.size());
    qsizetype i = 0;
    while (i < text.size()) {
        if (text[i] != u'&') {
            out += text[i++];
            continue;
        }
        const qsizetype semi = text.indexOf(u';', i);
        if (semi < 0 || semi - i > 10) {
            out += text[i++];
            continue;
        }
        const QStringView name = text.sliced(i + 1, semi - i - 1);
        QString decoded;
        if (name == u"amp")
            decoded = QStringLiteral("&");
        else if (name == u"lt")
            decoded = QStringLiteral("<");
        else if (name == u"gt")
            decoded = QStringLiteral(">");
        else if (name == u"quot")
            decoded = QStringLiteral("\"");
        else if (name == u"apos" || name == u"#39")
            decoded = QStringLiteral("'");
        else if (name.startsWith(u'#')) {
            bool ok = false;
            const uint code = (name.size() > 1 && (name[1] == u'x' || name[1] == u'X'))
                ? name.sliced(2).toUInt(&ok, 16)
                : name.sliced(1).toUInt(&ok, 10);
            if (ok && code > 0 && code <= 0x10FFFF) {
                const char32_t cp = code;
                decoded = QString::fromUcs4(&cp, 1);
            }
        }
        if (decoded.isEmpty()) {
            out += text[i++]; // unknown entity: keep as written
            continue;
        }
        out += decoded;
        i = semi + 1;
    }
    return out;
}

QString escapeHtmlAttribute(QStringView text)
{
    QString out;
    out.reserve(text.size());
    for (const QChar c : text) {
        switch (c.unicode()) {
        case u'&': out += QStringLiteral("&amp;"); break;
        case u'<': out += QStringLiteral("&lt;"); break;
        case u'>': out += QStringLiteral("&gt;"); break;
        case u'"': out += QStringLiteral("&quot;"); break;
        default:   out += c;
        }
    }
    return out;
}

} // namespace mdbr::render
