#ifndef HTMLNORMALIZER_H
#define HTMLNORMALIZER_H

#include <QString>

class HtmlNormalizer
{
public:
    static QString normalizeRawTags(const QString &html);
};

#endif
