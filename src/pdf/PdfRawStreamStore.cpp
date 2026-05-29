#include "PdfRawStreamStore.h"

bool PdfRawStreamStore::isValidKey(int objNum, int genNum) {
    return objNum > 0 && genNum >= 0;
}

PdfRawStreamStoreResult PdfRawStreamStore::add(int objNum, int genNum, const PdfRawStreamObject &stream) {
    return add(PdfRef{objNum, genNum}, stream);
}

PdfRawStreamStoreResult PdfRawStreamStore::add(PdfRef ref, const PdfRawStreamObject &stream) {
    PdfRawStreamStoreResult result;

    if (!isValidKey(ref.objNum, ref.genNum)) {
        result.warning = PdfRawStreamStoreWarning::InvalidStreamKey;
        result.diagnosticMessage = QStringLiteral("Invalid stream key: %1 %2")
            .arg(ref.objNum).arg(ref.genNum);
        return result;
    }

    PdfObjectKey key{ref.objNum, ref.genNum};
    bool duplicate = m_streams.contains(key);
    m_streams[key] = stream;
    result.stored = true;

    if (duplicate) {
        result.warning = PdfRawStreamStoreWarning::DuplicateStreamKey;
        result.diagnosticMessage = QStringLiteral("Duplicate stream key: %1 %2")
            .arg(ref.objNum).arg(ref.genNum);
    }
    return result;
}

const PdfRawStreamObject *PdfRawStreamStore::get(int objNum, int genNum) const {
    return get(PdfRef{objNum, genNum});
}

const PdfRawStreamObject *PdfRawStreamStore::get(PdfRef ref) const {
    PdfObjectKey key{ref.objNum, ref.genNum};
    auto it = m_streams.constFind(key);
    return it != m_streams.constEnd() ? &(*it) : nullptr;
}

bool PdfRawStreamStore::contains(int objNum, int genNum) const {
    return contains(PdfRef{objNum, genNum});
}

bool PdfRawStreamStore::contains(PdfRef ref) const {
    return m_streams.contains(PdfObjectKey{ref.objNum, ref.genNum});
}

void PdfRawStreamStore::clear() {
    m_streams.clear();
}

qsizetype PdfRawStreamStore::count() const {
    return m_streams.size();
}

bool PdfRawStreamStore::isEmpty() const {
    return m_streams.isEmpty();
}
