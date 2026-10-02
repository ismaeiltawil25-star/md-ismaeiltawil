#pragma once

// Converters for the custom elements md4c emits when its optional parsers are
// enabled (MarkdownHtmlRenderer turns these flags on):
//
//   MD_FLAG_WIKILINKS      <x-wikilink data-target="Page">label</x-wikilink>
//                          → <a href="Page.md">label</a>
//                            [[Page#Part]] → Page.md#part, [[#Part]] → #part
//
//   MD_FLAG_LATEXMATHSPANS <x-equation>…</x-equation>                 $…$
//                          <x-equation type="display">…</x-equation>  $$…$$
//                          → HTML via LatexMathConverter; a paragraph that
//                            holds only display math becomes a centred block.
//
// md4c does the parsing (so code spans, escapes and nesting follow its rules);
// these functions only translate its output.

#include <QString>

namespace mdbr::render {

[[nodiscard]] QString renderWikiLinks(const QString &html);
[[nodiscard]] QString renderMath(const QString &html);

} // namespace mdbr::render
