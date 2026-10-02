#ifndef MARKDOWNDOCUMENT_H
#define MARKDOWNDOCUMENT_H

#include <QString>

struct MarkdownDocument
{
    QString filePath;
    QString fileName;
    QString rawText;          // decoded text, line endings normalized to LF
    QString encodingName;     // detected source encoding, e.g. "UTF-8", "Windows-1256"
    bool encodingGuessed = false; // true when the encoding came from a heuristic
};

#endif
