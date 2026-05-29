#include "PdfStreamDecoder.h"
#include <QtZlib/zlib.h>
#include <limits>
#include <algorithm>

// --- Internal types ---

struct InflateResult {
    QByteArray bytes;
    PdfStreamDecoderWarning warning = PdfStreamDecoderWarning::None;
    QString diagnosticMessage;
};

enum class DecodeParmsStatus {
    Default,
    UseObject,
    Unsupported,
};

struct NormalizedDecodeParms {
    DecodeParmsStatus status = DecodeParmsStatus::Default;
    PdfObjectPtr object;
};

// --- Internal helpers ---

static NormalizedDecodeParms normalizeDecodeParms(PdfObjectPtr decodeParms) {
    NormalizedDecodeParms result;

    if (!decodeParms) {
        result.status = DecodeParmsStatus::Default;
        return result;
    }

    if (decodeParms->kind != PdfObjectKind::Array) {
        result.status = DecodeParmsStatus::UseObject;
        result.object = decodeParms;
        return result;
    }

    const auto &arr = decodeParms->arrayValue;
    if (!arr || arr->elements.size() != 1) {
        result.status = DecodeParmsStatus::Unsupported;
        return result;
    }

    result.status = DecodeParmsStatus::UseObject;
    result.object = arr->elements[0];
    return result;
}

static int predictorFromParms(PdfObjectPtr decodeParms) {
    if (!decodeParms) return 1;
    if (decodeParms->kind != PdfObjectKind::Dictionary) return 1;
    auto parms = decodeParms->dictValue;
    if (!parms) return 1;
    auto predObj = parms->get("Predictor");
    if (!predObj) return 1;
    if (predObj->kind != PdfObjectKind::Integer) return -1;
    int64_t pred = predObj->intValue;
    if (pred == 1) return 1;
    return -1;
}

static InflateResult zlibInflate(QByteArrayView input, qint64 maxOutput) {
    InflateResult result;

    if (input.isEmpty()) return result;

    z_stream strm = {};
    int ret = inflateInit(&strm);
    if (ret != Z_OK) {
        result.warning = PdfStreamDecoderWarning::DecodeFailure;
        result.diagnosticMessage = QStringLiteral("inflateInit failed: %1").arg(ret);
        return result;
    }

    QByteArray output;
    constexpr qint64 CHUNK = 32768;
    qsizetype inputOffset = 0;

    bool done = false;
    while (!done) {
        if (strm.avail_in == 0 && inputOffset < input.size()) {
            qsizetype remaining = input.size() - inputOffset;
            uInt chunk = static_cast<uInt>(std::min<qsizetype>(
                remaining, std::numeric_limits<uInt>::max()));
            strm.avail_in = chunk;
            strm.next_in = reinterpret_cast<Bytef*>(
                const_cast<char*>(input.data()) + inputOffset);
            inputOffset += chunk;
        }

        qint64 remainingOutput = maxOutput - output.size();
        if (remainingOutput <= 0) {
            // Output at limit — probe
            if (strm.avail_in == 0 && inputOffset < input.size()) {
                qsizetype rem = input.size() - inputOffset;
                uInt c = static_cast<uInt>(std::min<qsizetype>(
                    rem, std::numeric_limits<uInt>::max()));
                strm.avail_in = c;
                strm.next_in = reinterpret_cast<Bytef*>(
                    const_cast<char*>(input.data()) + inputOffset);
                inputOffset += c;
            }

            Bytef probe = 0;
            strm.avail_out = 1;
            strm.next_out = &probe;
            int probeRet = inflate(&strm, Z_NO_FLUSH);

            if (strm.avail_out == 0) {
                inflateEnd(&strm);
                result.warning = PdfStreamDecoderWarning::OutputTooLarge;
                result.diagnosticMessage = QStringLiteral("Decoded output exceeded limit %1").arg(maxOutput);
                return result;
            }

            if (probeRet == Z_STREAM_END) {
                break;
            }

            inflateEnd(&strm);
            if (probeRet == Z_OK) {
                result.warning = PdfStreamDecoderWarning::OutputTooLarge;
                result.diagnosticMessage = QStringLiteral("Stream continues beyond output limit %1").arg(maxOutput);
            } else {
                result.warning = PdfStreamDecoderWarning::DecodeFailure;
                result.diagnosticMessage = QStringLiteral("Inflate error at output limit: %1").arg(probeRet);
            }
            return result;
        }

        uInt chunkOut = static_cast<uInt>(std::min<qint64>(CHUNK, remainingOutput));
        qint64 oldSize = output.size();
        output.resize(oldSize + chunkOut);
        strm.avail_out = chunkOut;
        strm.next_out = reinterpret_cast<Bytef*>(output.data() + oldSize);

        ret = inflate(&strm, Z_NO_FLUSH);
        output.resize(oldSize + chunkOut - strm.avail_out);

        if (ret == Z_STREAM_END) {
            done = true;
        } else if (ret != Z_OK) {
            inflateEnd(&strm);
            result.warning = PdfStreamDecoderWarning::DecodeFailure;
            result.diagnosticMessage = QStringLiteral("inflate failed: %1").arg(ret);
            return result;
        }
    }

    inflateEnd(&strm);
    result.bytes = output;
    return result;
}

