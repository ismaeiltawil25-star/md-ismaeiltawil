#include "DocumentOpenPipeline.h"
#include "services/DocumentLoader.h"
#include "services/MarkdownPreprocessor.h"
#include "renderers/MarkdownHtmlRenderer.h"
#include <QFileInfo>
#include <QUrl>

DocumentOpenPipeline::DocumentOpenPipeline(RenderFn renderFn)
    : m_renderFn(renderFn ? renderFn : [](const QString &md) -> PipelineRenderResult {
        auto r = MarkdownHtmlRenderer::render(md);
        return { r.html, r.errorMessage, r.diagnostics };
    })
{
}

DocumentOpenResult DocumentOpenPipeline::open(const QString &filePath)
{
    DocumentOpenResult result;

    auto loadResult = DocumentLoader::load(filePath);
    if (!loadResult.isValid()) {
        result.errorMessage = loadResult.errorMessage;
        return result;
    }

    QString cleaned = MarkdownPreprocessor::stripYamlFrontMatter(loadResult.document.rawText);
    QString withDefinitions = MarkdownPreprocessor::convertDefinitionLists(cleaned);
    auto renderResult = m_renderFn(withDefinitions);

    QFileInfo fileInfo(filePath);
    result.tabState.filePath = fileInfo.absoluteFilePath();
    result.fileName = fileInfo.fileName();
    result.baseUrl = QUrl::fromLocalFile(fileInfo.absolutePath() + '/');
    if (loadResult.document.encodingGuessed)
        result.encodingNotice = QStringLiteral("encoding guessed: %1").arg(loadResult.document.encodingName);

    if (renderResult.isValid()) {
        result.tabState.contentMode = ContentMode::Html;
        result.tabState.originalHtml = renderResult.html;
        result.diagnostics = renderResult.diagnostics;
    } else {
        result.tabState.contentMode = ContentMode::Plain;
        result.tabState.originalPlainText = cleaned;
        result.renderWarning = renderResult.errorMessage;
        result.diagnostics = renderResult.diagnostics;
    }

    return result;
}
