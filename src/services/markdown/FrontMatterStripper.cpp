#include "FrontMatterStripper.h"

#include <QRegularExpression>
#include <QStringList>

namespace mdbr::markdown {

namespace {

[[nodiscard]] bool isOpeningDelimiter(QStringView line)
{
    return line.trimmed() == u"---" && line.startsWith(u"---");
}

[[nodiscard]] bool isClosingDelimiter(QStringView line)
{
    const QStringView trimmed = line.trimmed();
    return (trimmed == u"---" || trimmed == u"...") && !line.startsWith(u' ');
}

[[nodiscard]] bool isYamlKeyLine(const QString &line)
{
    // `key: value` or `key:` — plain (Unicode word) or quoted key, no spaces in plain keys.
    static const QRegularExpression keyLine(
        QStringLiteral(R"(^(?:[\w][\w.\-]*|"[^"]+"|'[^']+')\s*:(?:\s|$))"),
        QRegularExpression::UseUnicodePropertiesOption);
    return keyLine.match(line).hasMatch();
}

[[nodiscard]] bool looksLikeYaml(const QStringList &blockLines)
{
    bool sawKey = false;
    for (const QString &line : blockLines) {
        if (line.trimmed().isEmpty())
            continue;
        if (line.startsWith(u' ') || line.startsWith(u'\t'))
            continue; // continuation of a multi-line value or nested mapping
        if (line.startsWith(u'#'))
            continue; // YAML comment
        if (line == u"-" || line.startsWith(u"- "))
            continue; // sequence item
        if (isYamlKeyLine(line)) {
            sawKey = true;
            continue;
        }
        return false; // prose: this is not front matter
    }
    return sawKey;
}

} // namespace

QString stripFrontMatter(const QString &markdown)
{
    if (!markdown.startsWith(u"---"))
        return markdown;

    QString normalized = markdown;
    normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    const QStringList lines = normalized.split(u'\n');

    if (!isOpeningDelimiter(lines.first()))
        return markdown;

    qsizetype closingLine = -1;
    for (qsizetype i = 1; i < lines.size(); ++i) {
        if (isClosingDelimiter(lines[i])) {
            closingLine = i;
            break;
        }
    }
    if (closingLine < 0)
        return markdown;

    if (!looksLikeYaml(lines.mid(1, closingLine - 1)))
        return markdown;

    qsizetype bodyStart = closingLine + 1;
    while (bodyStart < lines.size() && lines[bodyStart].isEmpty())
        ++bodyStart;

    return lines.mid(bodyStart).join(u'\n');
}

} // namespace mdbr::markdown
