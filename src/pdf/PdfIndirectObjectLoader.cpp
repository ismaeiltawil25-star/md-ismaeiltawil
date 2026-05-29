#include "PdfIndirectObjectLoader.h"
#include "PdfObjectParser.h"

static void setWarning(PdfLoadResult &r, PdfIndirectObjectLoaderWarning w) {
    if (r.warning == PdfIndirectObjectLoaderWarning::None || static_cast<int>(w) < static_cast<int>(r.warning)) {
        r.warning = w;
    }
}

PdfLoadResult PdfIndirectObjectLoader::load(PdfByteReader &reader,
                                              const PdfXRefTable &xrefTable,
                                              PdfObjectStore &store) {
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
                setWarning(result, PdfIndirectObjectLoaderWarning::UnsupportedStream);
                result.diagnosticMessage = QStringLiteral("Stream object not supported at offset %1").arg(entry.offset);
                result.unsupportedStreamCount++;
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
