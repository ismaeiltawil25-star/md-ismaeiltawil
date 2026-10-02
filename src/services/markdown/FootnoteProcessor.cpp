#include "FootnoteProcessor.h"

#include "FenceTracker.h"
#include "InlineCodeSpans.h"

#include <QHash>
#include <QSet>
#include <QRegularExpression>
#include <QStringList>

namespace mdbr::markdown {

namespace {

struct Definition
{
    QStringList lines; // first line, then continuation lines (indent removed)
    qsizetype firstLine = 0;
    qsizetype lastLine = 0;
};

const QRegularExpression &definitionPattern()
{
    // Up to 3 spaces, [^label]: text   (label: no spaces or ']')
    static const QRegularExpression pattern(QStringLiteral(R"(^ {0,3}\[\^([^\]\s]+)\]:[ \t]?(.*)$)"));
    return pattern;
}

const QRegularExpression &referencePattern()
{
    static const QRegularExpression pattern(QStringLiteral(R"(\[\^([^\]\s]+)\])"));
    return pattern;
}

[[nodiscard]] bool isIndented(const QString &line)
{
    return line.startsWith(QStringLiteral("    ")) || line.startsWith(u'\t');
}

[[nodiscard]] QString stripIndent(const QString &line)
{
    return line.startsWith(u'\t') ? line.mid(1) : line.mid(4);
}

// Finds definitions outside fenced code. A definition continues over indented
// lines, also across single blank lines followed by more indented lines.
QHash<QString, Definition> collectDefinitions(const QStringList &lines)
{
    QHash<QString, Definition> definitions;
    FenceTracker fence;
    for (qsizetype i = 0; i < lines.size(); ++i) {
        if (fence.consume(lines[i]))
            continue;
        const auto match = definitionPattern().match(lines[i]);
        if (!match.hasMatch() || definitions.contains(match.captured(1)))
            continue;

        Definition def;
        def.firstLine = i;
        def.lines << match.captured(2);
        qsizetype j = i + 1;
        while (j < lines.size()) {
            if (isIndented(lines[j])) {
                def.lines << stripIndent(lines[j]);
                ++j;
            } else if (lines[j].trimmed().isEmpty() && j + 1 < lines.size() && isIndented(lines[j + 1])) {
                def.lines << QString();
                ++j;
            } else {
                break;
            }
        }
        def.lastLine = j - 1;
        definitions.insert(match.captured(1), def);
        i = j - 1;
    }
    return definitions;
}

[[nodiscard]] QString referenceHtml(int number, bool firstUse)
{
    const QString anchor = firstUse ? QStringLiteral(" name=\"fnref-%1\"").arg(number) : QString();
    return QStringLiteral("<sup><a href=\"#fn-%1\"%2>%1</a></sup>").arg(number).arg(anchor);
}

} // namespace

QString processFootnotes(const QString &markdown)
{
    if (!markdown.contains(QStringLiteral("[^")))
        return markdown;

    const QStringList lines = markdown.split(u'\n');
    QHash<QString, Definition> definitions = collectDefinitions(lines);
    if (definitions.isEmpty())
        return markdown;

    QList<bool> inDefinition(lines.size(), false);
    for (const Definition &def : std::as_const(definitions))
        for (qsizetype k = def.firstLine; k <= def.lastLine; ++k)
            inDefinition[k] = true;

    // Pass 1: which labels are referenced from the body (outside code)?
    QSet<QString> referenced;
    {
        FenceTracker fence;
        for (qsizetype i = 0; i < lines.size(); ++i) {
            if (fence.consume(lines[i]) || inDefinition[i])
                continue;
            (void)replaceOutsideCodeSpans(lines[i], referencePattern(), [&](const QRegularExpressionMatch &m) {
                referenced.insert(m.captured(1));
                return m.captured(0);
            });
        }
    }

    // Unreferenced definitions stay in the text exactly as written.
    for (auto it = definitions.begin(); it != definitions.end();) {
        if (referenced.contains(it.key())) {
            ++it;
            continue;
        }
        for (qsizetype k = it->firstLine; k <= it->lastLine; ++k)
            inDefinition[k] = false;
        it = definitions.erase(it);
    }
    if (definitions.isEmpty())
        return markdown;

    // Pass 2: replace references, drop definition lines.
    QHash<QString, int> numbers; // label -> number, in order of first use
    QStringList ordered;         // labels by number
    QStringList body;
    FenceTracker fence;
    for (qsizetype i = 0; i < lines.size(); ++i) {
        const QString &line = lines[i];
        if (fence.consume(line)) {
            body << line;
            continue;
        }
        if (inDefinition[i])
            continue;

        body << replaceOutsideCodeSpans(line, referencePattern(), [&](const QRegularExpressionMatch &m) {
            const QString label = m.captured(1);
            if (!definitions.contains(label))
                return m.captured(0); // undefined: keep as written
            const bool firstUse = !numbers.contains(label);
            if (firstUse) {
                numbers.insert(label, int(ordered.size()) + 1);
                ordered << label;
            }
            return referenceHtml(numbers.value(label), firstUse);
        });
    }

    // "***" cannot become a setext heading underline, unlike "---".
    QStringList section{QString(), QString(), QStringLiteral("***"), QString()};
    for (qsizetype n = 0; n < ordered.size(); ++n) {
        const Definition &def = definitions.value(ordered[n]);
        const int number = int(n) + 1;
        QStringList noteLines = def.lines;
        while (noteLines.size() > 1 && noteLines.last().trimmed().isEmpty())
            noteLines.removeLast();
        noteLines.last() += QStringLiteral(" <a href=\"#fnref-%1\">\u21A9</a>").arg(number);

        section << QStringLiteral("%1. <a name=\"fn-%1\"></a>%2").arg(number).arg(noteLines.first());
        for (qsizetype k = 1; k < noteLines.size(); ++k)
            section << (noteLines[k].isEmpty() ? QString() : QStringLiteral("    ") + noteLines[k]);
    }

    return body.join(u'\n') + u'\n' + section.join(u'\n') + u'\n';
}

} // namespace mdbr::markdown
