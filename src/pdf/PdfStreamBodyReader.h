#pragma once

#include "PdfByteReader.h"
#include <QByteArrayView>
#include <QString>
#include <cstdint>

enum class PdfStreamBodyReaderWarning : uint8_t {
    None,
    InvalidStreamOffset,
    InvalidLength,
    MissingStreamKeyword,
    MissingStreamEol,
    DataTooShort,
    MissingEndstream,
    InvalidEndstreamBoundary,
};

struct PdfStreamBodyResult {
    QByteArrayView body;
    PdfStreamBodyReaderWarning warning = PdfStreamBodyReaderWarning::None;
    QString diagnosticMessage;

    bool ok() const { return warning == PdfStreamBodyReaderWarning::None; }
};

class PdfStreamBodyReader {
public:
    static PdfStreamBodyResult readBody(PdfByteReader &reader,
                                         qsizetype streamKeywordOffset,
                                         qsizetype length);

private:
    static int consumeSingleEol(PdfByteReader &reader);
    static bool isPdfBoundary(uint8_t byte);
};
