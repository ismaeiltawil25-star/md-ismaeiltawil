#include "PdfStreamObjectReader.h"

static void collectFilterNames(PdfObjectPtr filterValue, QList<QByteArray> &filters) {
    if (filterValue->kind == PdfObjectKind::Name) {
        filters.append(filterValue->nameValue);
    } else if (filterValue->kind == PdfObjectKind::Array) {
        for (const auto &elem : filterValue->arrayValue->elements) {
            if (elem && elem->kind == PdfObjectKind::Name) {
                filters.append(elem->nameValue);
            }
        }
    }
}

PdfStreamObjectResult PdfStreamObjectReader::read(PdfByteReader &reader,
                                                    PdfObjectPtr dictionary,
                                                    qsizetype streamKeywordOffset,
                                                    PdfRef identity) {
    PdfStreamObjectResult result;

    // 1. Validate dictionary
    if (!dictionary) {
        result.warning = PdfStreamObjectReaderWarning::NullDictionary;
        result.diagnosticMessage = QStringLiteral("Stream dictionary is null");
        return result;
    }
    if (dictionary->kind != PdfObjectKind::Dictionary) {
        result.warning = PdfStreamObjectReaderWarning::InvalidDictionary;
        result.diagnosticMessage = QStringLiteral("Stream dictionary is not a dictionary");
        return result;
    }

    PdfDictionary &dict = *dictionary->dictValue;

    // 2. Extract metadata early
    result.stream.dictionary = dictionary;

    if (auto filterIt = dict.entries.find(QByteArray("Filter")); filterIt != dict.entries.end()) {
        collectFilterNames(filterIt.value(), result.stream.filters);
    }

    if (auto dpIt = dict.entries.find(QByteArray("DecodeParms")); dpIt != dict.entries.end()) {
        result.stream.decodeParms = dpIt.value();
    }

    if (identity.objNum > 0 && identity.genNum >= 0) {
        result.stream.identity = identity;
        result.stream.hasIdentity = true;
    }

    // 3. Validate /Length
    auto lengthIt = dict.entries.find(QByteArray("Length"));
    if (lengthIt == dict.entries.end()) {
        result.warning = PdfStreamObjectReaderWarning::MissingLength;
        result.diagnosticMessage = QStringLiteral("Stream dictionary missing /Length key");
        return result;
    }

    PdfObjectPtr lenObj = lengthIt.value();
    if (!lenObj) {
        result.warning = PdfStreamObjectReaderWarning::MissingLength;
        result.diagnosticMessage = QStringLiteral("Stream dictionary /Length value is null");
        return result;
    }

    if (lenObj->kind == PdfObjectKind::Reference) {
        result.warning = PdfStreamObjectReaderWarning::UnsupportedIndirectLength;
        result.diagnosticMessage = QStringLiteral("Indirect /Length reference not supported");
        return result;
    }

    if (lenObj->kind != PdfObjectKind::Integer) {
        result.warning = PdfStreamObjectReaderWarning::InvalidLengthType;
        result.diagnosticMessage = QStringLiteral("/Length is not an integer");
        return result;
    }

    int64_t rawLength = lenObj->intValue;

    if (rawLength < 0) {
        result.warning = PdfStreamObjectReaderWarning::NegativeLength;
        result.diagnosticMessage = QStringLiteral("/Length is negative: %1").arg(rawLength);
        return result;
    }

    if (rawLength > static_cast<int64_t>(std::numeric_limits<qsizetype>::max())) {
        result.warning = PdfStreamObjectReaderWarning::LengthOutOfRange;
        result.diagnosticMessage = QStringLiteral("/Length exceeds qsizetype range: %1").arg(rawLength);
        return result;
    }

    qsizetype length = static_cast<qsizetype>(rawLength);
    result.stream.length = length;

    // 4. Validate stream offset
    if (streamKeywordOffset < 0 || streamKeywordOffset >= reader.dataView().size()) {
        result.warning = PdfStreamObjectReaderWarning::InvalidStreamOffset;
        result.diagnosticMessage = QStringLiteral("Invalid stream keyword offset: %1").arg(streamKeywordOffset);
        return result;
    }

    // 5. Delegate to PdfStreamBodyReader
    PdfStreamBodyResult bodyResult = PdfStreamBodyReader::readBody(reader, streamKeywordOffset, length);

    if (bodyResult.ok()) {
        result.stream.rawBody = bodyResult.body;
        return result;
    }

    result.stream.rawBody = bodyResult.body;
    result.warning = PdfStreamObjectReaderWarning::BodyReaderFailure;
    result.bodyWarning = bodyResult.warning;
    result.diagnosticMessage = bodyResult.diagnosticMessage;
    return result;
}