// --- PdfStreamDecoder ---

bool PdfStreamDecoder::isFlateFilter(const QByteArray &name) {
    return name == "FlateDecode" || name == "Fl";
}

PdfStreamDecoder::PdfStreamDecoder(qint64 maxOutputSize)
    : m_maxOutputSize(std::max<qint64>(0, maxOutputSize)) {}

PdfDecodeResult PdfStreamDecoder::decode(const PdfRawStreamObject &stream) {
    if (stream.filters.size() == 0) {
        return decodeIdentity(stream.rawBody);
    }
    if (stream.filters.size() >= 2) {
        PdfDecodeResult r;
        r.warning = PdfStreamDecoderWarning::UnsupportedFilterChain;
        r.diagnosticMessage = QStringLiteral("Multiple filters not supported");
        return r;
    }

    if (isFlateFilter(stream.filters[0])) {
        return decodeFlate(stream.rawBody, stream.decodeParms);
    }

    PdfDecodeResult r;
    r.warning = PdfStreamDecoderWarning::UnsupportedFilter;
    r.diagnosticMessage = QStringLiteral("Unsupported filter: %1").arg(QString::fromUtf8(stream.filters[0]));
    return r;
}

PdfDecodeResult PdfStreamDecoder::decodeIdentity(QByteArrayView rawBody) {
    PdfDecodeResult r;
    if (rawBody.size() > m_maxOutputSize) {
        r.warning = PdfStreamDecoderWarning::OutputTooLarge;
        r.diagnosticMessage = QStringLiteral("Identity stream size %1 exceeds max %2")
            .arg(rawBody.size()).arg(m_maxOutputSize);
        return r;
    }
    r.decodedBytes = rawBody.toByteArray();
    return r;
}

PdfDecodeResult PdfStreamDecoder::decodeFlate(QByteArrayView rawBody, PdfObjectPtr decodeParms) {
    PdfDecodeResult r;

    auto norm = normalizeDecodeParms(decodeParms);
    if (norm.status == DecodeParmsStatus::Unsupported) {
        r.warning = PdfStreamDecoderWarning::UnsupportedPredictor;
        r.diagnosticMessage = QStringLiteral("Multi-element /DecodeParms array not supported for single filter");
        return r;
    }

    PdfObjectPtr effectiveParms = (norm.status == DecodeParmsStatus::UseObject)
        ? norm.object : PdfObjectPtr{};
    int pred = predictorFromParms(effectiveParms);
    if (pred != 1) {
        r.warning = PdfStreamDecoderWarning::UnsupportedPredictor;
        r.diagnosticMessage = QStringLiteral("Unsupported Predictor value in DecodeParms");
        return r;
    }

    if (rawBody.isEmpty()) return r;

    auto ir = zlibInflate(rawBody, m_maxOutputSize);
    r.decodedBytes = std::move(ir.bytes);
    r.warning = ir.warning;
    r.diagnosticMessage = ir.diagnosticMessage;
    return r;
}
