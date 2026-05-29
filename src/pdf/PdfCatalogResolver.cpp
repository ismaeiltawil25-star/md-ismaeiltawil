#include "PdfCatalogResolver.h"

PdfCatalogResult PdfCatalogResolver::resolve(const PdfTrailer &trailer,
                                               const PdfObjectStore &store) {
    PdfCatalogResult result;

    if (trailer.root.objNum <= 0 || trailer.root.genNum < 0) {
        result.warning = PdfCatalogResolverWarning::MissingRootReference;
        result.diagnosticMessage = QStringLiteral("Invalid root reference: %1 %2 R")
            .arg(trailer.root.objNum).arg(trailer.root.genNum);
        return result;
    }

    result.catalog.rootRef = trailer.root;

    auto resolved = store.resolve(trailer.root);
    if (!resolved.found()) {
        result.warning = PdfCatalogResolverWarning::UnresolvedRoot;
        result.diagnosticMessage = QStringLiteral("Root object %1 %2 R not found in store")
            .arg(trailer.root.objNum).arg(trailer.root.genNum);
        return result;
    }

    auto &root = resolved.object;
    if (root->kind != PdfObjectKind::Dictionary) {
        result.warning = PdfCatalogResolverWarning::RootNotDictionary;
        result.diagnosticMessage = QStringLiteral("Root object is not a dictionary");
        return result;
    }

    auto &dict = root->dictValue;
    result.catalog.rawDict = dict;

    auto typeObj = dict->get("Type");
    if (typeObj) {
        if (typeObj->kind != PdfObjectKind::Name || typeObj->nameValue != "Catalog") {
            result.warning = PdfCatalogResolverWarning::InvalidCatalogType;
            result.diagnosticMessage = QStringLiteral("Root object /Type is not Catalog");
            return result;
        }
    }

    auto pagesObj = dict->get("Pages");
    if (!pagesObj) {
        result.warning = PdfCatalogResolverWarning::MissingPagesReference;
        result.diagnosticMessage = QStringLiteral("Catalog dictionary missing /Pages entry");
        return result;
    }

    if (pagesObj->kind != PdfObjectKind::Reference) {
        result.warning = PdfCatalogResolverWarning::InvalidPagesReference;
        result.diagnosticMessage = QStringLiteral("/Pages is not a reference");
        return result;
    }

    if (pagesObj->refValue.objNum <= 0 || pagesObj->refValue.genNum < 0) {
        result.warning = PdfCatalogResolverWarning::InvalidPagesReference;
        result.diagnosticMessage = QStringLiteral("Invalid /Pages reference: %1 %2 R")
            .arg(pagesObj->refValue.objNum).arg(pagesObj->refValue.genNum);
        return result;
    }

    result.catalog.pagesRef = pagesObj->refValue;
    return result;
}
