#include "MarkdownPreprocessor.h"

#include "services/markdown/DefinitionListConverter.h"
#include "services/markdown/FrontMatterStripper.h"

QString MarkdownPreprocessor::convertDefinitionLists(const QString &markdown)
{
    return mdbr::markdown::convertDefinitionLists(markdown);
}

QString MarkdownPreprocessor::stripYamlFrontMatter(const QString &markdown)
{
    return mdbr::markdown::stripFrontMatter(markdown);
}
