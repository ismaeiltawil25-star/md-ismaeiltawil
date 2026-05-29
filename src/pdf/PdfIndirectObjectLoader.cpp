#include "PdfIndirectObjectLoader.h"
#include "PdfObjectParser.h"
#include "PdfStreamObjectReader.h"

static void setWarning(PdfLoadResult &r, PdfIndirectObjectLoaderWarning w) {
    if (r.warning == PdfIndirectObjectLoaderWarning::None || static_cast<int>(w) < static_cast<int>(r.warning)) {
        r.warning = w;
    }
}

static bool isPdfBoundary(uint8_t byte) {
    if (byte == 0x00 || byte == 0x09 || byte == 0x0A ||
        byte == 0x0C || byte == 0x0D || byte == 0x20) return true;
    if (byte == '(' || byte == ')' || byte == '<' || byte == '>' ||
        byte == '[' || byte == ']' || byte == '{' || byte == '}' ||
        byte == '/' || byte == '%') return true;
    return false;
}

static bool validateEndobj(PdfByteReader &reader) {
    reader.skipAsciiWhitespace();

    const qsizetype checkPos = reader.tell();
    QByteArrayView candidate = reader.viewAt(checkPos, 6);
    if (candidate.size() < 6 || candidate != QByteArrayView("endobj")) {
        return false;
    }
    reader.readBytes(6);

    if (!reader.eof()) {
        uint8_t nextByte = reader.peekByte().value();
        if (!isPdfBoundary(nextByte)) {
            return false;
        }
    }
    return true;
}

PdfLoadResult PdfIndirectObjectLoader::load(PdfByteReader &reader,
                                              const PdfXRefTable &xrefTable,
                                              PdfObjectStore &store,
                                              PdfRawStreamStore *streamStore) {
    PdfLoadResult result;
    qsizetype totalSize = reader.dataView().size();

    for (const auto &sub : xrefTable.subsections) {
        result.totalXRefEntries += sub.count;

        for (const auto &entry : sub.entries) {
            if (entry.type == PdfXRefEntryType::Free) {
                result.freeCount++;
                continue;
            }

            result.inUseEntries++;

            if (entry.objNum <= 0) {
                setWarning(result, PdfIndirectObjectLoaderWarning::StoreInsertFailure);
                result.diagnosticMessage = QStringLiteral("Invalid object number in xref entry: %1").arg(entry.objNum);
                result.skippedCount++;
                continue;
            }

            if (entry.offset < 0 || entry.offset >= totalSize) {
                setWarning(result, PdfIndirectObjectLoaderWarning::InvalidObjectOffset);
                result.diagnosticMessage = QStringLiteral("Invalid xref offset: %1").arg(entry.offset);
                result.skippedCount++;
                continue;
            }

            reader.seek(entry.offset);
            PdfTokenizer tokenizer(reader);
            PdfObjectParser parser(tokenizer);
            auto parsed = parser.parseIndirectObject();

            if (!parsed.object) {
                setWarning(result, PdfIndirectObjectLoaderWarning::ParseFailure);
                result.diagnosticMessage = QStringLiteral("Failed to parse indirect object at offset %1").arg(entry.offset);
                result.skippedCount++;
                continue;
            }

            if (parsed.warning == PdfObjectParserWarning::MissingEndobj) {
                setWarning(result, PdfIndirectObjectLoaderWarning::ParseFailure);
                result.diagnosticMessage = QStringLiteral("Indirect object missing endobj at offset %1").arg(entry.offset);
                result.skippedCount++;
                continue;
            }

            if (parsed.warning == PdfObjectParserWarning::UnsupportedStreamObject) {
                if (!streamStore) {
                    setWarning(result, PdfIndirectObjectLoaderWarning::UnsupportedStream);
                    result.diagnosticMessage = QStringLiteral("Stream object not supported at offset %1").arg(entry.offset);
                    result.unsupportedStreamCount++;
                    result.skippedCount++;
                    continue;
                }

                if (parsed.streamKeywordOffset < 0 ||
                    parsed.object->kind != PdfObjectKind::Dictionary) {
                    setWarning(result, PdfIndirectObjectLoaderWarning::ParseFailure);
                    result.diagnosticMessage = QStringLiteral("Stream object parse failure at offset %1").arg(entry.offset);
                    result.skippedCount++;
                    continue;
                }

                if (parsed.objNum != entry.objNum || parsed.genNum != entry.genNum) {
                    setWarning(result, PdfIndirectObjectLoaderWarning::ObjectNumberMismatch);
                    result.diagnosticMessage = QStringLiteral("Object number mismatch: xref=%1 g%2, object=%3 g%4")
                        .arg(entry.objNum).arg(entry.genNum).arg(parsed.objNum).arg(parsed.genNum);
                    result.mismatchCount++;
                    result.skippedCount++;
                    continue;
                }

                // Store dictionary in PdfObjectStore before body extraction
                auto storeResult = store.add(entry.objNum, entry.genNum, parsed.object);
                result.loadedCount++;
                if (storeResult.warning == PdfObjectStoreWarning::DuplicateObjectKey) {
                    result.duplicateCount++;
                }

                // Call PdfStreamObjectReader
                PdfStreamObjectResult streamResult = PdfStreamObjectReader::read(
                    reader, parsed.object, parsed.streamKeywordOffset,
                    PdfRef{entry.objNum, entry.genNum});

                if (!streamResult.ok()) {
                    result.streamFailureCount++;
                    continue;
                }

                // Validate endobj after stream body
                if (!validateEndobj(reader)) {
                    result.streamFailureCount++;
                    continue;
                }

                // Store in PdfRawStreamStore
                auto addResult = streamStore->add(
                    entry.objNum, entry.genNum, streamResult.stream);

                if (addResult.warning == PdfRawStreamStoreWarning::InvalidStreamKey) {
                    result.streamFailureCount++;
                } else {
                    result.streamLoadedCount++;
                    if (addResult.warning == PdfRawStreamStoreWarning::DuplicateStreamKey) {
                        result.duplicateStreamCount++;
                    }
                }
                continue;
            }

            if (parsed.objNum != entry.objNum || parsed.genNum != entry.genNum) {
                setWarning(result, PdfIndirectObjectLoaderWarning::ObjectNumberMismatch);
                result.diagnosticMessage = QStringLiteral("Object number mismatch: xref=%1 g%2, object=%3 g%4")
                    .arg(entry.objNum).arg(entry.genNum).arg(parsed.objNum).arg(parsed.genNum);
                result.mismatchCount++;
                result.skippedCount++;
                continue;
            }

            auto storeResult = store.add(entry.objNum, entry.genNum, parsed.object);

            switch (storeResult.warning) {
            case PdfObjectStoreWarning::None:
                result.loadedCount++;
                break;
            case PdfObjectStoreWarning::DuplicateObjectKey:
                result.loadedCount++;
                result.duplicateCount++;
                setWarning(result, PdfIndirectObjectLoaderWarning::DuplicateObjectKey);
                break;
            case PdfObjectStoreWarning::InvalidObjectKey:
            case PdfObjectStoreWarning::NullObject:
                setWarning(result, PdfIndirectObjectLoaderWarning::StoreInsertFailure);
                result.diagnosticMessage = storeResult.diagnosticMessage;
                result.skippedCount++;
                break;
            default:
                break;
            }
        }
    }

    return result;
}
