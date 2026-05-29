#pragma once

#include "PdfObject.h"

struct PdfCatalog {
    PdfRef rootRef;
    PdfRef pagesRef;
    QSharedPointer<PdfDictionary> rawDict;
};
