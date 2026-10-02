#ifndef MARKDOWNPREPROCESSOR_H
#define MARKDOWNPREPROCESSOR_H

#include <QString>

// Facade over the text-level Markdown preprocessing stages, kept for existing
// callers. Each stage lives in its own unit under services/markdown/:
//   - FrontMatterStripper     (YAML front matter)
//   - DefinitionListConverter (Term / ": description" lists)
// Both rely on FenceTracker so fenced code is never rewritten.
class MarkdownPreprocessor
{
public:
    static QString stripYamlFrontMatter(const QString &markdown);
    static QString convertDefinitionLists(const QString &markdown);
};

#endif
