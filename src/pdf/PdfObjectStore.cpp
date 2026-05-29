#include "PdfObjectStore.h"

bool PdfObjectStore::isValidKey(int objNum, int genNum) {
    return objNum > 0 && genNum >= 0;
}

PdfStoreResult PdfObjectStore::add(int objNum, int genNum, PdfObjectPtr object) {
    if (!isValidKey(objNum, genNum)) {
        PdfStoreResult r;
        r.warning = PdfObjectStoreWarning::InvalidObjectKey;
        r.diagnosticMessage = QStringLiteral("Invalid object key: %1 %2").arg(objNum).arg(genNum);
        return r;
    }

    if (object.isNull()) {
        PdfStoreResult r;
        r.warning = PdfObjectStoreWarning::NullObject;
        r.diagnosticMessage = QStringLiteral("Cannot store null object");
        return r;
    }

    PdfObjectKey key{objNum, genNum};
    bool existed = m_objects.contains(key);
    m_objects.insert(key, object);

    PdfStoreResult r;
    if (existed) {
        r.warning = PdfObjectStoreWarning::DuplicateObjectKey;
        r.diagnosticMessage = QStringLiteral("Duplicate object key: %1 %2").arg(objNum).arg(genNum);
    }
    return r;
}

PdfStoreResult PdfObjectStore::add(PdfRef ref, PdfObjectPtr object) {
    return add(ref.objNum, ref.genNum, object);
}

bool PdfObjectStore::contains(int objNum, int genNum) const {
    if (!isValidKey(objNum, genNum)) return false;
    return m_objects.contains(PdfObjectKey{objNum, genNum});
}

bool PdfObjectStore::contains(PdfRef ref) const {
    return contains(ref.objNum, ref.genNum);
}

PdfResolveResult PdfObjectStore::resolve(int objNum, int genNum) const {
    if (!isValidKey(objNum, genNum)) {
        PdfResolveResult r;
        r.warning = PdfObjectStoreWarning::InvalidObjectKey;
        r.diagnosticMessage = QStringLiteral("Invalid object key: %1 %2").arg(objNum).arg(genNum);
        return r;
    }

    auto it = m_objects.constFind(PdfObjectKey{objNum, genNum});
    if (it == m_objects.constEnd()) {
        PdfResolveResult r;
        r.warning = PdfObjectStoreWarning::MissingReference;
        r.diagnosticMessage = QStringLiteral("Object %1 gen %2 not found").arg(objNum).arg(genNum);
        return r;
    }

    PdfResolveResult r;
    r.object = *it;
    return r;
}

PdfResolveResult PdfObjectStore::resolve(PdfRef ref) const {
    return resolve(ref.objNum, ref.genNum);
}

void PdfObjectStore::remove(int objNum, int genNum) {
    m_objects.remove(PdfObjectKey{objNum, genNum});
}

void PdfObjectStore::remove(PdfRef ref) {
    remove(ref.objNum, ref.genNum);
}

void PdfObjectStore::clear() {
    m_objects.clear();
}

qsizetype PdfObjectStore::count() const {
    return m_objects.size();
}

bool PdfObjectStore::isEmpty() const {
    return m_objects.isEmpty();
}
