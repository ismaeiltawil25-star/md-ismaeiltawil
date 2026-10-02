#pragma once

// TextDecoder — turns raw file bytes into Unicode text.
//
// Responsibility: encoding detection and decoding ONLY. It knows nothing
// about Markdown, files, or the UI.
//
// Detection order (first match wins):
//   1. Byte Order Mark (UTF-8 / UTF-16 / UTF-32)          -> exact
//   2. BOM-less UTF-16 (one-sided NUL byte pattern)        -> guessed
//   3. Strictly valid UTF-8                                -> exact
//   4. Legacy single-byte code page:
//        Windows-1256 (Arabic) when the high bytes form Arabic words,
//        otherwise Windows-1252 (Western European)         -> guessed
//
// Single-byte decoding never fails, so decode() always returns text.
// Callers should surface `isGuessed` to the user, because a guess can be wrong.

#include <QByteArrayView>
#include <QString>

namespace mdbr::text {

enum class SourceEncoding
{
    Utf8,
    Utf8WithBom,
    Utf16LE,
    Utf16BE,
    Utf32LE,
    Utf32BE,
    Windows1256,
    Windows1252,
};

[[nodiscard]] QString encodingName(SourceEncoding encoding);

struct DecodedText
{
    QString text;
    SourceEncoding encoding = SourceEncoding::Utf8;
    bool isGuessed = false; // true when chosen by heuristic, not by BOM or strict validation
};

[[nodiscard]] DecodedText decode(QByteArrayView bytes);

// Converts CRLF and lone CR to LF so every later stage sees a single line-ending style.
[[nodiscard]] QString normalizeLineEndings(QString text);

} // namespace mdbr::text
