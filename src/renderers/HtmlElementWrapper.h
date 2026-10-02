#ifndef HTMLELEMENTWRAPPER_H
#define HTMLELEMENTWRAPPER_H

#include <QString>

class HtmlElementWrapper
{
public:
    // Quote levels beyond this stay plain <blockquote> (see wrapBlockquotes).
    static constexpr int kMaxBlockquoteTableDepth = 5;

    static QString wrapCodeBlocks(const QString &html);
    static QString wrapBlockquotes(const QString &html);
    static QString swapBlockquoteOrderForRtl(const QString &html);
};

#endif
