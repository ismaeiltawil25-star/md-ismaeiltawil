#ifndef RENDERPIPELINESTAGE_H
#define RENDERPIPELINESTAGE_H

#include <QString>

// Every HTML stage of MarkdownHtmlRenderer::render(), in execution order.
// Each stage reports a StageDiagnostic (name + timing), so a slow or failing
// stage can be identified from the diagnostics alone.
enum class RenderStage
{
    Md4c,
    Alerts,            // > [!NOTE] callouts           (extensions/AlertRenderer)
    Math,              // $..$ / $$..$$                (extensions/LinkAndMathRenderers)
    WikiLinks,         // [[Page]]                     (extensions/LinkAndMathRenderers)
    HeadingAnchors,    // <a name> for #section links  (extensions/HeadingAnchors)
    InlineExtensions,  // ==mark== ^sup^ :emoji:       (extensions/InlineExtensions)
    SyntaxHighlight,   // ```lang code colouring       (extensions/SyntaxHighlighter)
    BlockquoteWrap,
    CodeBlockWrap,
    HtmlNormalize,
    RtlProcessing,
    RtlBlockquoteOrder,
    TaskListNormalize
};

inline const char *renderStageName(RenderStage stage)
{
    switch (stage) {
    case RenderStage::Md4c:               return "md4c";
    case RenderStage::Alerts:             return "alerts";
    case RenderStage::Math:               return "math";
    case RenderStage::WikiLinks:          return "wiki-links";
    case RenderStage::HeadingAnchors:     return "heading-anchors";
    case RenderStage::InlineExtensions:   return "inline-extensions";
    case RenderStage::SyntaxHighlight:    return "syntax-highlight";
    case RenderStage::BlockquoteWrap:     return "blockquote-wrap";
    case RenderStage::CodeBlockWrap:      return "code-block-wrap";
    case RenderStage::HtmlNormalize:      return "html-normalize";
    case RenderStage::RtlProcessing:      return "rtl-processing";
    case RenderStage::RtlBlockquoteOrder: return "rtl-blockquote-order";
    case RenderStage::TaskListNormalize:  return "task-list-normalize";
    }
    return "unknown";
}

#endif
