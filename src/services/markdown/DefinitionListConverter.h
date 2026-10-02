#pragma once

// DefinitionListConverter — adds PHP-Markdown-Extra style definition lists,
// which md4c does not support:
//
//     Term
//     : Description
//
// becomes an HTML <table class="definition-table"> that QTextBrowser can style.
//
// Safety rules:
//   * lines inside fenced code blocks are never touched (see FenceTracker);
//   * a term line may not look like another block (heading, list, quote,
//     table row, HTML, fence, indented code);
//   * the emitted table is followed by a blank line so md4c ends the HTML
//     block there and parses the next paragraph as Markdown again.
//
// Known limitation: terms and descriptions are HTML-escaped; only `inline code`
// in descriptions is converted. Other inline Markdown is shown literally.

#include <QString>

namespace mdbr::markdown {

[[nodiscard]] QString convertDefinitionLists(const QString &markdown);

} // namespace mdbr::markdown
