#pragma once

#include "PdfVersion.h"
#include "PdfXRef.h"
#include "PdfXRefParser.h"
#include "PdfTrailer.h"
#include "PdfTrailerParser.h"
#include "PdfObjectStore.h"
#include "PdfCatalog.h"
#include "PdfIndirectObjectLoader.h"
#include "PdfPageTree.h"
#include "PdfPageTreeResolver.h"
#include "PdfRawStreamStore.h"

struct PdfDocument {
    QSharedPointer<const QByteArray> pdfData;

    PdfVersion version;
    PdfXRefTable xrefTable;
    PdfTrailer trailer;
    PdfObjectStore store;
    PdfRawStreamStore streamStore;
    PdfCatalog catalog;
    PdfPageTree pageTree;

    PdfXRefParserWarning xrefWarning = PdfXRefParserWarning::None;
    PdfTrailerParserWarning trailerWarning = PdfTrailerParserWarning::None;
    PdfIndirectObjectLoaderWarning objectLoaderWarning = PdfIndirectObjectLoaderWarning::None;
    PdfPageTreeResolverWarning pageTreeWarning = PdfPageTreeResolverWarning::None;

    int streamLoadedCount = 0;
    int streamFailureCount = 0;
    int duplicateStreamCount = 0;
    int unsupportedStreamCount = 0;

    int pageCount() const { return pageTree.actualCount; }
};
