#ifndef MARKDOWNDOCUMENT_H
#define MARKDOWNDOCUMENT_H

#include <QString>

struct MarkdownDocument
{
    QString filePath;
    QString fileName;
    QString rawText;
};

#endif
