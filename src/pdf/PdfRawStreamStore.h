#pragma once

#include "PdfObjectStore.h"
#include "PdfRawStreamObject.h"
#include <QHash>
#include <QString>
#include <cstdint>

enum class PdfRawStreamStoreWarning : uint8_t {
    None,
    InvalidStreamKey,
    DuplicateStreamKey,
};

struct PdfRawStreamStoreResult {
    bool stored = false;
    PdfRawStreamStoreWarning warning = PdfRawStreamStoreWarning::None;
    QString diagnosticMessage;

    bool ok() const { return warning == PdfRawStreamStoreWarning::None; }
};

class PdfRawStreamStore {
public:
    PdfRawStreamStore() = default;

    PdfRawStreamStoreResult add(int objNum, int genNum, const PdfRawStreamObject &stream);
    PdfRawStreamStoreResult add(PdfRef ref, const PdfRawStreamObject &stream);

    const PdfRawStreamObject *get(int objNum, int genNum) const;
    const PdfRawStreamObject *get(PdfRef ref) const;

    bool contains(int objNum, int genNum) const;
    bool contains(PdfRef ref) const;

    void clear();
    qsizetype count() const;
    bool isEmpty() const;

private:
    QHash<PdfObjectKey, PdfRawStreamObject> m_streams;
    static bool isValidKey(int objNum, int genNum);
};
