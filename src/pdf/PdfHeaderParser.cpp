#include "PdfHeaderParser.h"

static constexpr qsizetype kMaxLeadingWhitespace = 1024;
static constexpr qsizetype kHeaderSize = 8;

static bool isAsciiDigit(char c) {
    return c >= '0' && c <= '9';
}

HeaderParseResult PdfHeaderParser::parse(PdfByteReader &reader) {
    HeaderParseResult result;

    qsizetype startPos = reader.tell();

    reader.skipAsciiWhitespace();

    if (reader.remaining() < kHeaderSize) {
        reader.seek(startPos);
        result.error = PdfHeaderError::TooShort;
        result.errorMessage = QStringLiteral("File too short for PDF header");
        return result;
    }

    QByteArrayView header = reader.viewAt(reader.tell(), kHeaderSize);
    if (header.size() < kHeaderSize) {
        reader.seek(startPos);
        result.error = PdfHeaderError::TooShort;
        result.errorMessage = QStringLiteral("File too short for PDF header");
        return result;
    }

    if (header[0] != '%' || header[1] != 'P' || header[2] != 'D' || header[3] != 'F' || header[4] != '-') {
        reader.seek(startPos);
        result.error = PdfHeaderError::MissingMagic;
        result.errorMessage = QStringLiteral("Missing %PDF- magic");
        return result;
    }

    char majorChar = header[5];
    char dotChar = header[6];
    char minorChar = header[7];

    if (!isAsciiDigit(majorChar)) {
        reader.seek(startPos);
        result.error = PdfHeaderError::InvalidVersionDigits;
        result.errorMessage = QStringLiteral("Major version is not a digit");
        return result;
    }

    if (dotChar != '.') {
        reader.seek(startPos);
        result.error = PdfHeaderError::MissingVersionDot;
        result.errorMessage = QStringLiteral("Missing version dot separator");
        return result;
    }

    if (!isAsciiDigit(minorChar)) {
        reader.seek(startPos);
        result.error = PdfHeaderError::InvalidVersionDigits;
        result.errorMessage = QStringLiteral("Minor version is not a digit");
        return result;
    }

    int major = majorChar - '0';
    int minor = minorChar - '0';

    if (major != 1) {
        reader.seek(startPos);
        result.error = PdfHeaderError::UnsupportedMajorVersion;
        result.errorMessage = QStringLiteral("Unsupported major version: %1").arg(major);
        return result;
    }

    if (minor > 7) {
        reader.seek(startPos);
        result.error = PdfHeaderError::UnsupportedMinorVersion;
        result.errorMessage = QStringLiteral("Unsupported minor version: %1").arg(minor);
        return result;
    }

    reader.skipBytes(kHeaderSize);

    result.success = true;
    result.version = PdfVersion{static_cast<uint8_t>(major), static_cast<uint8_t>(minor)};
    return result;
}
