#ifndef HTMLELEMENTWRAPPER_H
#define HTMLELEMENTWRAPPER_H

#include <QString>

class HtmlElementWrapper
{
public:
    static QString wrapCodeBlocks(const QString &html);
    static QString wrapBlockquotes(const QString &html);
    static QString swapBlockquoteOrderForRtl(const QString &html);
};

#endif
