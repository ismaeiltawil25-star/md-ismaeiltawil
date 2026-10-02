#pragma once

// FootnoteProcessor — GitHub/Pandoc footnotes, which md4c does not support.
//
//   Text with a note[^1] and another[^long-name].
//
//   [^1]: The note.
//   [^long-name]: A longer note.
//       Indented lines continue the same note.
//
// Output (still Markdown, with small inline-HTML pieces md4c passes through):
//   * each reference becomes a superscript number linking to its note;
//   * definitions are removed from where they were written and collected in
//     a numbered list at the end of the document, each with a back-link ↩;
//   * numbers follow the order of first reference, as on GitHub.
//
// Safety: fenced code (FenceTracker) and inline `code` spans are never
// changed. References without a definition, and definitions that are never
// referenced, are left exactly as written so no text is lost.

#include <QString>

namespace mdbr::markdown {

[[nodiscard]] QString processFootnotes(const QString &markdown);

} // namespace mdbr::markdown
