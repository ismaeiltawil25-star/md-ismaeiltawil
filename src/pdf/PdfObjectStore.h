#pragma once

#include "PdfObject.h"
#include <QHash>
#include <QString>

struct PdfObjectKey {
    int objNum = 0;
    int genNum = 0;
    bool operator==(const PdfObjectKey &o) const = default;
};

inline size_t qHash(const PdfObjectKey &k, size_t seed = 0) {
    return qHash(static_cast<quint64>(k.objNum) << 32 | static_cast<quint32>(k.genNum), seed);
}

enum class PdfObjectStoreWarning : uint8_t {
    None,
    DuplicateObjectKey,
    MissingReference,
    InvalidObjectKey,
    NullObject,
};

struct PdfStoreResult {
    PdfObjectStoreWarning warning = PdfObjectStoreWarning::None;
    QString diagnosticMessage;

    bool ok() const { return warning == PdfObjectStoreWarning::None; }
};

struct PdfResolveResult {
    PdfObjectPtr object;
    PdfObjectStoreWarning warning = PdfObjectStoreWarning::None;
    QString diagnosticMessage;

    bool found() const { return object != nullptr && warning == PdfObjectStoreWarning::None; }
};

class PdfObjectStore {
public:
    PdfObjectStore() = default;

    PdfStoreResult add(int objNum, int genNum, PdfObjectPtr object);
    PdfStoreResult add(PdfRef ref, PdfObjectPtr object);

    bool contains(int objNum, int genNum) const;
    bool contains(PdfRef ref) const;

    PdfResolveResult resolve(int objNum, int genNum) const;
    PdfResolveResult resolve(PdfRef ref) const;

    void remove(int objNum, int genNum);
    void remove(PdfRef ref);

    void clear();
    qsizetype count() const;
    bool isEmpty() const;

private:
    QHash<PdfObjectKey, PdfObjectPtr> m_objects;
    static bool isValidKey(int objNum, int genNum);
};
