#pragma once

// InlineExtensions — popular inline syntax that md4c does not know:
//
//   ==highlighted==   → highlighted text           (Pandoc, Obsidian, Typora)
//   ^superscript^     → superscript, no spaces     (Pandoc: mc^2^, 2^10^)
//   :shortcode:       → emoji                      (GitHub, Slack)
//
// Applied to visible text only (see HtmlTextRuns): never inside code, links
// or math. A delimiter pair must be inside one text run, so ==a **b** c==
// (formatting inside the highlight) is left as written.
//
// Subscript with single tildes (H~2~O) is intentionally NOT supported: md4c
// treats ~x~ as strikethrough, matching GitHub. Use <sub>2</sub> instead.

#include <QString>
#include <QStringView>

namespace mdbr::render {

struct InlineExtensionOptions
{
    bool highlight = true;
    bool superscript = true;
    bool emoji = true;
};

[[nodiscard]] QString applyInlineExtensions(const QString &html, const InlineExtensionOptions &options = {});

// Single-run transforms, exposed for unit tests.
[[nodiscard]] QString applyHighlight(QStringView text);
[[nodiscard]] QString applySuperscript(QStringView text);
[[nodiscard]] QString applyEmojiShortcodes(QStringView text);

} // namespace mdbr::render
