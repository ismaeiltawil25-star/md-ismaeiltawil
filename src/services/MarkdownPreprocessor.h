#ifndef MARKDOWNPREPROCESSOR_H
#define MARKDOWNPREPROCESSOR_H

#include <QString>

class MarkdownPreprocessor
{
public:
    static QString stripYamlFrontMatter(const QString &markdown);
    static QString convertDefinitionLists(const QString &markdown);
};

#endif
