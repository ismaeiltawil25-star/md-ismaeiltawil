#pragma once

#include "PdfRawStreamObject.h"
#include "PdfByteReader.h"
#include "PdfStreamBodyReader.h"
#include <QString>
#include <cstdint>

enum class PdfStreamObjectReaderWarning : uint8_t {
    None,
    NullDictionary,
    InvalidDictionary,
    MissingLength,
    NegativeLength,
    InvalidLengthType,
    UnsupportedIndirectLength,
    LengthOutOfRange,
    InvalidStreamOffset,
    BodyReaderFailure,
};

struct PdfStreamObjectResult {
    PdfRawStreamObject stream;
    PdfStreamObjectReaderWarning warning = PdfStreamObjectReaderWarning::None;
    PdfStreamBodyReaderWarning bodyWarning = PdfStreamBodyReaderWarning::None;
    QString diagnosticMessage;

    bool ok() const { return warning == PdfStreamObjectReaderWarning::None; }
};

class PdfStreamObjectReader {
public:
    static PdfStreamObjectResult read(PdfByteReader &reader,
                                       PdfObjectPtr dictionary,
                                       qsizetype streamKeywordOffset,
                                       PdfRef identity = PdfRef{0, 0});
};
