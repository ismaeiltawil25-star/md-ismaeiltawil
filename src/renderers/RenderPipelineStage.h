#ifndef RENDERPIPELINESTAGE_H
#define RENDERPIPELINESTAGE_H

#include <QString>

enum class RenderStage
{
    Md4c,
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
