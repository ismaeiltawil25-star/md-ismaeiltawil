#include "MarkdownHtmlRenderer.h"
#include "HtmlElementWrapper.h"
#include "HtmlNormalizer.h"
#include "HtmlRtlProcessor.h"
#include "TaskListNormalizer.h"
#include "utils/ErrorLogger.h"
#include "utils/PerformanceMonitor.h"

#include <md4c-html.h>

#include <QElapsedTimer>

static void append_html_chunk(const MD_CHAR *data, MD_SIZE size, void *userData)
{
    auto *out = static_cast<QString *>(userData);
    out->append(QString::fromUtf8(data, static_cast<qsizetype>(size)));
}

MarkdownHtmlRenderer::Result MarkdownHtmlRenderer::render(const QString &markdown)
{
    PerformanceMonitor::Timer timer("MarkdownHtmlRenderer::render");
    Result result;

    const QByteArray utf8 = markdown.toUtf8();

    unsigned mdFlags = MD_FLAG_COLLAPSEWHITESPACE | MD_FLAG_TABLES | MD_FLAG_TASKLISTS;
    unsigned htmlFlags = MD_HTML_FLAG_SKIP_UTF8_BOM;

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

    stageTimer.start();
    QString blockquoteWrapped = HtmlElementWrapper::wrapBlockquotes(html);
    result.diagnostics.append({RenderStage::BlockquoteWrap, true, {}, stageTimer.elapsed()});

    stageTimer.start();
    QString codeBlockWrapped = HtmlElementWrapper::wrapCodeBlocks(blockquoteWrapped);
    result.diagnostics.append({RenderStage::CodeBlockWrap, true, {}, stageTimer.elapsed()});

    stageTimer.start();
    QString rawNormalized = HtmlNormalizer::normalizeRawTags(codeBlockWrapped);
    result.diagnostics.append({RenderStage::HtmlNormalize, true, {}, stageTimer.elapsed()});

    stageTimer.start();
    QString rtlApplied = HtmlRtlProcessor::applyRtl(rawNormalized);
    result.diagnostics.append({RenderStage::RtlProcessing, true, {}, stageTimer.elapsed()});

    stageTimer.start();
    QString rtlBlockquoteFixed = HtmlElementWrapper::swapBlockquoteOrderForRtl(rtlApplied);
    result.diagnostics.append({RenderStage::RtlBlockquoteOrder, true, {}, stageTimer.elapsed()});

    stageTimer.start();
    result.html = TaskListNormalizer::normalizeTaskLists(rtlBlockquoteFixed);
    result.diagnostics.append({RenderStage::TaskListNormalize, true, {}, stageTimer.elapsed()});
    return result;
}

MarkdownHtmlRenderer::Result MarkdownHtmlRenderer::render(const MarkdownDocument &doc)
{
    return render(doc.rawText);
}
