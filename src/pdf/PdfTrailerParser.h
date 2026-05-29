#pragma once

#include "PdfByteReader.h"
#include "PdfTrailer.h"
#include <QString>

enum class PdfTrailerParserWarning : uint8_t {
    None,
    InvalidTrailerOffset,
    MissingTrailerKeyword,
    MissingTrailerDict,
    MalformedTrailerDict,
    MissingRequiredKey,
    EncryptedUnsupported,
};

struct PdfTrailerResult {
    PdfTrailer trailer;
    PdfTrailerParserWarning warning = PdfTrailerParserWarning::None;
    QString diagnosticMessage;

    bool ok() const { return warning == PdfTrailerParserWarning::None; }
};

class PdfTrailerParser {
public:
    PdfTrailerResult parse(PdfByteReader &reader, qsizetype trailerOffset);
};
