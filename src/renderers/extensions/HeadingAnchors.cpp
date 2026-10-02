#include "HeadingAnchors.h"

#include "HtmlEntities.h"

#include <QHash>
#include <QRegularExpression>

namespace mdbr::render {

QString slugify(QStringView headingText)
{
    QString slug;
    slug.reserve(headingText.size());
    for (const QChar c : headingText) {
        if (c.isLetterOrNumber() || c.isMark() || c == u'-' || c == u'_')
            slug += c.toLower();
        else if (c.isSpace())
            slug += u'-';
    }
    return slug;
}

QString addHeadingAnchors(const QString &html)
{
    // md4c emits headings as <hN>...</hN> without attributes.
    static const QRegularExpression heading(QStringLiteral(R"(<h([1-6])>(.*?)</h\1>)"),
                                            QRegularExpression::DotMatchesEverythingOption);
    static const QRegularExpression tags(QStringLiteral("<[^>]*>"));

    QHash<QString, int> used;
    QString out;
    out.reserve(html.size() + html.size() / 32);
    qsizetype last = 0;

    auto it = heading.globalMatch(html);
    while (it.hasNext()) {
        const auto match = it.next();
        out += QStringView(html).sliced(last, match.capturedStart() - last);
        last = match.capturedEnd();

        QString text = match.captured(2);
        text.remove(tags);
        QString slug = slugify(decodeHtmlEntities(text));
        if (slug.isEmpty())
            slug = QStringLiteral("section");
        const int seen = used.value(slug, 0);
        used.insert(slug, seen + 1);
        if (seen > 0)
            slug += u'-' + QString::number(seen);

        out += QStringLiteral("<h%1><a name=\"%2\"></a>%3</h%1>")
                   .arg(match.capturedView(1), escapeHtmlAttribute(slug), match.capturedView(2));
    }
    out += QStringView(html).sliced(last);
    return out;
}

} // namespace mdbr::render
