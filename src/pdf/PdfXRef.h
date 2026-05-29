#pragma once

#include <QList>
#include <cstdint>
#include <QtTypes>

enum class PdfXRefEntryType : uint8_t {
    InUse,
    Free,
};

struct PdfXRefEntry {
    qsizetype offset = 0;
    int genNum = 0;
    PdfXRefEntryType type = PdfXRefEntryType::Free;
};

struct PdfXRefSubsection {
    int startObjNum = 0;
    int count = 0;
    QList<PdfXRefEntry> entries;
};

struct PdfXRefTable {
    qsizetype startxref = 0;
    QList<PdfXRefSubsection> subsections;

    int totalEntries() const {
        int total = 0;
        for (const auto &s : subsections) total += s.count;
        return total;
    }
};
