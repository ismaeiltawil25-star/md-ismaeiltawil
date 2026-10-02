#pragma once

// InlineCodeSpans — find `code spans` in one line of Markdown (CommonMark
// §6.1: a backtick run is closed by the next run of the SAME length).
//
// Text-level preprocessors use replaceOutsideCodeSpans() so that syntax inside
// `inline code` is never rewritten. Limitation: a code span that continues on
// the next line is not detected (rare in practice).

#include <QList>
#include <QRegularExpression>
#include <QString>
#include <QStringView>

#include <functional>
#include <utility>

namespace mdbr::markdown {

// [start, end) ranges of code spans in `line`, including the backticks.
[[nodiscard]] QList<std::pair<qsizetype, qsizetype>> codeSpanRanges(QStringView line);

// Applies `pattern` to the parts of `line` outside code spans; each match is
// replaced by `replace(match)`.
[[nodiscard]] QString replaceOutsideCodeSpans(
    const QString &line, const QRegularExpression &pattern,
    const std::function<QString(const QRegularExpressionMatch &)> &replace);

} // namespace mdbr::markdown
