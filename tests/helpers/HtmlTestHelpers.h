#pragma once

// Helpers for asserting on rendered HTML.
//
// Prefer checking what the reader SEES (visibleText) over raw HTML substrings:
// stages such as syntax highlighting legitimately wrap words in <span>s, which
// breaks substring checks without changing the text the user reads.

#include <QRegularExpression>
#include <QString>

namespace mdbr::test {

// HTML with all tags removed (entities are left as written).
inline QString visibleText(const QString &html)
{
    static const QRegularExpression tags(QStringLiteral("<[^>]*>"));
    QString text = html;
    text.remove(tags);
    return text;
}

} // namespace mdbr::test
