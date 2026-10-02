#include "DefinitionListConverter.h"
#include "FenceTracker.h"

#include <QRegularExpression>
#include <QStringList>

namespace mdbr::markdown {

namespace {

constexpr QStringView kDescriptionPrefix = u": ";

[[nodiscard]] bool isDescriptionLine(const QString &line)
{
    return line.startsWith(kDescriptionPrefix);
}

// A term must be a plain text line, not the start of another Markdown block.
[[nodiscard]] bool isTermCandidate(const QString &line)
{
    static const QRegularExpression orderedListMarker(QStringLiteral(R"(^\d{1,9}[.)](?:\s|$))"));

    if (line.trimmed().isEmpty())
        return false;

    constexpr QStringView kBlockStarters = u" \t>|-*+#<`~:";
    if (kBlockStarters.contains(line.front()))
        return false;

    return !orderedListMarker.match(line).hasMatch();
}

[[nodiscard]] QString renderDescription(const QString &rawDescription)
{
    static const QRegularExpression inlineCode(QStringLiteral("`([^`]+)`"));
    QString html = rawDescription.toHtmlEscaped();
    html.replace(inlineCode, QStringLiteral("<code>\\1</code>"));
    return html;
}

void appendRow(QString &out, const QString &term, const QString &descriptionLine)
{
    out += QStringLiteral("<tr><td class=\"definition-term\">");
    out += term.toHtmlEscaped();
    out += QStringLiteral("</td><td class=\"definition-description\">");
    out += renderDescription(descriptionLine.mid(kDescriptionPrefix.size()));
    out += QStringLiteral("</td></tr>\n");
}

} // namespace

QString convertDefinitionLists(const QString &markdown)
{
    if (markdown.isEmpty())
        return markdown;

    const QStringList lines = markdown.split(u'\n');
    const auto startsDefinition = [&lines](qsizetype at) {
        return at + 1 < lines.size()
            && isTermCandidate(lines[at])
            && isDescriptionLine(lines[at + 1]);
    };

    QString result;
    result.reserve(markdown.size() + markdown.size() / 8);
    FenceTracker fence;
    qsizetype i = 0;

    while (i < lines.size()) {
        if (fence.consume(lines[i]) || !startsDefinition(i)) {
            result += lines[i];
            result += u'\n';
            ++i;
            continue;
        }

        result += QStringLiteral("<table class=\"definition-table\" width=\"100%\""
                                 " cellspacing=\"0\" cellpadding=\"0\">\n");
        while (startsDefinition(i)) {
            appendRow(result, lines[i], lines[i + 1]);
            i += 2;
            while (i < lines.size() && lines[i].isEmpty())
                ++i; // blank lines between entries belong to the same list
        }
        // The blank line terminates md4c's HTML block (CommonMark HTML block type 6).
        result += QStringLiteral("</table>\n\n");
    }

    return result;
}

} // namespace mdbr::markdown
