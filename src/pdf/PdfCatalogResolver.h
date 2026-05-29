#pragma once

#include "PdfTrailer.h"
#include "PdfObjectStore.h"
#include "PdfCatalog.h"
#include <QString>

enum class PdfCatalogResolverWarning : uint8_t {
    None,
    MissingRootReference,
    UnresolvedRoot,
    RootNotDictionary,
    InvalidCatalogType,
    MissingPagesReference,
    InvalidPagesReference,
};

struct PdfCatalogResult {
    PdfCatalog catalog;
    PdfCatalogResolverWarning warning = PdfCatalogResolverWarning::None;
    QString diagnosticMessage;

    bool ok() const { return warning == PdfCatalogResolverWarning::None; }
};

class PdfCatalogResolver {
public:
    PdfCatalogResult resolve(const PdfTrailer &trailer,
                             const PdfObjectStore &store);
};
