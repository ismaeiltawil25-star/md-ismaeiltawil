#pragma once

// Helpers that run Markdown through the same stages as DocumentOpenPipeline
// (minus file I/O) and load the result into a QTextDocument — the component
// the app actually displays. Asserting on the QTextDocument catches HTML that
// md4c produces but Qt's rich-text engine silently ignores.

#include <QTextDocument>
#include <memory>

#include "renderers/MarkdownHtmlRenderer.h"
#include "services/MarkdownPreprocessor.h"

namespace mdbr::test {

struct RenderedDocument
{
    MarkdownHtmlRenderer::Result renderResult;
    std::unique_ptr<QTextDocument> document = std::make_unique<QTextDocument>();
};

inline QString preprocess(const QString &markdown)
{
    return MarkdownPreprocessor::convertDefinitionLists(
        MarkdownPreprocessor::stripYamlFrontMatter(markdown));
}

inline RenderedDocument renderToDocument(const QString &markdown)
{
    RenderedDocument out;
    out.renderResult = MarkdownHtmlRenderer::render(preprocess(markdown));
    out.document->setHtml(out.renderResult.html);
    return out;
}

} // namespace mdbr::test
