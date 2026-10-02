#include "LinkAndMathRenderers.h"

#include "HeadingAnchors.h"
#include "HtmlEntities.h"
#include "LatexMathConverter.h"

#include <QFileInfo>
#include <QRegularExpression>

namespace mdbr::render {

namespace {

// Text fonts, not "Cambria Math"/"STIX Two Math": math fonts have very tall
// line metrics, which make QTextDocument push <sup>/<sub> far off the baseline.
constexpr auto kMathFont =
    "font-family: 'Cambria', 'Times New Roman', serif; font-style: italic;";

[[nodiscard]] QString wikiHref(const QString &target)
{
    const qsizetype hash = target.indexOf(u'#');
    QString page = hash < 0 ? target : target.left(hash);
    const QString fragment = hash < 0 ? QString() : target.mid(hash + 1);

    page = page.trimmed();
    if (!page.isEmpty() && QFileInfo(page).suffix().isEmpty())
        page += QStringLiteral(".md"); // [[Page]] means Page.md next to this file

    return fragment.isEmpty() ? page : page + u'#' + slugify(fragment);
}

} // namespace

QString renderWikiLinks(const QString &html)
{
    if (!html.contains(u"<x-wikilink"))
        return html;

    static const QRegularExpression wiki(QStringLiteral(R"re(<x-wikilink data-target="([^"]*)">(.*?)</x-wikilink>)re"),
                                         QRegularExpression::DotMatchesEverythingOption);
    QString out;
    qsizetype last = 0;
    auto it = wiki.globalMatch(html);
    while (it.hasNext()) {
        const auto m = it.next();
        out += QStringView(html).sliced(last, m.capturedStart() - last);
        last = m.capturedEnd();
        const QString target = decodeHtmlEntities(m.capturedView(1));
        out += QStringLiteral("<a href=\"%1\">%2</a>").arg(escapeHtmlAttribute(wikiHref(target)), m.captured(2));
    }
    out += QStringView(html).sliced(last);
    return out;
}

QString renderMath(const QString &html)
{
    if (!html.contains(u"<x-equation"))
        return html;

    // Content is escaped text: it never contains '<', which keeps matching linear.
    static const QRegularExpression displayParagraph(
        QStringLiteral(R"(<p>\s*<x-equation type="display">([^<]*)</x-equation>\s*</p>)"));
    static const QRegularExpression equation(
        QStringLiteral(R"(<x-equation( type="display")?>([^<]*)</x-equation>)"));

    QString out = html;
    out.replace(displayParagraph, QStringLiteral("<p align=\"center\" class=\"math-display\">"
                                                 "<x-equation type=\"display\">\\1</x-equation></p>"));

    QString result;
    qsizetype last = 0;
    auto it = equation.globalMatch(out);
    while (it.hasNext()) {
        const auto m = it.next();
        result += QStringView(out).sliced(last, m.capturedStart() - last);
        last = m.capturedEnd();
        const bool display = !m.capturedView(1).isEmpty();
        const QString body = latexToHtml(decodeHtmlEntities(m.capturedView(2)));
        result += QStringLiteral("<span style=\"%1%2\">%3</span>")
                      .arg(QString::fromLatin1(kMathFont),
                           display ? QStringLiteral(" font-size: 1.4em;") : QString(), body);
    }
    result += QStringView(out).sliced(last);
    return result;
}

} // namespace mdbr::render
