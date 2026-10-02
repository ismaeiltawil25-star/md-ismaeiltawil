#include "TextDecoder.h"

#include <QStringDecoder>

#include <array>
#include <optional>

namespace mdbr::text {

namespace {

using CodePageTable = std::array<char16_t, 128>; // maps bytes 0x80..0xFF

// Generated from Python's reference codecs (cp1256 / cp1252).
// Bytes undefined in a code page map to U+FFFD.
constexpr CodePageTable kWindows1256 = {
    0x20AC, 0x067E, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
    0x02C6, 0x2030, 0x0679, 0x2039, 0x0152, 0x0686, 0x0698, 0x0688,
    0x06AF, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0x06A9, 0x2122, 0x0691, 0x203A, 0x0153, 0x200C, 0x200D, 0x06BA,
    0x00A0, 0x060C, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
    0x00A8, 0x00A9, 0x06BE, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
    0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
    0x00B8, 0x00B9, 0x061B, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x061F,
    0x06C1, 0x0621, 0x0622, 0x0623, 0x0624, 0x0625, 0x0626, 0x0627,
    0x0628, 0x0629, 0x062A, 0x062B, 0x062C, 0x062D, 0x062E, 0x062F,
    0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x00D7,
    0x0637, 0x0638, 0x0639, 0x063A, 0x0640, 0x0641, 0x0642, 0x0643,
    0x00E0, 0x0644, 0x00E2, 0x0645, 0x0646, 0x0647, 0x0648, 0x00E7,
    0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x0649, 0x064A, 0x00EE, 0x00EF,
    0x064B, 0x064C, 0x064D, 0x064E, 0x00F4, 0x064F, 0x0650, 0x00F7,
    0x0651, 0x00F9, 0x0652, 0x00FB, 0x00FC, 0x200E, 0x200F, 0x06D2,
};

constexpr CodePageTable kWindows1252 = {
    0x20AC, 0xFFFD, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
    0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0xFFFD, 0x017D, 0xFFFD,
    0xFFFD, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0xFFFD, 0x017E, 0x0178,
    0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
    0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
    0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
    0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
    0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
    0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
    0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
    0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF,
    0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7,
    0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
    0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7,
    0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FF,
};

[[nodiscard]] constexpr unsigned char byteAt(QByteArrayView bytes, qsizetype i)
{
    return static_cast<unsigned char>(bytes[i]);
}

[[nodiscard]] constexpr bool isArabicBlock(char16_t cp)
{
    return cp >= 0x0600 && cp <= 0x06FF;
}

[[nodiscard]] QString decodeSingleByte(QByteArrayView bytes, const CodePageTable &table)
{
    QString out(bytes.size(), Qt::Uninitialized);
    QChar *dst = out.data();
    for (qsizetype i = 0; i < bytes.size(); ++i) {
        const unsigned char b = byteAt(bytes, i);
        dst[i] = b < 0x80 ? QChar(b) : QChar(table[b - 0x80]);
    }
    return out;
}

[[nodiscard]] std::optional<QString> decodeStrictUtf8(QByteArrayView bytes)
{
    QStringDecoder decoder(QStringConverter::Utf8, QStringConverter::Flag::Stateless);
    QString text = decoder.decode(bytes);
    if (decoder.hasError())
        return std::nullopt;
    return text;
}

[[nodiscard]] std::optional<SourceEncoding> fromBomEncoding(QStringConverter::Encoding encoding)
{
    switch (encoding) {
    case QStringConverter::Utf8:    return SourceEncoding::Utf8WithBom;
    case QStringConverter::Utf16LE: return SourceEncoding::Utf16LE;
    case QStringConverter::Utf16BE: return SourceEncoding::Utf16BE;
    case QStringConverter::Utf32LE: return SourceEncoding::Utf32LE;
    case QStringConverter::Utf32BE: return SourceEncoding::Utf32BE;
    default:                        return std::nullopt;
    }
}

// UTF-16 text without a BOM: ASCII characters leave a NUL in one half of almost
// every code unit, while the other half is almost never NUL.
[[nodiscard]] std::optional<SourceEncoding> detectBomlessUtf16(QByteArrayView bytes)
{
    if (bytes.size() < 4 || bytes.size() % 2 != 0)
        return std::nullopt;

    qsizetype evenNuls = 0;
    qsizetype oddNuls = 0;
    for (qsizetype i = 0; i < bytes.size(); ++i) {
        if (bytes[i] == '\0')
            ++(i % 2 == 0 ? evenNuls : oddNuls);
    }

    const qsizetype units = bytes.size() / 2;
    const auto dominant = [units](qsizetype many, qsizetype few) {
        return many * 4 >= units && few * 20 <= units;
    };

    if (dominant(oddNuls, evenNuls))
        return SourceEncoding::Utf16LE;
    if (dominant(evenNuls, oddNuls))
        return SourceEncoding::Utf16BE;
    return std::nullopt;
}

// Windows-1256 shares many byte values with Windows-1252 (e.g. 0xE4 is 'ä' or 'م').
// Arabic is chosen only when most high bytes map to Arabic letters AND most of them
// sit in runs of 3+ high bytes (Arabic words). Accented letters inside Latin words
// ("Grüße", "café") form runs of at most 1-2 bytes and do not qualify.
[[nodiscard]] bool looksLikeWindows1256Arabic(QByteArrayView bytes)
{
    constexpr qsizetype kMinWordRun = 3;

    qsizetype highBytes = 0;
    qsizetype arabicLetters = 0;
    qsizetype inWordRuns = 0;
    qsizetype runLength = 0;

    const auto closeRun = [&] {
        if (runLength >= kMinWordRun)
            inWordRuns += runLength;
        runLength = 0;
    };

    for (qsizetype i = 0; i < bytes.size(); ++i) {
        const unsigned char b = byteAt(bytes, i);
        if (b < 0x80) {
            closeRun();
            continue;
        }
        ++highBytes;
        ++runLength;
        if (isArabicBlock(kWindows1256[b - 0x80]))
            ++arabicLetters;
    }
    closeRun();

    if (highBytes == 0)
        return false;
    return arabicLetters * 10 >= highBytes * 6 && inWordRuns * 2 >= highBytes;
}

[[nodiscard]] QStringConverter::Encoding toQtEncoding(SourceEncoding encoding)
{
    switch (encoding) {
    case SourceEncoding::Utf16LE: return QStringConverter::Utf16LE;
    case SourceEncoding::Utf16BE: return QStringConverter::Utf16BE;
    case SourceEncoding::Utf32LE: return QStringConverter::Utf32LE;
    case SourceEncoding::Utf32BE: return QStringConverter::Utf32BE;
    default:                      return QStringConverter::Utf8;
    }
}

[[nodiscard]] QString decodeWith(SourceEncoding encoding, QByteArrayView bytes)
{
    // The default decoder flags strip a leading BOM.
    QStringDecoder decoder(toQtEncoding(encoding));
    return decoder.decode(bytes);
}

} // namespace

QString encodingName(SourceEncoding encoding)
{
    switch (encoding) {
    case SourceEncoding::Utf8:        return QStringLiteral("UTF-8");
    case SourceEncoding::Utf8WithBom: return QStringLiteral("UTF-8 (BOM)");
    case SourceEncoding::Utf16LE:     return QStringLiteral("UTF-16 LE");
    case SourceEncoding::Utf16BE:     return QStringLiteral("UTF-16 BE");
    case SourceEncoding::Utf32LE:     return QStringLiteral("UTF-32 LE");
    case SourceEncoding::Utf32BE:     return QStringLiteral("UTF-32 BE");
    case SourceEncoding::Windows1256: return QStringLiteral("Windows-1256");
    case SourceEncoding::Windows1252: return QStringLiteral("Windows-1252");
    }
    return QStringLiteral("unknown");
}

DecodedText decode(QByteArrayView bytes)
{
    if (bytes.isEmpty())
        return {};

    if (const auto qtEncoding = QStringConverter::encodingForData(bytes)) {
        if (const auto encoding = fromBomEncoding(*qtEncoding))
            return {decodeWith(*encoding, bytes), *encoding, false};
    }

    if (const auto utf16 = detectBomlessUtf16(bytes))
        return {decodeWith(*utf16, bytes), *utf16, true};

    if (auto utf8 = decodeStrictUtf8(bytes))
        return {std::move(*utf8), SourceEncoding::Utf8, false};

    if (looksLikeWindows1256Arabic(bytes))
        return {decodeSingleByte(bytes, kWindows1256), SourceEncoding::Windows1256, true};

    return {decodeSingleByte(bytes, kWindows1252), SourceEncoding::Windows1252, true};
}

QString normalizeLineEndings(QString text)
{
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    return text;
}

} // namespace mdbr::text
