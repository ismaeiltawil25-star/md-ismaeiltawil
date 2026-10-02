#pragma once

// MarkdownExtensions — the switchboard for syntax beyond CommonMark + GFM.
//
// Every extension can be turned off independently (for a future settings
// page, for strict-CommonMark rendering, or to isolate a bug). Defaults enable
// everything. The renderer and preprocessor read these flags; no extension
// checks global state.

namespace mdbr {

struct MarkdownExtensions
{
    bool footnotes = true;          // [^1]                   services/markdown/FootnoteProcessor
    bool alerts = true;             // > [!NOTE]              renderers/extensions/AlertRenderer
    bool math = true;               // $x$, $$x$$             md4c + LatexMathConverter
    bool wikiLinks = true;          // [[Page]]               md4c + LinkAndMathRenderers
    bool headingAnchors = true;     // #section links         renderers/extensions/HeadingAnchors
    bool highlight = true;          // ==text==               renderers/extensions/InlineExtensions
    bool superscript = true;        // ^text^
    bool emojiShortcodes = true;    // :smile:
    bool syntaxHighlighting = true; // ```cpp                 renderers/extensions/SyntaxHighlighter

    [[nodiscard]] static MarkdownExtensions none()
    {
        return {false, false, false, false, false, false, false, false, false};
    }
};

} // namespace mdbr
