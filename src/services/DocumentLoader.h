#ifndef DOCUMENTLOADER_H
#define DOCUMENTLOADER_H

#include <QString>
#include "models/MarkdownDocument.h"

class DocumentLoader
{
public:
    static constexpr qint64 MaxFileSize = 10 * 1024 * 1024;

    struct Result
    {
        MarkdownDocument document;
        QString errorMessage;

        bool isValid() const { return errorMessage.isEmpty(); }
    };

    static Result load(const QString &filePath);
};

#endif
