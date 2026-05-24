#include "MarkdownPreprocessor.h"

#include <QRegularExpression>

QString MarkdownPreprocessor::convertDefinitionLists(const QString &markdown)
{
    if (markdown.isEmpty())
        return markdown;

    QStringList lines = markdown.split(QChar('\n'));
    QString result;
    bool inFence = false;
    int i = 0;

    while (i < lines.size()) {
        if (lines[i].startsWith(QLatin1String("```"))) {
            inFence = !inFence;
            result += lines[i] + QChar('\n');
            ++i;
            continue;
        }

        if (inFence) {
            result += lines[i] + QChar('\n');
            ++i;
            continue;
        }

        // Detect definition pattern: non-empty line immediately followed by ": " line
        if (i + 1 < lines.size()
            && !lines[i].isEmpty()
            && !lines[i].startsWith(QLatin1Char(' '))
            && !lines[i].startsWith(QLatin1Char('\t'))
            && !lines[i].startsWith(QLatin1Char('>'))
            && !lines[i].startsWith(QLatin1Char('|'))
            && !lines[i].startsWith(QLatin1Char('-'))
            && !lines[i].startsWith(QLatin1Char('*'))
            && lines[i + 1].startsWith(QLatin1String(": ")))
        {
            result += QStringLiteral("<table class=\"definition-table\" width=\"100%\""
                                     " cellspacing=\"0\" cellpadding=\"0\">\n");

            while (i < lines.size()
                   && !lines[i].isEmpty()
                   && i + 1 < lines.size()
                   && lines[i + 1].startsWith(QLatin1String(": ")))
            {
                QString term = lines[i].toHtmlEscaped();
                QString desc = lines[i + 1].mid(2).toHtmlEscaped();
                // Manually convert inline code inside description
                desc.replace(QRegularExpression(QStringLiteral("`([^`]+)`")),
                             QStringLiteral("<code>\\1</code>"));

                result += QStringLiteral("<tr><td class=\"definition-term\">");
                result += term;
                result += QStringLiteral("</td><td class=\"definition-description\">");
                result += desc;
                result += QStringLiteral("</td></tr>\n");

                i += 2;

                // Skip blank lines between entries
                while (i < lines.size() && lines[i].isEmpty())
                    ++i;
            }

            result += QStringLiteral("</table>\n");
            continue;
        }

        result += lines[i] + QChar('\n');
        ++i;
    }

    return result;
}

QString MarkdownPreprocessor::stripYamlFrontMatter(const QString &markdown)
{
    if (markdown.isEmpty())
        return markdown;

    QString normalized = markdown;
    normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));

    if (!normalized.startsWith(QLatin1String("---")))
        return markdown;

    if (normalized.length() > 3 && normalized[3] != QLatin1Char('\n'))
        return markdown;

    const QString needle = QStringLiteral("\n---");
    int searchStart = 3;

    while (true) {
        int idx = normalized.indexOf(needle, searchStart);
        if (idx == -1)
            return markdown;

        int after = idx + 4;
        if (after >= normalized.length() || normalized[after] == QLatin1Char('\n')) {
            int afterClose = after;
            while (afterClose < normalized.length() && normalized[afterClose] == QLatin1Char('\n'))
                ++afterClose;
            return normalized.mid(afterClose);
        }

        searchStart = idx + 1;
    }
}
