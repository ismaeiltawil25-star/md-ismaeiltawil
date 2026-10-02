#ifndef MARKDOWNPREPROCESSOR_H
#define MARKDOWNPREPROCESSOR_H

#include <QString>

#include "renderers/MarkdownExtensions.h"

// Facade over the text-level Markdown preprocessing stages, kept for existing
// callers. Each stage lives in its own unit under services/markdown/:
//   - FrontMatterStripper     (YAML front matter)
//   - FootnoteProcessor       ([^1] references and definitions)
//   - DefinitionListConverter (Term / ": description" lists)
// All rely on FenceTracker so fenced code is never rewritten.
//
// preprocess() is the single entry point that runs every text stage in order;
// the open pipeline and the tests both use it, so the order is defined once.
class MarkdownPreprocessor
{
public:
    static QString stripYamlFrontMatter(const QString &markdown);
    static QString convertDefinitionLists(const QString &markdown);
    static QString processFootnotes(const QString &markdown);

    // Front matter -> footnotes -> definition lists.
    static QString preprocess(const QString &markdown,
                              const mdbr::MarkdownExtensions &extensions = {});
};

#endif
