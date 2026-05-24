#ifndef MARKDOWNHTMLRENDERER_H
#define MARKDOWNHTMLRENDERER_H

#include <QString>
#include "models/MarkdownDocument.h"

class MarkdownHtmlRenderer
{
public:
    struct Result
    {
        QString html;
        QString errorMessage;

        bool isValid() const { return errorMessage.isEmpty(); }
    };

    static Result render(const QString &markdown);
    static Result render(const MarkdownDocument &doc);
};

#endif
