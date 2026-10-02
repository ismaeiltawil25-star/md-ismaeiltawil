#pragma once

// SyntaxHighlighter — colours fenced code blocks with a language tag:
//
//   ```cpp            → <pre><code class="language-cpp"> … </code></pre>
//
// Tokens are wrapped in <span class="tok-…"> (keyword, type, string, comment,
// number, preproc); the colours come from the reader stylesheet, so they
// follow the day/night theme (ReaderStylesheetBuilder).
//
// Languages are data, not code: see SyntaxLanguages.cpp. To add one, add a
// LanguageRules entry there and a row to SyntaxHighlighterTest. Unknown or
// missing languages are left as plain code.
//
// Upgrade path: a full highlighter (e.g. KDE's MIT-licensed
// KSyntaxHighlighting) can replace highlightCodeBlocks() without touching the
// pipeline, as long as it emits the same tok-* classes.
//
// Limits: blocks larger than kMaxHighlightChars stay plain (keeps huge
// generated files fast). Tokenizing is a single linear pass.

#include <QString>

namespace mdbr::render {

inline constexpr qsizetype kMaxHighlightChars = 200'000;

[[nodiscard]] QString highlightCodeBlocks(const QString &html);

// Highlights one block of plain (unescaped) source; returns escaped HTML.
// Returns nullopt-equivalent (empty + false) when the language is unknown.
[[nodiscard]] bool isKnownLanguage(const QString &language);
[[nodiscard]] QString highlightSource(const QString &source, const QString &language);

} // namespace mdbr::render
