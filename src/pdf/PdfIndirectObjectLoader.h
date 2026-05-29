#pragma once

#include "PdfByteReader.h"
#include "PdfXRef.h"
#include "PdfObjectStore.h"
#include <QString>

enum class PdfIndirectObjectLoaderWarning : uint8_t {
    None,
    InvalidObjectOffset,
    ParseFailure,
    ObjectNumberMismatch,
    StoreInsertFailure,
    DuplicateObjectKey,
    UnsupportedStream,
};

struct PdfLoadResult {
    PdfIndirectObjectLoaderWarning warning = PdfIndirectObjectLoaderWarning::None;
    QString diagnosticMessage;

    int totalXRefEntries = 0;
    int inUseEntries = 0;
    int freeCount = 0;
    int loadedCount = 0;
    int skippedCount = 0;
    int mismatchCount = 0;
    int duplicateCount = 0;
    int unsupportedStreamCount = 0;

    bool ok() const { return warning == PdfIndirectObjectLoaderWarning::None; }
};

class PdfIndirectObjectLoader {
public:
    PdfLoadResult load(PdfByteReader &reader,
                       const PdfXRefTable &xrefTable,
                       PdfObjectStore &store);
};
