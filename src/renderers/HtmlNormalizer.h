#ifndef HTMLNORMALIZER_H
#define HTMLNORMALIZER_H

#include <QString>

class HtmlNormalizer
{
public:
    // Nested <div> levels beyond this stay plain divs (QTextDocument layout cost).
    static constexpr int kMaxDivTableDepth = 3;

    static QString normalizeRawTags(const QString &html);
};

#endif
