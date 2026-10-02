#include "InlineCodeSpans.h"

namespace mdbr::markdown {

namespace {

[[nodiscard]] qsizetype runLength(QStringView line, qsizetype at)
{
    qsizetype end = at;
    while (end < line.size() && line[end] == u'`')
        ++end;
    return end - at;
}

} // namespace

QList<std::pair<qsizetype, qsizetype>> codeSpanRanges(QStringView line)
{
    QList<std::pair<qsizetype, qsizetype>> ranges;
    qsizetype i = 0;
    while (i < line.size()) {
        if (line[i] == u'\\' && i + 1 < line.size()) {
            i += 2; // escaped character (an escaped backtick does not open a span)
            continue;
        }
        if (line[i] != u'`') {
            ++i;
            continue;
        }
        const qsizetype openLength = runLength(line, i);
        qsizetype search = i + openLength;
        qsizetype close = -1;
        while (search < line.size()) {
            const qsizetype next = line.indexOf(u'`', search);
            if (next < 0)
                break;
            const qsizetype length = runLength(line, next);
            if (length == openLength) {
                close = next;
                break;
            }
            search = next + length;
        }
        if (close < 0) {
            i += openLength; // unmatched run: literal backticks
            continue;
        }
        ranges.append({i, close + openLength});
        i = close + openLength;
    }
    return ranges;
}

QString replaceOutsideCodeSpans(const QString &line, const QRegularExpression &pattern,
                                const std::function<QString(const QRegularExpressionMatch &)> &replace)
{
    const auto spans = codeSpanRanges(line);
    QString out;
    qsizetype pos = 0;

    const auto processSegment = [&](qsizetype from, qsizetype to) {
        const QString segment = line.mid(from, to - from);
        qsizetype last = 0;
        auto it = pattern.globalMatch(segment);
        while (it.hasNext()) {
            const auto m = it.next();
            out += QStringView(segment).sliced(last, m.capturedStart() - last);
            out += replace(m);
            last = m.capturedEnd();
        }
        out += QStringView(segment).sliced(last);
    };

    for (const auto &[start, end] : spans) {
        processSegment(pos, start);
        out += QStringView(line).sliced(start, end - start); // code span verbatim
        pos = end;
    }
    processSegment(pos, line.size());
    return out;
}

} // namespace mdbr::markdown
