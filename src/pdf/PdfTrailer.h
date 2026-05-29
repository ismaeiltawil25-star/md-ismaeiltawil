#pragma once

#include "PdfObject.h"
#include <QByteArray>
#include <optional>
#include <QtTypes>

struct PdfFileId {
    QByteArray first;
    QByteArray second;
};

struct PdfTrailer {
    int size = 0;
    PdfRef root;
    std::optional<qsizetype> prev;
    std::optional<PdfFileId> id;
    bool encrypted = false;
    QSharedPointer<PdfDictionary> rawDict;
};
