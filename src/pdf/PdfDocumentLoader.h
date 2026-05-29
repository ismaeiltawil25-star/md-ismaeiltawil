#pragma once

#include "PdfDocument.h"
#include <QByteArray>
#include <QString>

enum class PdfDocumentLoaderWarning : uint8_t {
    None,
    InvalidHeader,
    MissingEofMarker,
    MissingStartxref,
    InvalidStartxref,
    XrefParseFailure,
    TrailerParseFailure,
    EncryptedUnsupported,
    ObjectLoadFailure,
    CatalogFailure,
    PageTreeFailure,
};

struct PdfDocumentResult {
    PdfDocument document;
    PdfDocumentLoaderWarning warning = PdfDocumentLoaderWarning::None;
    QString diagnosticMessage;

    bool ok() const { return warning == PdfDocumentLoaderWarning::None; }
};

class PdfDocumentLoader {
public:
    PdfDocumentResult load(QByteArrayView data);

private:
    struct PdfStartxrefResult {
        qsizetype offset = -1;
        PdfDocumentLoaderWarning warning = PdfDocumentLoaderWarning::None;
        QString diagnosticMessage;
        bool ok() const { return warning == PdfDocumentLoaderWarning::None; }
    };

    static PdfStartxrefResult findStartxref(QByteArrayView data);
    static bool isStructuralPageTreeWarning(PdfPageTreeResolverWarning w);
};
