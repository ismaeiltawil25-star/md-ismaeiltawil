#pragma once

#include "PdfByteReader.h"
#include "PdfVersion.h"
#include <QString>

enum class PdfHeaderError {
    None,
    TooShort,
    MissingMagic,
    MissingVersionDot,
    InvalidVersionDigits,
    UnsupportedMajorVersion,
    UnsupportedMinorVersion
};

struct HeaderParseResult {
    bool success = false;
    PdfVersion version{};
    PdfHeaderError error = PdfHeaderError::None;
    QString errorMessage;
};

class PdfHeaderParser {
public:
    static HeaderParseResult parse(PdfByteReader &reader);
};
