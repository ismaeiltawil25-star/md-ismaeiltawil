#include "AlertRenderer.h"

#include <QRegularExpression>

namespace mdbr::render {

namespace {

struct AlertStyle
{
    const char16_t *title;
    const char16_t *icon;
    const char *color; // readable on both light and dark pages (GitHub palette)
};

[[nodiscard]] AlertStyle styleFor(const QString &type)
{
    if (type == QLatin1StringView("TIP"))     return {u"Tip", u"\U0001F4A1", "#3fb950"};
    if (type == QLatin1StringView("IMPORTANT")) return {u"Important", u"\u2757", "#ab7df8"};
    if (type == QLatin1StringView("WARNING")) return {u"Warning", u"\u26A0\uFE0F", "#d29922"};
    if (type == QLatin1StringView("CAUTION")) return {u"Caution", u"\u26D4", "#f85149"};
    return {u"Note", u"\u2139\uFE0F", "#4493f8"};
}

} // namespace

QString renderAlerts(const QString &html)
{
    if (!html.contains(u"[!"))
        return html;

    // md4c output: <blockquote>\n<p>[!NOTE]\nBody…</p>
    static const QRegularExpression marker(
        QStringLiteral(R"(<blockquote>\s*<p>\[!(NOTE|TIP|IMPORTANT|WARNING|CAUTION)\][ \t]*\n?)"),
        QRegularExpression::CaseInsensitiveOption);

    QString out;
    qsizetype last = 0;
    auto it = marker.globalMatch(html);
    while (it.hasNext()) {
        const auto m = it.next();
        out += QStringView(html).sliced(last, m.capturedStart() - last);
        last = m.capturedEnd();

        const AlertStyle style = styleFor(m.captured(1).toUpper());
        out += QStringLiteral("<blockquote data-alert-color=\"%1\">\n"
                              "<p><b style=\"color: %1;\">%2 %3</b></p>\n<p>")
                   .arg(QString::fromLatin1(style.color),
                        QString::fromUtf16(style.icon), QString::fromUtf16(style.title));
    }
    out += QStringView(html).sliced(last);
    return out;
}

} // namespace mdbr::render
