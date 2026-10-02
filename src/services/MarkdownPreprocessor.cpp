#include "MarkdownPreprocessor.h"

#include "services/markdown/DefinitionListConverter.h"
#include "services/markdown/FootnoteProcessor.h"
#include "services/markdown/FrontMatterStripper.h"

QString MarkdownPreprocessor::convertDefinitionLists(const QString &markdown)
{
    return mdbr::markdown::convertDefinitionLists(markdown);
}

QString MarkdownPreprocessor::stripYamlFrontMatter(const QString &markdown)
{
    return mdbr::markdown::stripFrontMatter(markdown);
}

QString MarkdownPreprocessor::processFootnotes(const QString &markdown)
{
    return mdbr::markdown::processFootnotes(markdown);
}

QString MarkdownPreprocessor::preprocess(const QString &markdown, const mdbr::MarkdownExtensions &extensions)
{
    QString text = stripYamlFrontMatter(markdown);
    if (extensions.footnotes)
        text = processFootnotes(text);
    return convertDefinitionLists(text);
}
