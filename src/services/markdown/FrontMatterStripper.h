#pragma once

// FrontMatterStripper — removes a leading YAML front-matter block.
//
// A document is treated as having front matter ONLY when:
//   1. its first line is exactly "---" (trailing whitespace allowed);
//   2. a later line is "---" or "..." (the closing delimiter);
//   3. the block between them is non-empty YAML-looking content: every
//      non-blank line is a `key: value` pair, a `- list item`, an indented
//      continuation, or a `# comment`, and at least one `key:` line exists.
//
// Rule 3 is what prevents silent content loss: a document that merely starts
// with a horizontal rule ("---") followed by prose is returned unchanged.
//
// Input with CRLF line endings is accepted. When nothing is stripped the input
// is returned exactly as given.

#include <QString>

namespace mdbr::markdown {

[[nodiscard]] QString stripFrontMatter(const QString &markdown);

} // namespace mdbr::markdown
