#pragma once

#include "PdfObject.h"
#include "PdfStreamBodyReader.h"
#include <QByteArrayView>
#include <QList>
#include <limits>

struct PdfRawStreamObject {
    PdfObjectPtr dictionary;
    QByteArrayView rawBody;
    qsizetype length = 0;
    QList<QByteArray> filters;
    PdfObjectPtr decodeParms;
    PdfRef identity;
    bool hasIdentity = false;
};
