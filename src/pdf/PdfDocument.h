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

struct PdfDocument {
    PdfVersion version;
    PdfXRefTable xrefTable;
    PdfTrailer trailer;
    PdfObjectStore store;
    PdfCatalog catalog;
    PdfPageTree pageTree;

    PdfXRefParserWarning xrefWarning = PdfXRefParserWarning::None;
    PdfTrailerParserWarning trailerWarning = PdfTrailerParserWarning::None;
    PdfIndirectObjectLoaderWarning objectLoaderWarning = PdfIndirectObjectLoaderWarning::None;
    PdfPageTreeResolverWarning pageTreeWarning = PdfPageTreeResolverWarning::None;

    int pageCount() const { return pageTree.actualCount; }
};
