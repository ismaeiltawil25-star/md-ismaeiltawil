#pragma once

#include "PdfByteReader.h"
#include "PdfXRef.h"
#include <QString>

enum class PdfXRefParserWarning : uint8_t {
    None,
    MissingXrefKeyword,
    InvalidStartxref,
    InvalidSubsectionHeader,
    TruncatedEntry,
    MalformedEntry,
    UnexpectedToken,
};

struct PdfXRefParserResult {
    PdfXRefTable table;
    PdfXRefParserWarning warning = PdfXRefParserWarning::None;
    QString diagnosticMessage;
    qsizetype trailerOffset = -1;

    bool ok() const { return warning == PdfXRefParserWarning::None; }
};

class PdfXRefParser {
public:
    PdfXRefParserResult parse(PdfByteReader &reader, qsizetype startxref);

private:
    static qsizetype parseDecimal10(const QByteArrayView &view);
};
