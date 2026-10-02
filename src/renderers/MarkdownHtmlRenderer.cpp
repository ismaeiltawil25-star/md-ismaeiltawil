#include "MarkdownHtmlRenderer.h"
#include "HtmlElementWrapper.h"
#include "HtmlNormalizer.h"
#include "HtmlRtlProcessor.h"
#include "TaskListNormalizer.h"
#include "extensions/AlertRenderer.h"
#include "extensions/HeadingAnchors.h"
#include "extensions/InlineExtensions.h"
#include "extensions/LinkAndMathRenderers.h"
#include "extensions/SyntaxHighlighter.h"
#include "utils/ErrorLogger.h"
#include "utils/PerformanceMonitor.h"

#include <md4c-html.h>

#include <QElapsedTimer>

static void append_html_chunk(const MD_CHAR *data, MD_SIZE size, void *userData)
{
    auto *out = static_cast<QString *>(userData);
    out->append(QString::fromUtf8(data, static_cast<qsizetype>(size)));
}

MarkdownHtmlRenderer::Result MarkdownHtmlRenderer::render(const QString &markdown,
                                                          const mdbr::MarkdownExtensions &extensions)
{
    PerformanceMonitor::Timer timer("MarkdownHtmlRenderer::render");
    Result result;

    const QByteArray utf8 = markdown.toUtf8();

    // GitHub-flavored Markdown: tables, task lists, ~~strikethrough~~ and bare
    // URL / www. / e-mail autolinks. Math and wiki links are parsed by md4c and
    // translated by the extension stages below.
    unsigned mdFlags = MD_DIALECT_GITHUB | MD_FLAG_COLLAPSEWHITESPACE;
    if (extensions.math)
        mdFlags |= MD_FLAG_LATEXMATHSPANS;
    if (extensions.wikiLinks)
        mdFlags |= MD_FLAG_WIKILINKS;
    const unsigned htmlFlags = MD_HTML_FLAG_SKIP_UTF8_BOM;

    QElapsedTimer stageTimer;
    stageTimer.start();

    QString html;
    int ret = md_html(utf8.constData(), utf8.size(),
                      append_html_chunk, &html,
                      mdFlags, htmlFlags);

    if (ret != 0) {
        QString errMsg = QString("md4c HTML rendering failed with code %1").arg(ret);
        ErrorLogger::instance().logError("MarkdownHtmlRenderer", errMsg);
        result.errorMessage = errMsg;
        result.diagnostics.append({RenderStage::Md4c, false, errMsg, stageTimer.elapsed()});
        return result;
    }
    result.diagnostics.append({RenderStage::Md4c, true, {}, stageTimer.elapsed()});

    // Runs one stage (when enabled) and records its timing.
    const auto runStage = [&](RenderStage stage, bool enabled, auto &&transform) {
        if (!enabled)
            return;
        stageTimer.start();
        html = transform(html);
        result.diagnostics.append({stage, true, {}, stageTimer.elapsed()});
    };

    using namespace mdbr::render;
    const InlineExtensionOptions inlineOptions{extensions.highlight, extensions.superscript,
                                               extensions.emojiShortcodes};
    const bool anyInline = extensions.highlight || extensions.superscript || extensions.emojiShortcodes;

    // Extensions first, on md4c's clean output (before tables are introduced).
    runStage(RenderStage::Alerts, extensions.alerts, renderAlerts);
    runStage(RenderStage::Math, extensions.math, renderMath);
    runStage(RenderStage::WikiLinks, extensions.wikiLinks, renderWikiLinks);
    runStage(RenderStage::HeadingAnchors, extensions.headingAnchors, addHeadingAnchors);
    runStage(RenderStage::InlineExtensions, anyInline,
             [&](const QString &h) { return applyInlineExtensions(h, inlineOptions); });
    runStage(RenderStage::SyntaxHighlight, extensions.syntaxHighlighting, highlightCodeBlocks);

    // Presentation stages: adapt HTML to what QTextDocument can display.
    runStage(RenderStage::BlockquoteWrap, true, HtmlElementWrapper::wrapBlockquotes);
    runStage(RenderStage::CodeBlockWrap, true, HtmlElementWrapper::wrapCodeBlocks);
    runStage(RenderStage::HtmlNormalize, true, HtmlNormalizer::normalizeRawTags);
    runStage(RenderStage::RtlProcessing, true, HtmlRtlProcessor::applyRtl);
    runStage(RenderStage::RtlBlockquoteOrder, true, HtmlElementWrapper::swapBlockquoteOrderForRtl);
    runStage(RenderStage::TaskListNormalize, true, TaskListNormalizer::normalizeTaskLists);

    result.html = std::move(html);
    return result;
}

MarkdownHtmlRenderer::Result MarkdownHtmlRenderer::render(const MarkdownDocument &doc)
{
    return render(doc.rawText);
}
