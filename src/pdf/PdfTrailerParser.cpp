#include "PdfTrailerParser.h"
#include "PdfTokenizer.h"
#include "PdfObjectParser.h"
#include <limits>

PdfTrailerResult PdfTrailerParser::parse(PdfByteReader &reader, qsizetype trailerOffset) {
    PdfTrailerResult result;
    qsizetype originalPos = reader.tell();
    qsizetype totalSize = reader.dataView().size();

    if (trailerOffset < 0 || trailerOffset >= totalSize) {
        reader.seek(originalPos);
        result.warning = PdfTrailerParserWarning::InvalidTrailerOffset;
        result.diagnosticMessage = QStringLiteral("Invalid trailer offset: %1").arg(trailerOffset);
        return result;
    }

    reader.seek(trailerOffset);

    PdfTokenizer tokenizer(reader);
    auto first = tokenizer.nextToken();

    if (first.token.type != PdfTokenType::Keyword || first.token.bytes != "trailer") {
        result.warning = PdfTrailerParserWarning::MissingTrailerKeyword;
        result.diagnosticMessage = QStringLiteral("Expected 'trailer' keyword at offset %1").arg(trailerOffset);
        return result;
    }

    PdfObjectParser objectParser(tokenizer);
    auto objResult = objectParser.parseObject();

    if (!objResult.object) {
        result.warning = PdfTrailerParserWarning::MissingTrailerDict;
        result.diagnosticMessage = QStringLiteral("Missing trailer dictionary");
        return result;
    }

    if (objResult.object->kind != PdfObjectKind::Dictionary) {
        result.warning = PdfTrailerParserWarning::MissingTrailerDict;
        result.diagnosticMessage = QStringLiteral("Trailer content is not a dictionary");
        return result;
    }

    auto &dict = objResult.object->dictValue;
    result.trailer.rawDict = dict;

    PdfTrailerParserWarning highestWarning = PdfTrailerParserWarning::None;

    // Check for malformed dict warning from object parser
    if (objResult.warning != PdfObjectParserWarning::None) {
        highestWarning = PdfTrailerParserWarning::MalformedTrailerDict;
        result.diagnosticMessage = objResult.diagnosticMessage;
    }

    // Extract /Size
    auto sizeObj = dict->get("Size");
    bool sizeValid = false;
    if (sizeObj && sizeObj->kind == PdfObjectKind::Integer) {
        int64_t sizeVal = sizeObj->intValue;
        if (sizeVal > 0 && sizeVal <= static_cast<int64_t>(std::numeric_limits<int>::max())) {
            result.trailer.size = static_cast<int>(sizeVal);
            sizeValid = true;
        }
    }

    // Extract /Root
    auto rootObj = dict->get("Root");
    bool rootValid = false;
    if (rootObj && rootObj->kind == PdfObjectKind::Reference) {
        if (rootObj->refValue.objNum > 0 && rootObj->refValue.genNum >= 0) {
            result.trailer.root = rootObj->refValue;
            rootValid = true;
        }
    }

    if (!sizeValid || !rootValid) {
        if (highestWarning == PdfTrailerParserWarning::None) {
            highestWarning = PdfTrailerParserWarning::MissingRequiredKey;
            QString missing;
            if (!sizeValid) missing += "Size ";
            if (!rootValid) missing += "Root";
            result.diagnosticMessage = QStringLiteral("Missing or invalid required trailer key(s): %1").arg(missing.trimmed());
        }
        result.warning = highestWarning;
        return result;
    }

    // Extract /Prev
    auto prevObj = dict->get("Prev");
    if (prevObj && prevObj->kind == PdfObjectKind::Integer) {
        int64_t pv = prevObj->intValue;
        if (pv >= 0 && pv <= static_cast<int64_t>(std::numeric_limits<qsizetype>::max())) {
            result.trailer.prev = static_cast<qsizetype>(pv);
        }
    }

    // Extract /ID
    auto idObj = dict->get("ID");
    if (idObj && idObj->kind == PdfObjectKind::Array) {
        auto &arr = idObj->arrayValue->elements;
        if (arr.size() == 2 &&
            arr[0]->kind == PdfObjectKind::String &&
            arr[1]->kind == PdfObjectKind::String) {
            PdfFileId fid;
            fid.first = arr[0]->stringValue;
            fid.second = arr[1]->stringValue;
            result.trailer.id = fid;
        }
    }

    // Check for /Encrypt
    if (dict->has("Encrypt")) {
        result.trailer.encrypted = true;
        if (highestWarning == PdfTrailerParserWarning::None) {
            highestWarning = PdfTrailerParserWarning::EncryptedUnsupported;
            result.diagnosticMessage = QStringLiteral("PDF document is encrypted");
        }
    }

    result.warning = highestWarning;
    return result;
}
