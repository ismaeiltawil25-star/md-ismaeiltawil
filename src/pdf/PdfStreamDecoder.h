#pragma once

#include "PdfRawStreamObject.h"
#include <QByteArray>
#include <QString>
#include <cstdint>

enum class PdfStreamDecoderWarning : uint8_t {
    None,
    UnsupportedFilter,
    UnsupportedFilterChain,
    UnsupportedPredictor,
    DecodeFailure,
    OutputTooLarge,
};

struct PdfDecodeResult {
    QByteArray decodedBytes;
    PdfStreamDecoderWarning warning = PdfStreamDecoderWarning::None;
    QString diagnosticMessage;

    bool ok() const { return warning == PdfStreamDecoderWarning::None; }
};

class PdfStreamDecoder {
public:
    static constexpr qint64 DEFAULT_MAX_OUTPUT = 256 * 1024 * 1024;

    explicit PdfStreamDecoder(qint64 maxOutputSize = DEFAULT_MAX_OUTPUT);

    PdfDecodeResult decode(const PdfRawStreamObject &stream);

private:
    qint64 m_maxOutputSize;

    PdfDecodeResult decodeIdentity(QByteArrayView rawBody);
    PdfDecodeResult decodeFlate(QByteArrayView rawBody, PdfObjectPtr decodeParms);

    static bool isFlateFilter(const QByteArray &name);
};
